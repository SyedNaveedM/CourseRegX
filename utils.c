#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>

#include "utils.h"
#include "common.h"

// Authentication function
int authenticate(int client_socket, int role)
{
    if(role < 1 || role > 3){
        send_message(client_socket, "Invalid choice entered. ");
        return -1;
    }
    char buffer[BUFFER_SIZE] = {0};
    char username[50], password[50];
    int fd, id = -1;
    int found = 0;

    // Request username
    strcpy(buffer, "Enter username: ");
    send_message_expect(client_socket, buffer);
    memset(buffer, 0, BUFFER_SIZE);
    {
        int check = recv_message(client_socket, buffer);
        if (check < 0)
        {
            send_message(client_socket, "Unknown Error Occurred\n");
        }
    }
    strcpy(username, buffer);

    // Request password
    strcpy(buffer, "Enter password: ");
    send_message_expect(client_socket, buffer);
    memset(buffer, 0, BUFFER_SIZE);
    {
        int check = recv_message(client_socket, buffer);
        if (check < 0)
        {
            send_message(client_socket, "Unknown Error Occurred\n");
        }
    }
    strcpy(password, buffer);

    // Check credentials based on role
    switch (role)
    {
    case 1: // Admin
        if (strcmp(username, "admin") == 0 && strcmp(password, "admin123") == 0)
        {
            strcpy(buffer, "Authentication successful!\n");
            send_message(client_socket, buffer);
            // printf("%s what is this", buffer);
            return 1; // Admin ID is always 1
        }
        break;
    case 2:
    { // Faculty
        Faculty faculty;
        sem_wait(&faculty_sem);
        fd = open("faculty.dat", O_RDONLY);
        if (fd != -1)
        {
            while (read(fd, &faculty, sizeof(Faculty)) > 0)
            {
                if (strcmp(username, faculty.name) == 0 && strcmp(password, faculty.password) == 0)
                {
                    found = 1;
                    id = faculty.id;
                    break;
                }
            }
            close(fd);
        }
        sem_post(&faculty_sem);
        break;
    }
    case 3:
    { // Student
        Student student;
        sem_wait(&student_sem);
        fd = open("students.dat", O_RDONLY);
        if (fd != -1)
        {
            while (read(fd, &student, sizeof(Student)) > 0)
            {
                if (strcmp(username, student.name) == 0 && strcmp(password, student.password) == 0)
                {
                    if (student.active)
                    {
                        found = 1;
                        id = student.id;
                    }
                    else
                    {
                        strcpy(buffer, "Your account is deactivated. Contact admin.\n");
                        send_message(client_socket, buffer);
                        sem_post(&student_sem);
                        return -1;
                    }
                    break;
                }
            }
            close(fd);
        }
        sem_post(&student_sem);
        break;
    }
    }

    if (found)
    {
        strcpy(buffer, "Authentication successful!\n");
        send_message(client_socket, buffer);
        return id;
    }
    else
    {
        // printf("%s\n", buffer);
        strcpy(buffer, "Authentication failed! Incorrect username or password.\n");
        send_message(client_socket, buffer);
        return -1;
    }
}

// Change password function
void change_password(int client_socket, int id, int role)
{
    char buffer[BUFFER_SIZE];
    char old_password[50], new_password[50];
    int fd, found = 0;

    // Get old password
    strcpy(buffer, "Enter current password: ");
    send_message_expect(client_socket, buffer);
    memset(buffer, 0, BUFFER_SIZE);
    {
        int check = recv_message(client_socket, buffer);
        if (check < 0)
        {
            send_message(client_socket, "Unknown Error Occurred\n");
        }
    }
    strcpy(old_password, buffer);
    // printf("old pwd: %s\n", old_password);
    // Get new password
    strcpy(buffer, "Enter new password: ");
    send_message_expect(client_socket, buffer);
    memset(buffer, 0, BUFFER_SIZE);
    {
        int check = recv_message(client_socket, buffer);
        if (check < 0)
        {
            send_message(client_socket, "Unknown Error Occurred\n");
        }
    }
    strcpy(new_password, buffer);

    if (role == 2)
    { // Faculty
        Faculty faculty;
        sem_wait(&faculty_sem);

        // Create a temporary file
        int temp_fd = open("temp_faculty.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (temp_fd == -1)
        {
            strcpy(buffer, "Error: Could not create temporary file.\n");
            send_message(client_socket, buffer);
            sem_post(&faculty_sem);
            return;
        }

        // Open original file
        fd = open("faculty.dat", O_RDONLY);
        if (fd == -1)
        {
            strcpy(buffer, "Error: Could not open faculty database.\n");
            send_message(client_socket, buffer);
            close(temp_fd);
            sem_post(&faculty_sem);
            return;
        }

        // Find and update faculty
        while (read(fd, &faculty, sizeof(Faculty)) > 0)
        {
            if (faculty.id == id)
            {
                found = 1;

                if (strcmp(faculty.password, old_password) != 0)
                {
                    strcpy(buffer, "Error: Current password is incorrect.\n");
                    send_message(client_socket, buffer);
                    close(fd);
                    close(temp_fd);
                    unlink("temp_faculty.dat");
                    sem_post(&faculty_sem);
                    return;
                }

                strcpy(faculty.password, new_password);
            }
            write(temp_fd, &faculty, sizeof(Faculty));
        }

        close(fd);
        close(temp_fd);

        if (found)
        {
            if (rename("temp_faculty.dat", "faculty.dat") == 0)
            {
                strcpy(buffer, "Password changed successfully.\n");
            }
            else
            {
                strcpy(buffer, "Error: Failed to update password.\n");
            }
        }
        else
        {
            unlink("temp_faculty.dat");
            strcpy(buffer, "Error: Faculty not found.\n");
        }

        sem_post(&faculty_sem);
    }
    else if (role == 3)
    { // Student
        Student student;
        sem_wait(&student_sem);

        // Create a temporary file
        int temp_fd = open("temp_students.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (temp_fd == -1)
        {
            strcpy(buffer, "Error: Could not create temporary file.\n");
            send_message(client_socket, buffer);
            sem_post(&student_sem);
            return;
        }

        // Open original file
        fd = open("students.dat", O_RDONLY);
        if (fd == -1)
        {
            strcpy(buffer, "Error: Could not open student database.\n");
            send_message(client_socket, buffer);
            close(temp_fd);
            sem_post(&student_sem);
            return;
        }

        // Find and update student
        while (read(fd, &student, sizeof(Student)) > 0)
        {
            if (student.id == id)
            {
                found = 1;

                if (strcmp(student.password, old_password) != 0)
                {
                    strcpy(buffer, "Error: Current password is incorrect.\n");
                    send_message(client_socket, buffer);
                    close(fd);
                    close(temp_fd);
                    unlink("temp_students.dat");
                    sem_post(&student_sem);
                    return;
                }

                strcpy(student.password, new_password);
            }
            write(temp_fd, &student, sizeof(Student));
        }

        close(fd);
        close(temp_fd);

        if (found)
        {
            if (rename("temp_students.dat", "students.dat") == 0)
            {
                strcpy(buffer, "Password changed successfully.\n");
            }
            else
            {
                strcpy(buffer, "Error: Failed to update password.\n");
            }
        }
        else
        {
            unlink("temp_students.dat");
            strcpy(buffer, "Error: Student not found.\n");
        }

        sem_post(&student_sem);
    }

    send_message(client_socket, buffer);
}

// Exit function
void exit_client(int client_socket)
{
    char buffer[BUFFER_SIZE];
    strcpy(buffer, "Goodbye! Thank you for using Academia Portal.EXIT\n");
    send_message(client_socket, buffer);
}

void  exit_client_logged_in(int client_socket)
{
    char buffer[BUFFER_SIZE];
    strcpy(buffer, "Goodbye! Thank you for using Academia Portal.\n");
    send_message(client_socket, buffer);
}

void send_message_expect(int socket, const char *message)
{
    send(socket, message, strlen(message), 0);
    send(socket, "EXPECT", 6, 0); // Always send marker at end
    fflush(stdout);
}

void send_message(int socket, const char *message)
{
    send(socket, message, strlen(message), 0);
    send(socket, "END", 3, 0); // Always send marker at end
    fflush(stdout);
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





