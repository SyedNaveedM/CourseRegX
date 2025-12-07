// tests/test_common.h
#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 5050
#define SERVER_IP "127.0.0.1"

#define BUF 2048

// Connect to server and return socket
int connect_to_server();

// Send command + read full server response
void send_cmd(int sock, const char *cmd, char *response);

// Assert helper
void assert_contains(const char *response, const char *expected, const char *msg);

#endif