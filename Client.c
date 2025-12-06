#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include "common.h"
#define BUFFER_SIZE 1024

int client_socket;

// Function to handle SIGINT signal (Ctrl+C)
void handle_sigint(int sig)
{
    (void)sig; 
    printf("\nDisconnecting from server...\n");
    close(client_socket);
    exit(0);
}

int recv_message(int socket, char *buffer)
{
    static char static_buffer[BUFFER_SIZE * 2] = {0}; // queue-like buffer
    static int buffer_filled = 0;

    memset(buffer, 0, BUFFER_SIZE);

    while (1)
    {
        // Step 1: Check if static_buffer has a complete message
        char *end_ptr = strstr(static_buffer, "END");
        char *expect_ptr = strstr(static_buffer, "EXPECT");
        char *exit_ptr = strstr(static_buffer, "EXIT");

        // Priority order: EXIT > END > EXPECT
        if (exit_ptr != NULL &&
            (end_ptr == NULL || exit_ptr < end_ptr) &&
            (expect_ptr == NULL || exit_ptr < expect_ptr))
        {
            *exit_ptr = '\0';
            strncpy(buffer, static_buffer, BUFFER_SIZE - 1);

            int remaining = buffer_filled - (exit_ptr - static_buffer + 4); // 4 = strlen("EXIT")
            memmove(static_buffer, exit_ptr + 4, remaining);
            static_buffer[remaining] = '\0';
            buffer_filled = remaining;

            return 2; // EXIT found
        }
        else if (end_ptr != NULL &&
                 (expect_ptr == NULL || end_ptr < expect_ptr))
        {
            *end_ptr = '\0';
            strncpy(buffer, static_buffer, BUFFER_SIZE - 1);

            int remaining = buffer_filled - (end_ptr - static_buffer + 3); // 3 = strlen("END")
            memmove(static_buffer, end_ptr + 3, remaining);
            static_buffer[remaining] = '\0';
            buffer_filled = remaining;

            return 0; // END found
        }
        else if (expect_ptr != NULL)
        {
            *expect_ptr = '\0';
            strncpy(buffer, static_buffer, BUFFER_SIZE - 1);

            int remaining = buffer_filled - (expect_ptr - static_buffer + 6); // 6 = strlen("EXPECT")
            memmove(static_buffer, expect_ptr + 6, remaining);
            static_buffer[remaining] = '\0';
            buffer_filled = remaining;

            return 1; // EXPECT found
        }

        // Step 2: If marker not found, receive more data into temp
        char temp[BUFFER_SIZE];
        int bytes = recv(socket, temp, BUFFER_SIZE - 1, 0);
        if (bytes <= 0)
            return -1;

        temp[bytes] = '\0';

        // Append to static_buffer
        if (buffer_filled + bytes >= BUFFER_SIZE * 2)
            return -1; // overflow
        strcat(static_buffer, temp);
        buffer_filled += bytes;
    }

    return -1;
}





void send_message(int socket, const char *message)
{
    send(socket, message, strlen(message), 0);
    send(socket, "END", 3, 0); // Always send marker at end
    fflush(stdout);
}


int main(int argc, char *argv[])
{
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = {0};
    char input[BUFFER_SIZE] = {0};

    // Set up signal handler
    signal(SIGINT, handle_sigint);

    // Create socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (argc > 1)
    {
        // Use provided IP address
        if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0)
        {
            perror("Invalid address");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        // Use localhost by default
        if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)
        {
            perror("Invalid address");
            exit(EXIT_FAILURE);
        }
    }

    // Connect to server
    printf("Connecting to server...\n");
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n\n");
    int f = 0;

    // Main communication loop
    while (1)
    {
        // Clear buffer
        memset(buffer, 0, BUFFER_SIZE);
        memset(input, 0, BUFFER_SIZE);

        // Receive data from server
        f = recv_message(client_socket, buffer);

        if (f == 2)
        {
            printf("Server disconnected.\n");
            break;
        }

        // Display received message
        printf("%s", buffer);

        // Get user input
        if (f == 1 && fgets(input, BUFFER_SIZE, stdin) != NULL)
        {
            // Remove newline character
            input[strcspn(input, "\n")] = 0;

            // Send input to server
            send_message(client_socket, input);

            // Check if user wants to exit
            if (strcmp(input, "5") == 0 && strstr(buffer, "EXIT") != NULL)
            {
                // Receive goodbye message
                memset(buffer, 0, BUFFER_SIZE);
                int check = recv_message(client_socket, buffer);
                if (check < 0)
                {
                    printf("Unknown Error...\n");
                }
                printf("%s", buffer);
                break;
            }
        }
    }

    // Close socket
    close(client_socket);
    return 0;
}