#ifndef COMMON_H
#define COMMON_H

#include <semaphore.h>

// Constants
#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define MAX_COURSES 50
#define MAX_SEATS 10

// Structure definitions
typedef struct {
    int id;
    char name[50];
    char password[50];
    int active; // 1 for active, 0 for inactive
} Student;

typedef struct {
    int id;
    char name[50];
    char password[50];
} Faculty;

typedef struct {
    int id;
    char name[100];
    int faculty_id;
    int total_seats;
    int available_seats;
    int enrolled_students[MAX_SEATS];
    int enrollment_count;
} Course;

// Global semaphores (declared as extern, defined in server.c)
extern sem_t student_sem, faculty_sem, course_sem;

// Function prototypes for menu functions (used in server.c)
void admin_menu(int client_socket);
void faculty_menu(int client_socket, int faculty_id);
void student_menu(int client_socket, int student_id);

#endif 