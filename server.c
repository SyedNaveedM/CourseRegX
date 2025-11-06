#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <stdint.h> 

#include "common.h"
#include "utils.h"

void handle_client(int client_socket);
void signal_handler(int sig);

sem_t student_sem, faculty_sem, course_sem;

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    pthread_t thread_id;

    // binary semaphores for locking students.dat, faculty.dat, courses.dat while writing
    sem_init(&student_sem, 0, 1);
    sem_init(&faculty_sem, 0, 1);
    sem_init(&course_sem, 0, 1);

    signal(SIGINT, signal_handler);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started. Waiting for connections...\n");
    fflush(stdout);

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("New client connected: %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        fflush(stdout);

        pthread_create(&thread_id, NULL, (void *)handle_client, (void *)(intptr_t)client_socket);
        printf("\n");
    }

    sem_destroy(&student_sem);
    sem_destroy(&faculty_sem);
    sem_destroy(&course_sem);
    close(server_fd);

    return 0;
}

void signal_handler(int sig) {
    (void)sig; // Cast to void to suppress unused parameter warning

    printf("\nShutting down server...\n");
    fflush(stdout);

    sem_destroy(&student_sem);
    sem_destroy(&faculty_sem);
    sem_destroy(&course_sem);

    exit(0);
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int role, user_id;

    while (1) {
        strcpy(buffer,
            "\n....................Welcome Back to Academia :: Course Registration....................\n"
            "Login Type\n"
            "Enter Your Choice { 1. Admin , 2. Professor, 3. Student, 4. Exit } : "
        );
        send_message_expect(client_socket, buffer);

        memset(buffer, 0, BUFFER_SIZE);

        int check = recv_message(client_socket, buffer);
        if (check < 0) {
            send_message(client_socket, "Unknown Error Occurred\n");
            continue;
        }
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline

        role = atoi(buffer);

        if (role == 4) {
            exit_client(client_socket);
            break;
        }

        user_id = authenticate(client_socket, role);
        if (user_id < 0) {
            send_message(client_socket, "Try again...\n");
            continue;
        }

        switch (role) {
            case 1:
                admin_menu(client_socket);
                break;

            case 2:
                faculty_menu(client_socket, user_id);
                break;

            case 3:
                student_menu(client_socket, user_id);
                break;

            default:
                send_message(client_socket, "Invalid choice. Disconnecting...\n");
                break;
        }
    }

    close(client_socket);
    pthread_exit(NULL);
}