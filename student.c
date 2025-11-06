#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <semaphore.h>

#include "common.h"
#include "utils.h"

extern sem_t course_sem; // Assuming course_sem is defined in server.c

// Function prototypes for student functions
void student_menu(int client_socket, int student_id);
void view_all_courses(int client_socket);
void enroll_course(int client_socket, int student_id);
void drop_course(int client_socket, int student_id);
void view_enrolled_courses(int client_socket, int student_id);


// Student menu function
void student_menu(int client_socket, int student_id) {
    char buffer[BUFFER_SIZE];
    int choice;

    while (1) {
        strcpy(buffer, "\n--- Student Menu ---\n1. View All Courses\n2. Enroll (pick) new course\n3. Drop Course\n4. View Enrolled Course Details\n5. Change Password\n6. Logout and Exit\n\nEnter your choice: ");
        send_message_expect(client_socket, buffer);
        fflush(stdout);
        memset(buffer, 0, BUFFER_SIZE);

        int check = recv_message(client_socket, buffer);
        if(check < 0){
            send_message(client_socket, "Unknown Error Occurred\n");
            return; // Exit menu loop on communication error
        }
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline
        choice = atoi(buffer);

        switch (choice) {
            case 1:
                view_all_courses(client_socket);
                break;
            case 2:
                enroll_course(client_socket, student_id);
                break;
            case 3:
                drop_course(client_socket, student_id);
                break;
            case 4:
                view_enrolled_courses(client_socket, student_id);
                break;
            case 5:
                change_password(client_socket, student_id, 3); // 3 for student
                break;
            case 6:
                exit_client_logged_in(client_socket);
                return; // Exit the student menu loop
            default:
                strcpy(buffer, "Invalid choice. Please try again.\n");
                send_message(client_socket, buffer);
                fflush(stdout);
                break; // Add break statement
        }
    }
}

// View all courses function
void view_all_courses(int client_socket) {
    char buffer[BUFFER_SIZE];
    Course course;
    int fd;

    strcpy(buffer, "Available courses:\n");
    send_message(client_socket, buffer);

    sem_wait(&course_sem);
    fd = open("courses.dat", O_RDONLY);
    if (fd == -1) {
        strcpy(buffer, "No courses available.\n");
        send_message(client_socket, buffer);
        fflush(stdout);
        sem_post(&course_sem);
        return;
    }

    while (read(fd, &course, sizeof(Course)) > 0) {
        sprintf(buffer, "ID: %d, Name: %s, Available Seats: %d\n",
                course.id, course.name, course.available_seats);
        send_message(client_socket, buffer);
        fflush(stdout);
    }

    close(fd);
    sem_post(&course_sem);
}

// Enroll in course function
void enroll_course(int client_socket, int student_id) {
    char buffer[BUFFER_SIZE];
    Course course;
    int course_id, fd, found = 0, already_enrolled = 0;

    view_all_courses(client_socket);

    strcpy(buffer, "Enter course ID to enroll (0 to cancel): ");
    send_message_expect(client_socket, buffer);
    fflush(stdout);
    memset(buffer, 0, BUFFER_SIZE);
    {int check = recv_message(client_socket, buffer);
        if(check < 0){
            send_message(client_socket, "Unknown Error Occurred\n");
            return; // Return on communication error
        }
    }
    buffer[strcspn(buffer, "\n")] = 0; // Remove newline
    course_id = atoi(buffer);

    if (course_id == 0) {
        strcpy(buffer, "Enrollment canceled.\n");
        send_message(client_socket, buffer);
        fflush(stdout);
        return;
    }

    sem_wait(&course_sem);

    int temp_fd = open("temp_courses.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd == -1) {
        strcpy(buffer, "Error: Could not create temporary file.\n");
        send_message(client_socket, buffer);
        fflush(stdout);
        sem_post(&course_sem);
        return;
    }

    fd = open("courses.dat", O_RDONLY);
    if (fd == -1) {
        strcpy(buffer, "Error: Could not open course database.\n");
        send_message(client_socket, buffer);
        fflush(stdout);
        close(temp_fd);
        unlink("temp_courses.dat");
        sem_post(&course_sem);
        return;
    }

    while (read(fd, &course, sizeof(Course)) > 0) {
        if (course.id == course_id) {
            found = 1;

            for (int i = 0; i < course.enrollment_count; i++) {
                if (course.enrolled_students[i] == student_id) {
                    already_enrolled = 1;
                    break;
                }
            }

            if (already_enrolled) {
                strcpy(buffer, "You are already enrolled in this course.\n");
                send_message(client_socket, buffer);
                fflush(stdout);
            } else if (course.available_seats <= 0) {
                strcpy(buffer, "No available seats in this course.\n");
                send_message(client_socket, buffer);
                fflush(stdout);
            } else {
                course.enrolled_students[course.enrollment_count] = student_id;
                course.enrollment_count++;
                course.available_seats--;

                strcpy(buffer, "Enrolled successfully!\n");
                send_message(client_socket, buffer);
                fflush(stdout);
            }
        }
        write(temp_fd, &course, sizeof(Course));
    }

    close(fd);
    close(temp_fd);

    if (found && !already_enrolled && course.available_seats >= 0) {
        if (rename("temp_courses.dat", "courses.dat") != 0) {
             strcpy(buffer, "Error: Failed to update course file after enrollment.\n");
             send_message(client_socket, buffer);
             fflush(stdout);
             unlink("temp_courses.dat");
        }
    } else {
        unlink("temp_courses.dat");
        if (!found) {
             strcpy(buffer, "Course not found.\n");
             send_message(client_socket, buffer);
             fflush(stdout);
        }
    }

    sem_post(&course_sem);
}

// Drop course function
void drop_course(int client_socket, int student_id) {
    char buffer[BUFFER_SIZE];
    Course course;
    int course_id, fd, found = 0, enrolled = 0;

    strcpy(buffer, "Your enrolled courses:\n");
    send_message(client_socket, buffer);
    fflush(stdout);

    sem_wait(&course_sem);
    fd = open("courses.dat", O_RDONLY);
    if (fd == -1) {
        strcpy(buffer, "No courses available.\n");
        send_message(client_socket, buffer);
        fflush(stdout);
        sem_post(&course_sem);
        return;
    }

    int has_courses = 0;
    while (read(fd, &course, sizeof(Course)) > 0) {
        for (int i = 0; i < course.enrollment_count; i++) {
            if (course.enrolled_students[i] == student_id) {
                sprintf(buffer, "ID: %d, Name: %s\n", course.id, course.name);
                send_message(client_socket, buffer);
                fflush(stdout);
                has_courses = 1;
                break;
            }
        }
    }

    close(fd);
    sem_post(&course_sem);

    if (!has_courses) {
        strcpy(buffer, "You are not enrolled in any courses.\n");
        send_message(client_socket, buffer);
        fflush(stdout);
        return;
    }

    strcpy(buffer, "Enter course ID to drop (0 to cancel): ");
    send_message_expect(client_socket, buffer);
    fflush(stdout);
    memset(buffer, 0, BUFFER_SIZE);
    {int check = recv_message(client_socket, buffer);
        if(check < 0){
            send_message(client_socket, "Unknown Error Occurred\n");
            return; // Return on communication error
        }
    }
     buffer[strcspn(buffer, "\n")] = 0; // Remove newline
    course_id = atoi(buffer);

    if (course_id == 0) {
        strcpy(buffer, "Drop course canceled.\n");
        send_message(client_socket, buffer);
        fflush(stdout);
        return;
    }

    sem_wait(&course_sem);

    int temp_fd = open("temp_courses.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd == -1) {
        strcpy(buffer, "Error: Could not create temporary file.\n");
        send_message(client_socket, buffer);
        fflush(stdout);
        sem_post(&course_sem);
        return;
    }

    fd = open("courses.dat", O_RDONLY);
    if (fd == -1) {
        strcpy(buffer, "Error: Could not open course database.\n");
        send_message(client_socket, buffer);
        fflush(stdout);
        close(temp_fd);
        unlink("temp_courses.dat");
        sem_post(&course_sem);
        return;
    }

    while (read(fd, &course, sizeof(Course)) > 0) {
        if (course.id == course_id) {
            found = 1;

            for (int i = 0; i < course.enrollment_count; i++) {
                if (course.enrolled_students[i] == student_id) {
                    enrolled = 1;

                    for (int j = i; j < course.enrollment_count - 1; j++) {
                        course.enrolled_students[j] = course.enrolled_students[j + 1];
                    }
                    course.enrollment_count--;
                    course.available_seats++;

                    strcpy(buffer, "Course dropped successfully!\n");
                    send_message(client_socket, buffer);
                    fflush(stdout);
                    break;
                }
            }

            if (!enrolled) {
                strcpy(buffer, "You are not enrolled in this course.\n");
                send_message(client_socket, buffer);
                fflush(stdout);
            }
        }
        write(temp_fd, &course, sizeof(Course));
    }

    close(fd);
    close(temp_fd);

    if (found && enrolled) {
        if (rename("temp_courses.dat", "courses.dat") != 0) {
             strcpy(buffer, "Error: Failed to update course file after dropping course.\n");
             send_message(client_socket, buffer);
             fflush(stdout);
             unlink("temp_courses.dat");
        }
    } else {
        unlink("temp_courses.dat");
        if (!found) {
            strcpy(buffer, "Course not found.\n");
            send_message(client_socket, buffer);
            fflush(stdout);
        }
    }

    sem_post(&course_sem);
}

// View enrolled courses function
void view_enrolled_courses(int client_socket, int student_id) {
    char buffer[BUFFER_SIZE];
    Course course;
    int fd, found_course = 0;

    sem_wait(&course_sem);
    fd = open("courses.dat", O_RDONLY);
    if (fd == -1) {
        strcpy(buffer, "No courses available.\n");
        send_message(client_socket, buffer);
        fflush(stdout);
        sem_post(&course_sem);
        return;
    }

    strcpy(buffer, "Your enrolled courses:\n");
    send_message(client_socket, buffer);
    fflush(stdout);

    while (read(fd, &course, sizeof(Course)) > 0) {
        for (int i = 0; i < course.enrollment_count; i++) {
            if (course.enrolled_students[i] == student_id) {
                found_course = 1;
                sprintf(buffer, "ID: %d, Name: %s, Faculty ID: %d\n",
                        course.id, course.name, course.faculty_id);
                send_message(client_socket, buffer);
                fflush(stdout);
                break;
            }
        }
    }

    close(fd);
    sem_post(&course_sem);

    if (!found_course) {
        strcpy(buffer, "You are not enrolled in any courses.\n");
        send_message(client_socket, buffer);
        fflush(stdout);
    }
}