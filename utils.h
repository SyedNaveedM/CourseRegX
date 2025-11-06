#ifndef UTILS_H
#define UTILS_H

#include "common.h"
#include <sys/socket.h> // Required for send/recv prototypes

// Function prototypes for utility functions
int authenticate(int client_socket, int role);
void change_password(int client_socket, int id, int role);
void exit_client(int client_socket);
void exit_client_logged_in(int client_socket); // Added prototype
void send_message_expect(int socket, const char *message);
void send_message(int socket, const char *message);
int recv_message(int socket, char* buffer);

#endif 