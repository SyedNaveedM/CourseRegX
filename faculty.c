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

// Function prototypes for faculty functions
void faculty_menu(int client_socket, int faculty_id);
void view_offering_courses(int client_socket, int faculty_id);
void add_course(int client_socket, int faculty_id);
void remove_course(int client_socket, int faculty_id);
void update_course_details(int client_socket, int faculty_id);


// Faculty menu function
void faculty_menu(int client_socket, int faculty_id) {
    char buffer[BUFFER_SIZE];
    int choice;

    while (1) {
        strcpy(buffer, "\n........ Welcome to Faculty Menu ........\n1. View Offering Courses\n2. Add New Course\n3. Remove Course from Catalog\n4. Update Course Details\n5. Change Password\n6. Logout and Exit\nEnter your choice: ");
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
                view_offering_courses(client_socket, faculty_id);
                break;
            case 2:
                add_course(client_socket, faculty_id);
                break;
            case 3:
                remove_course(client_socket, faculty_id);
                break;
            case 4:
                update_course_details(client_socket, faculty_id);
                break;
            case 5:
                change_password(client_socket, faculty_id, 2); // 2 for faculty
                break;
            case 6:
                exit_client_logged_in(client_socket);
                return; // Exit the faculty menu loop
            default:
                strcpy(buffer, "Invalid choice. Please try again.\n");
                send_message(client_socket, buffer);
                fflush(stdout);
                break; // Add break statement
        }
    }
}

// View offering courses function
void view_offering_courses(int client_socket, int faculty_id) {
    char buffer[BUFFER_SIZE];
    Course course;
    int fd, found = 0;

    strcpy(buffer, "Your offered courses:\n");
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

    while (read(fd, &course, sizeof(Course)) > 0) {
        if (course.faculty_id == faculty_id) {
            sprintf(buffer, "ID: %d, Name: %s, Enrolled: %d/%d\n",
                    course.id, course.name, course.enrollment_count, course.total_seats);
            send_message(client_socket, buffer);
            fflush(stdout);
            found = 1;
        }
    }

    close(fd);
    sem_post(&course_sem);

    if (!found) {
        strcpy(buffer, "You haven't offered any courses yet.\n");
        send_message(client_socket, buffer);
        fflush(stdout);
    }
}

// Add course function
void add_course(int client_socket, int faculty_id) {
    char buffer[BUFFER_SIZE];
    Course course;
    int fd, last_id = 0;

    strcpy(buffer, "Enter course name: ");
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
    strcpy(course.name, buffer);

    strcpy(buffer, "Enter total seats: ");
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
    course.total_seats = atoi(buffer);

    // do not add the course if no of seats is less than or 0
    if (course.total_seats <= 0) {
        strcpy(buffer, "Error: Total seats must be a positive number.\n");
        send_message(client_socket, buffer);
        fflush(stdout);
        return; 
    }

    course.available_seats = course.total_seats;

    course.faculty_id = faculty_id;
    course.enrollment_count = 0;
    for (int i = 0; i < MAX_SEATS; i++) {
        course.enrolled_students[i] = 0;
    }

    sem_wait(&course_sem);

    fd = open("courses.dat", O_RDONLY);
    if (fd != -1) {
        Course temp;
        while (read(fd, &temp, sizeof(Course)) > 0) {
            last_id = temp.id;
        }
        close(fd);
    }

    course.id = last_id + 1;

    fd = open("courses.dat", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        strcpy(buffer, "Error: Could not open course database.\n");
        send_message(client_socket, buffer);
        fflush(stdout);
        sem_post(&course_sem);
        return;
    }

    write(fd, &course, sizeof(Course));
    close(fd);
    sem_post(&course_sem);

    sprintf(buffer, "Course added successfully! ID: %d\n", course.id);
    send_message(client_socket, buffer);
    fflush(stdout);
}

// Remove course function
void remove_course(int client_socket, int faculty_id) {
    char buffer[BUFFER_SIZE];
    Course course;
    int course_id, fd, found = 0;

    view_offering_courses(client_socket, faculty_id);

    strcpy(buffer, "Enter course ID to remove (0 to cancel): ");
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
        strcpy(buffer, "Operation canceled.\n");
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

    found = 0;
    int authorized = 0;
    int removed_successfully = 0;

    while (read(fd, &course, sizeof(Course)) > 0) {
        if (course.id == course_id) {
            found = 1;

            if (course.faculty_id == faculty_id) {
                authorized = 1;

                if (course.enrollment_count > 0) {
                    strcpy(buffer, "Error: Cannot remove course with enrolled students.\n");
                    send_message(client_socket, buffer);
                    fflush(stdout);
                    write(temp_fd, &course, sizeof(Course));
                } else {
                    strcpy(buffer, "Course removed successfully.\n");
                    send_message(client_socket, buffer);
                    fflush(stdout);
                    removed_successfully = 1;
                }
            } else {
                strcpy(buffer, "Error: You are not authorized to remove this course.\n");
                send_message(client_socket, buffer);
                fflush(stdout);
                write(temp_fd, &course, sizeof(Course));
            }
        } else {
            write(temp_fd, &course, sizeof(Course));
        }
    }

    close(fd);
    close(temp_fd);

    if (found && authorized && removed_successfully) {
        if (rename("temp_courses.dat", "courses.dat") != 0) {
             strcpy(buffer, "Error: Failed to update course file after removal.\n");
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

// Update course details function
void update_course_details(int client_socket, int faculty_id) {
    char buffer[BUFFER_SIZE];
    Course course;
    int course_id, fd, found = 0;

    view_offering_courses(client_socket, faculty_id);

    strcpy(buffer, "Enter course ID to update (0 to cancel): ");
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
        strcpy(buffer, "Operation canceled.\n");
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

    found = 0;
    int authorized = 0;
    int updated_successfully = 0;

    while (read(fd, &course, sizeof(Course)) > 0) {
        if (course.id == course_id) {
            found = 1;

            if (course.faculty_id == faculty_id) {
                authorized = 1;

                sprintf(buffer, "Current course details:\nName: %s\nTotal Seats: %d\nAvailable Seats: %d\n",
                        course.name, course.total_seats, course.available_seats);
                send_message(client_socket, buffer);
                fflush(stdout);

                strcpy(buffer, "Enter new course name (or press Enter to keep current): ");
                send_message_expect(client_socket, buffer);
                fflush(stdout);
                memset(buffer, 0, BUFFER_SIZE);
                {int check = recv_message(client_socket, buffer);
                    if(check < 0){
                        send_message(client_socket, "Unknown Error Occurred\n");
                        // Do not return here, still need to write the current course details to temp_fd
                    } else {
                         buffer[strcspn(buffer, "\n")] = 0; // Remove newline
                         if (strlen(buffer) > 0) {
                            strcpy(course.name, buffer);
                        }
                    }
                }


                strcpy(buffer, "Enter new total seats (or 0 to keep current): ");
                send_message_expect(client_socket, buffer);
                fflush(stdout);
                memset(buffer, 0, BUFFER_SIZE);
                {int check = recv_message(client_socket, buffer);
                    if(check < 0){
                        send_message(client_socket, "Unknown Error Occurred\n");
                         // Do not return here, still need to write the current course details to temp_fd
                    } else {
                        buffer[strcspn(buffer, "\n")] = 0; // Remove newline
                        int new_total_seats = atoi(buffer);
                        if (new_total_seats > 0) {
                            if (new_total_seats < course.enrollment_count) {
                                strcpy(buffer, "Error: Cannot set total seats less than current enrollment.\n");
                                send_message(client_socket, buffer);
                                fflush(stdout);
                            } else {
                                int seats_difference = new_total_seats - course.total_seats;
                                course.total_seats = new_total_seats;
                                course.available_seats += seats_difference;
                                updated_successfully = 1;
                                strcpy(buffer, "Course updated successfully.\n");
                                send_message(client_socket, buffer);
                                fflush(stdout);
                            }
                        } else {
                            updated_successfully = 1;
                            strcpy(buffer, "Course updated successfully.\n");
                            send_message(client_socket, buffer);
                            fflush(stdout);
                        }
                    }
                }
            } else {
                strcpy(buffer, "Error: You are not authorized to update this course.\n");
                send_message(client_socket, buffer);
                fflush(stdout);
            }
        }
        write(temp_fd, &course, sizeof(Course));
    }

    close(fd);
    close(temp_fd);

    if (found && authorized && updated_successfully) {
        if (rename("temp_courses.dat", "courses.dat") != 0) {
             strcpy(buffer, "Error: Failed to update course file.\n");
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