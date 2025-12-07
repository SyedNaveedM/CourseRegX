#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <pthread.h>
#include "utils.h"
#include "common.h"

static char static_buffer[BUFFER_SIZE * 2] = {0};
static int buffer_filled = 0;
static int current_fd = -1;

#ifdef TESTING
void reset_recv_buffer(void)
{
    static_buffer[0] = '\0';
    buffer_filled = 0;
    current_fd = -1;
}
#endif

int authenticate(int client_socket, int role)
{
    if (role < 1 || role > 3)
    {
        send_message(client_socket, "Invalid choice entered. ");
        return -1;
    }

    char buffer[BUFFER_SIZE] = {0};
    char username[50], password[50];
    int fd, id = -1;
    int found = 0;

    send_message_expect(client_socket, "Enter username: ");
    memset(buffer, 0, BUFFER_SIZE);

    if (recv_message(client_socket, buffer) < 0)
        send_message(client_socket, "Unknown Error Occurred\n");

    strcpy(username, buffer);

    send_message_expect(client_socket, "Enter password: ");
    memset(buffer, 0, BUFFER_SIZE);

    if (recv_message(client_socket, buffer) < 0)
        send_message(client_socket, "Unknown Error Occurred\n");

    strcpy(password, buffer);

    if (role == 1)
    {
        if (strcmp(username, "admin") == 0 &&
            strcmp(password, "admin123") == 0)
        {
            send_message(client_socket, "Authentication successful!\n");
            return 1;
        }
    }
    else if (role == 2)
    {
        Faculty faculty;
        sem_wait(&faculty_sem);

        fd = open("faculty.dat", O_RDONLY);
        if (fd != -1)
        {
            while (read(fd, &faculty, sizeof(Faculty)) > 0)
            {
                if (strcmp(username, faculty.name) == 0 &&
                    strcmp(password, faculty.password) == 0)
                {
                    found = 1;
                    id = faculty.id;
                    break;
                }
            }
            close(fd);
        }

        sem_post(&faculty_sem);
    }
    else if (role == 3)
    {
        Student student;
        sem_wait(&student_sem);

        fd = open("students.dat", O_RDONLY);
        if (fd != -1)
        {
            while (read(fd, &student, sizeof(Student)) > 0)
            {
                if (strcmp(username, student.name) == 0 &&
                    strcmp(password, student.password) == 0)
                {
                    if (student.active)
                    {
                        found = 1;
                        id = student.id;
                    }
                    else
                    {
                        send_message(client_socket,
                                     "Your account is deactivated. Contact admin.\n");
                        sem_post(&student_sem);
                        return -1;
                    }
                    break;
                }
            }
            close(fd);
        }

        sem_post(&student_sem);
    }

    if (found)
    {
        send_message(client_socket, "Authentication successful!\n");
        return id;
    }

    send_message(client_socket,
                 "Authentication failed! Incorrect username or password.\n");
    return -1;
}

void change_password(int client_socket, int id, int role)
{
    char buffer[BUFFER_SIZE];
    char old_password[50], new_password[50];
    int fd, found = 0;

    send_message_expect(client_socket, "Enter current password: ");
    memset(buffer, 0, BUFFER_SIZE);

    if (recv_message(client_socket, buffer) < 0)
        send_message(client_socket, "Unknown Error Occurred\n");

    strcpy(old_password, buffer);

    send_message_expect(client_socket, "Enter new password: ");
    memset(buffer, 0, BUFFER_SIZE);

    if (recv_message(client_socket, buffer) < 0)
        send_message(client_socket, "Unknown Error Occurred\n");

    strcpy(new_password, buffer);

    if (role == 2)
    {
        Faculty faculty;
        sem_wait(&faculty_sem);

        int temp_fd = open("temp_faculty.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (temp_fd == -1)
        {
            send_message(client_socket, "Error: Could not create temporary file.\n");
            sem_post(&faculty_sem);
            return;
        }

        fd = open("faculty.dat", O_RDONLY);
        if (fd == -1)
        {
            send_message(client_socket, "Error: Could not open faculty database.\n");
            close(temp_fd);
            sem_post(&faculty_sem);
            return;
        }

        while (read(fd, &faculty, sizeof(Faculty)) > 0)
        {
            if (faculty.id == id)
            {
                found = 1;

                if (strcmp(faculty.password, old_password) != 0)
                {
                    send_message(client_socket, "Error: Current password incorrect.\n");
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
                send_message(client_socket, "Password changed successfully.\n");
            else
                send_message(client_socket, "Error: Failed to update password.\n");
        }
        else
        {
            unlink("temp_faculty.dat");
            send_message(client_socket, "Error: Faculty not found.\n");
        }

        sem_post(&faculty_sem);
        return;
    }

    if (role == 3)
    {
        Student student;
        sem_wait(&student_sem);

        int temp_fd = open("temp_students.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (temp_fd == -1)
        {
            send_message(client_socket, "Error: Could not create temporary file.\n");
            sem_post(&student_sem);
            return;
        }

        fd = open("students.dat", O_RDONLY);
        if (fd == -1)
        {
            send_message(client_socket, "Error: Could not open student database.\n");
            close(temp_fd);
            sem_post(&student_sem);
            return;
        }

        while (read(fd, &student, sizeof(Student)) > 0)
        {
            if (student.id == id)
            {
                found = 1;

                if (strcmp(student.password, old_password) != 0)
                {
                    send_message(client_socket, "Error: Current password incorrect.\n");
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
                send_message(client_socket, "Password changed successfully.\n");
            else
                send_message(client_socket, "Error: Failed to update password.\n");
        }
        else
        {
            unlink("temp_students.dat");
            send_message(client_socket, "Error: Student not found.\n");
        }

        sem_post(&student_sem);
        return;
    }
}

void exit_client(int client_socket)
{
    send_message(client_socket,
                 "Goodbye! Thank you for using Academia Portal.EXIT\n");
}

void exit_client_logged_in(int client_socket)
{
    send_message(client_socket,
                 "Goodbye! Thank you for using Academia Portal.\n");
}

void send_message_expect(int socket, const char *message)
{
    send(socket, message, strlen(message), 0);
    send(socket, "EXPECT", 6, 0);
}

void send_message(int socket, const char *message)
{
    send(socket, message, strlen(message), 0);
    send(socket, "END", 3, 0);
}

int recv_message(int socket, char *buffer)
{

    if (current_fd != socket)
    {
        current_fd = socket;
        buffer_filled = 0;
        static_buffer[0] = '\0';
    }

    memset(buffer, 0, BUFFER_SIZE);

    while (1)
    {
        char *end_ptr = strstr(static_buffer, "END");
        char *expect_ptr = strstr(static_buffer, "EXPECT");
        char *exit_ptr = strstr(static_buffer, "EXIT");

        if (exit_ptr &&
            (!end_ptr || exit_ptr < end_ptr) &&
            (!expect_ptr || exit_ptr < expect_ptr))
        {
            *exit_ptr = '\0';
            strncpy(buffer, static_buffer, BUFFER_SIZE - 1);

            int consumed = (exit_ptr - static_buffer) + 4;
            int remaining = buffer_filled - consumed;

            if (remaining < 0)
                remaining = 0;

            memmove(static_buffer, static_buffer + consumed, remaining);
            static_buffer[remaining] = '\0';
            buffer_filled = remaining;

            return 2;
        }

        if (end_ptr &&
            (!expect_ptr || end_ptr < expect_ptr))
        {
            *end_ptr = '\0';
            strncpy(buffer, static_buffer, BUFFER_SIZE - 1);

            int consumed = (end_ptr - static_buffer) + 3;
            int remaining = buffer_filled - consumed;

            if (remaining < 0)
                remaining = 0;

            memmove(static_buffer, static_buffer + consumed, remaining);
            static_buffer[remaining] = '\0';
            buffer_filled = remaining;

            return 0;
        }

        if (expect_ptr)
        {
            *expect_ptr = '\0';
            strncpy(buffer, static_buffer, BUFFER_SIZE - 1);

            int consumed = (expect_ptr - static_buffer) + 6;
            int remaining = buffer_filled - consumed;

            if (remaining < 0)
                remaining = 0;

            memmove(static_buffer, static_buffer + consumed, remaining);
            static_buffer[remaining] = '\0';
            buffer_filled = remaining;

            return 1;
        }

        char temp[BUFFER_SIZE];
        int bytes = recv(socket, temp, BUFFER_SIZE - 1, 0);

        if (bytes <= 0)
            return -1;

        temp[bytes] = '\0';

        if (buffer_filled + bytes >= BUFFER_SIZE * 2)
            return -1;

        memcpy(static_buffer + buffer_filled, temp, bytes + 1);
        buffer_filled += bytes;
    }

    return -1;
}
