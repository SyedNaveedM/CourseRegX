#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <semaphore.h>
#include <ctype.h>

#include "common.h"
#include "utils.h"

// the semaphores used for locking while adding students and faculty
extern sem_t student_sem;
extern sem_t faculty_sem;


// function prototypes
void admin_menu(int client_socket);
void add_student(int client_socket);
void view_student_details(int client_socket);
void add_faculty(int client_socket);
void view_faculty_details(int client_socket);
void activate_student(int client_socket);
void block_student(int client_socket);
void modify_student_details(int client_socket);
void modify_faculty_details(int client_socket);

void admin_menu(int client_socket)
{
    char buffer[BUFFER_SIZE];
    int choice;

    while (1)
    {
        strcpy(buffer, "\n........ Welcome to Admin Menu ........\n"
                       "1. Add Student\n"
                       "2. View Student Details\n"
                       "3. Add Faculty\n"
                       "4. View Faculty Details\n"
                       "5. Activate Student\n"
                       "6. Block Student\n"
                       "7. Modify Student details\n"
                       "8. Modify Faculty Details\n"
                       "9. Logout and Exit\n"
                       "Enter your choice: ");

        send_message_expect(client_socket, buffer);
        memset(buffer, 0, BUFFER_SIZE);

        int check = recv_message(client_socket, buffer);
        if (check < 0)
        {
            send_message(client_socket, "Unknown Error Occurred\n");
            return;
        }

        buffer[strcspn(buffer, "\n")] = 0;

        // error checking of the entered choice
        int is_valid_number = 1;
        if (strlen(buffer) == 0) {
            is_valid_number = 0;
        } else {
            for (int i = 0; buffer[i] != '\0'; i++) {
                if (!isdigit(buffer[i])) {
                    is_valid_number = 0;
                    break;
                }
            }
        }

        if (!is_valid_number) {
            choice = 0;
        } else {
            choice = atoi(buffer);
        }

        send_message(client_socket, "\n"); // Add a newline after the choice is entered

        switch (choice)
        {
            case 1:
                add_student(client_socket);
                break;
            case 2:
                view_student_details(client_socket);
                break;
            case 3:
                add_faculty(client_socket);
                break;
            case 4:
                view_faculty_details(client_socket);
                break;
            case 5:
                activate_student(client_socket);
                break;
            case 6:
                block_student(client_socket);
                break;
            case 7:
                modify_student_details(client_socket);
                break;
            case 8:
                modify_faculty_details(client_socket);
                break;
            case 9:
                send_message(client_socket, "Logging out...\n");
                exit_client_logged_in(client_socket);
                return;
            default:
                strcpy(buffer, "Invalid choice. Please try again.\n");
                send_message(client_socket, buffer);
                break;
        }

        memset(buffer, 0, BUFFER_SIZE);
    }
}

void add_student(int client_socket)
{
    char buffer[BUFFER_SIZE];
    Student student;
    int fd, last_id = 0;
    int duplicate_found = 0;

    strcpy(buffer, "Enter student name: ");
    send_message_expect(client_socket, buffer);
    memset(buffer, 0, BUFFER_SIZE);

    int check = recv_message(client_socket, buffer);
    if (check < 0)
    {
        send_message(client_socket, "Unknown Error Occurred\n");
        return;
    }
    buffer[strcspn(buffer, "\n")] = 0;
    strcpy(student.name, buffer);

    // lock students.dat for adding a new student
    sem_wait(&student_sem);
    fd = open("students.dat", O_RDONLY);
    if (fd != -1)
    {
        Student temp;
        // check for duplicate usernames
        while (read(fd, &temp, sizeof(Student)) > 0)
        {
            if (strcmp(student.name, temp.name) == 0)
            {
                duplicate_found = 1;
                break;
            }
            last_id = temp.id;
        }
        close(fd);
    }

    if (duplicate_found)
    {
        strcpy(buffer, "Error: Student with this username already exists.\n");
        send_message(client_socket, buffer);
        sem_post(&student_sem);
        return;
    }

    strcpy(buffer, "Enter student password: ");
    send_message_expect(client_socket, buffer);
    memset(buffer, 0, BUFFER_SIZE);

    check = recv_message(client_socket, buffer);
    if (check < 0)
    {
        send_message(client_socket, "Unknown Error Occurred\n");
        sem_post(&student_sem);
        return;
    }
    buffer[strcspn(buffer, "\n")] = 0;
    strcpy(student.password, buffer);

    // default
    student.active = 1;

    student.id = last_id + 1;

    fd = open("students.dat", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1)
    {
        strcpy(buffer, "Error: Could not open student database for writing.\n");
        send_message(client_socket, buffer);
        sem_post(&student_sem);
        return;
    }

    write(fd, &student, sizeof(Student));
    close(fd);

    // wrote the student into the file, now unlock
    sem_post(&student_sem);

    sprintf(buffer, "Student added successfully! ID: %d\n", student.id);
    send_message(client_socket, buffer);
}

void view_student_details(int client_socket) {
    char buffer[BUFFER_SIZE];
    int fd;
    Student student;
    int found = 0;

    sem_wait(&student_sem);

    fd = open("students.dat", O_RDONLY);
    if (fd == -1) {
        strcpy(buffer, "Error: Could not open student database.\n");
        send_message(client_socket, buffer);
        sem_post(&student_sem);
        return;
    }

    //  only show usernames, not passwords
    send_message(client_socket, "Student Details:\n");
    while (read(fd, &student, sizeof(Student)) > 0) {
        sprintf(buffer, "ID: %d, Name: %s, Status: %s\n", student.id, student.name, student.active ? "Active" : "Blocked");
        send_message(client_socket, buffer);
        found = 1;
    }

    if (!found) {
        send_message(client_socket, "No students found.\n");
    }

    close(fd);
    sem_post(&student_sem);
}

void add_faculty(int client_socket)
{
    char buffer[BUFFER_SIZE];
    Faculty faculty;
    int fd, last_id = 0;
    int duplicate_found = 0;

    strcpy(buffer, "Enter faculty name: ");
    send_message_expect(client_socket, buffer);
    memset(buffer, 0, BUFFER_SIZE);

    int check = recv_message(client_socket, buffer);
    if (check < 0)
    {
        send_message(client_socket, "Unknown Error Occurred\n");
        return;
    }
    buffer[strcspn(buffer, "\n")] = 0;
    strcpy(faculty.name, buffer);

    // lock
    sem_wait(&faculty_sem);
    fd = open("faculty.dat", O_RDONLY);
    if (fd != -1)
    {
        Faculty temp;
        while (read(fd, &temp, sizeof(Faculty)) > 0)
        {
            if (strcmp(faculty.name, temp.name) == 0)
            {
                duplicate_found = 1;
                break;
            }
            last_id = temp.id;
        }
        close(fd);
    }

    if (duplicate_found)
    {
        strcpy(buffer, "Error: Faculty with this username already exists.\n");
        send_message(client_socket, buffer);
        sem_post(&faculty_sem);
        return;
    }

    strcpy(buffer, "Enter faculty password: ");
    send_message_expect(client_socket, buffer);
    memset(buffer, 0, BUFFER_SIZE);

    check = recv_message(client_socket, buffer);
    if (check < 0)
    {
        send_message(client_socket, "Unknown Error Occurred\n");
        sem_post(&faculty_sem);
        return;
    }
    buffer[strcspn(buffer, "\n")] = 0;
    strcpy(faculty.password, buffer);

    faculty.id = last_id + 1;

    fd = open("faculty.dat", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1)
    {
        strcpy(buffer, "Error: Could not open faculty database for writing.\n");
        send_message(client_socket, buffer);
        sem_post(&faculty_sem);
        return;
    }

    write(fd, &faculty, sizeof(Faculty));
    close(fd);

    // unlock
    sem_post(&faculty_sem);

    sprintf(buffer, "Faculty added successfully! ID: %d\n", faculty.id);
    send_message(client_socket, buffer);
}

// same as student details, just open faculty.dat instead
void view_faculty_details(int client_socket) {
    char buffer[BUFFER_SIZE];
    int fd;
    Faculty faculty;
    int found = 0;

    sem_wait(&faculty_sem);

    fd = open("faculty.dat", O_RDONLY);
    if (fd == -1) {
        strcpy(buffer, "Error: Could not open faculty database.\n");
        send_message(client_socket, buffer);
        sem_post(&faculty_sem);
        return;
    }

    send_message(client_socket, "Faculty Details:\n");
    while (read(fd, &faculty, sizeof(Faculty)) > 0) {
        sprintf(buffer, "ID: %d, Name: %s\n", faculty.id, faculty.name);
        send_message(client_socket, buffer);
        found = 1;
    }

    if (!found) {
        send_message(client_socket, "No faculty found.\n");
    }

    close(fd);
    sem_post(&faculty_sem);
}


void activate_student(int client_socket)
{
    char buffer[BUFFER_SIZE];
    int student_id, fd, found = 0;
    Student student;

    strcpy(buffer, "Enter student ID to activate: ");
    send_message_expect(client_socket, buffer);
    memset(buffer, 0, BUFFER_SIZE);

    int check = recv_message(client_socket, buffer);
    if (check < 0)
    {
        send_message(client_socket, "Unknown Error Occurred\n");
        return;
    }
    buffer[strcspn(buffer, "\n")] = 0;

    student_id = atoi(buffer);
    if (student_id <= 0) {
        send_message(client_socket, "Invalid Student ID.\n");
        return;
    }

    sem_wait(&student_sem);

    // here, we will use a temporary file for updating. this is safe for we update only if there are no errors

    int temp_fd = open("temp_students.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd == -1)
    {
        strcpy(buffer, "Error: Could not create temporary file.\n");
        send_message(client_socket, buffer);
        sem_post(&student_sem);
        return;
    }

    fd = open("students.dat", O_RDONLY);
    if (fd == -1)
    {
        strcpy(buffer, "Error: Could not open student database.\n");
        send_message(client_socket, buffer);
        close(temp_fd);
        unlink("temp_students.dat");
        sem_post(&student_sem);
        return;
    }

    while (read(fd, &student, sizeof(Student)) > 0)
    {
        if (student.id == student_id)
        {
            if(student.active==1)
            {
                strcpy(buffer, "Error: Student already activated.\n");
                close(fd);
                close(temp_fd); 
                
                unlink("temp_students.dat");
                // unlock
                sem_post(&student_sem);
                send_message(client_socket, buffer);

                return;
            }
            found = 1;
            student.active = 1;
        }
        write(temp_fd, &student, sizeof(Student));
    }

    close(fd);
    close(temp_fd);

    if (found)
    {
        if (rename("temp_students.dat", "students.dat") == 0)
        {
            sprintf(buffer, "Student ID %d activated successfully.\n", student_id);
        }
        else
        {
            strcpy(buffer, "Error: Failed to activate student.\n");
            unlink("temp_students.dat");
        }
    }
    else
    {
        unlink("temp_students.dat");
        strcpy(buffer, "Error: Student not found.\n");
    }

    sem_post(&student_sem);
    send_message(client_socket, buffer);
}



void block_student(int client_socket)
{
    char buffer[BUFFER_SIZE];
    int student_id, fd, found = 0;
    Student student;

    strcpy(buffer, "Enter student ID to block: ");
    send_message_expect(client_socket, buffer);
    memset(buffer, 0, BUFFER_SIZE);

    int check = recv_message(client_socket, buffer);
    if (check < 0)
    {
        send_message(client_socket, "Unknown Error Occurred\n");
        return;
    }
    buffer[strcspn(buffer, "\n")] = 0;

    student_id = atoi(buffer);
    if (student_id <= 0) {
        send_message(client_socket, "Invalid Student ID.\n");
        return;
    }

    sem_wait(&student_sem);

    int temp_fd = open("temp_students.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd == -1)
    {
        strcpy(buffer, "Error: Could not create temporary file.\n");
        send_message(client_socket, buffer);
        sem_post(&student_sem);
        return;
    }

    fd = open("students.dat", O_RDONLY);
    if (fd == -1)
    {
        strcpy(buffer, "Error: Could not open student database.\n");
        send_message(client_socket, buffer);
        close(temp_fd);
        unlink("temp_students.dat");
        sem_post(&student_sem);
        return;
    }

    // toggle his status to 0
    while (read(fd, &student, sizeof(Student)) > 0)
    {
        if (student.id == student_id)
        {
            if(student.active==0)
            {
                strcpy(buffer, "Error: Student already blocked.\n");
                close(fd);
                close(temp_fd); 
                
                unlink("temp_students.dat");
                // unlock
                sem_post(&student_sem);
                send_message(client_socket, buffer);

                return;
            }
            found = 1;
            student.active = 0;
        }
        write(temp_fd, &student, sizeof(Student));
    }

    close(fd);
    close(temp_fd);

    if (found)
    {
        if (rename("temp_students.dat", "students.dat") == 0)
        {
            sprintf(buffer, "Student ID %d blocked successfully.\n", student_id);
        }
        else
        {
            strcpy(buffer, "Error: Failed to block student.\n");
            unlink("temp_students.dat");
        }
    }
    else
    {
        unlink("temp_students.dat");
        strcpy(buffer, "Error: Student not found.\n");
    }

    sem_post(&student_sem);
    send_message(client_socket, buffer);
}

void modify_student_details(int client_socket)
{
    char buffer[BUFFER_SIZE];
    int student_id, fd, found = 0;
    Student student;
    char new_name[50];
    char new_password[50];
    int duplicate_found = 0;

    strcpy(buffer, "Enter Student ID to modify: ");
    send_message_expect(client_socket, buffer);
    memset(buffer, 0, BUFFER_SIZE);

    int check = recv_message(client_socket, buffer);
    if (check < 0)
    {
        send_message(client_socket, "Unknown Error Occurred\n");
        return;
    }
    buffer[strcspn(buffer, "\n")] = 0;

    student_id = atoi(buffer);
    if (student_id <= 0) {
        send_message(client_socket, "Invalid Student ID.\n");
        return;
    }

    strcpy(buffer, "Enter new name (or leave blank to keep current): ");
    send_message_expect(client_socket, buffer);
    memset(buffer, 0, BUFFER_SIZE);

    check = recv_message(client_socket, buffer);
    if (check < 0)
    {
        send_message(client_socket, "Unknown Error Occurred\n");
        return;
    }
    buffer[strcspn(buffer, "\n")] = 0;
    strcpy(new_name, buffer);

    if (strlen(new_name) > 0)
    {
        sem_wait(&student_sem);
        fd = open("students.dat", O_RDONLY);
        if (fd != -1)
        {
            Student temp_student;
            while (read(fd, &temp_student, sizeof(Student)) > 0)
            {
                if (temp_student.id != student_id && strcmp(new_name, temp_student.name) == 0)
                {
                    duplicate_found = 1;
                    break;
                }
            }
            close(fd);
        }
        sem_post(&student_sem);
    }

    if (duplicate_found)
    {
        strcpy(buffer, "Error: A user with this name already exists.\n");
        send_message(client_socket, buffer);
        return;
    }

    strcpy(buffer, "Enter new password (or leave blank to keep current): ");
    send_message_expect(client_socket, buffer);
    memset(buffer, 0, BUFFER_SIZE);

    check = recv_message(client_socket, buffer);
    if (check < 0)
    {
        send_message(client_socket, "Unknown Error Occurred\n");
        return;
    }
    buffer[strcspn(buffer, "\n")] = 0;
    strcpy(new_password, buffer);

    sem_wait(&student_sem);

    int temp_fd = open("temp_students.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd == -1)
    {
        strcpy(buffer, "Error: Could not create temporary file.\n");
        send_message(client_socket, buffer);
        sem_post(&student_sem);
        return;
    }

    fd = open("students.dat", O_RDONLY);
    if (fd == -1)
    {
        strcpy(buffer, "Error: Could not open student database.\n");
        send_message(client_socket, buffer);
        close(temp_fd);
        unlink("temp_students.dat");
        sem_post(&student_sem);
        return;
    }

    while (read(fd, &student, sizeof(Student)) > 0)
    {
        if (student.id == student_id)
        {
            found = 1;
            if (strlen(new_name) > 0)
            {
                strcpy(student.name, new_name);
            }
            if (strlen(new_password) > 0)
            {
                strcpy(student.password, new_password);
            }
        }
        write(temp_fd, &student, sizeof(Student));
    }

    close(fd);
    close(temp_fd);

    if (found)
    {
        if (rename("temp_students.dat", "students.dat") == 0)
        {
            strcpy(buffer, "Student details updated successfully.\n");
        }
        else
        {
            strcpy(buffer, "Error: Failed to update student details.\n");
            unlink("temp_students.dat");
        }
    }
    else
    {
        unlink("temp_students.dat");
        strcpy(buffer, "Error: Student not found.\n");
    }

    sem_post(&student_sem);
    send_message(client_socket, buffer);
}

void modify_faculty_details(int client_socket)
{
    char buffer[BUFFER_SIZE];
    int faculty_id, fd, found = 0;
    Faculty faculty;
    char new_name[50];
    char new_password[50];
    int duplicate_found = 0;

    strcpy(buffer, "Enter Faculty ID to modify: ");
    send_message_expect(client_socket, buffer);
    memset(buffer, 0, BUFFER_SIZE);

    int check = recv_message(client_socket, buffer);
    if (check < 0)
    {
        send_message(client_socket, "Unknown Error Occurred\n");
        return;
    }
    buffer[strcspn(buffer, "\n")] = 0;

    faculty_id = atoi(buffer);
    if (faculty_id <= 0) {
        send_message(client_socket, "Invalid Faculty ID.\n");
        return;
    }

    strcpy(buffer, "Enter new name (or leave blank to keep current): ");
    send_message_expect(client_socket, buffer);
    memset(buffer, 0, BUFFER_SIZE);

    check = recv_message(client_socket, buffer);
    if (check < 0)
    {
        send_message(client_socket, "Unknown Error Occurred\n");
        return;
    }
    buffer[strcspn(buffer, "\n")] = 0;
    strcpy(new_name, buffer);

    if (strlen(new_name) > 0)
    {
        sem_wait(&faculty_sem);
        fd = open("faculty.dat", O_RDONLY);
        if (fd != -1)
        {
            Faculty temp_faculty;
            while (read(fd, &temp_faculty, sizeof(Faculty)) > 0)
            {
                if (temp_faculty.id != faculty_id && strcmp(new_name, temp_faculty.name) == 0)
                {
                    duplicate_found = 1;
                    break;
                }
            }
            close(fd);
        }
        sem_post(&faculty_sem);
    }

    if (duplicate_found)
    {
        strcpy(buffer, "Error: A user with this name already exists.\n");
        send_message(client_socket, buffer);
        return;
    }

    strcpy(buffer, "Enter new password (or leave blank to keep current): ");
    send_message_expect(client_socket, buffer);
    memset(buffer, 0, BUFFER_SIZE);

    check = recv_message(client_socket, buffer);
    if (check < 0)
    {
        send_message(client_socket, "Unknown Error Occurred\n");
        return;
    }
    buffer[strcspn(buffer, "\n")] = 0;
    strcpy(new_password, buffer);

    sem_wait(&faculty_sem);

    int temp_fd = open("temp_faculty.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd == -1)
    {
        strcpy(buffer, "Error: Could not create temporary file.\n");
        send_message(client_socket, buffer);
        sem_post(&faculty_sem);
        return;
    }

    fd = open("faculty.dat", O_RDONLY);
    if (fd == -1)
    {
        strcpy(buffer, "Error: Could not open faculty database.\n");
        send_message(client_socket, buffer);
        close(temp_fd);
        unlink("temp_faculty.dat");
        sem_post(&faculty_sem);
        return;
    }

    while (read(fd, &faculty, sizeof(Faculty)) > 0)
    {
        if (faculty.id == faculty_id)
        {
            found = 1;
            if (strlen(new_name) > 0)
            {
                strcpy(faculty.name, new_name);
            }
            if (strlen(new_password) > 0)
            {
                strcpy(faculty.password, new_password);
            }
        }
        write(temp_fd, &faculty, sizeof(Faculty));
    }

    close(fd);
    close(temp_fd);

    if (found)
    {
        if (rename("temp_faculty.dat", "faculty.dat") == 0)
        {
            strcpy(buffer, "Faculty details updated successfully.\n");
        }
        else
        {
            strcpy(buffer, "Error: Failed to update faculty details.\n");
            unlink("temp_faculty.dat");
        }
    }
    else
    {
        unlink("temp_faculty.dat");
        strcpy(buffer, "Error: Faculty not found.\n");
    }

    sem_post(&faculty_sem);
    send_message(client_socket, buffer);
}