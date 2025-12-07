// tests/test_utils.c
#include "test_common.h"

int connect_to_server() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); exit(1); }

    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serv.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("connect");
        exit(1);
    }
    return sock;
}

// int connect_to_server() {
//     printf("[DEBUG] Creating client socket...\n");

//     int sock = socket(AF_INET, SOCK_STREAM, 0);
//     if (sock < 0) {
//         perror("[ERROR] socket failed");
//         exit(1);
//     }

//     struct sockaddr_in serv;
//     serv.sin_family = AF_INET;
//     serv.sin_port = htons(SERVER_PORT);
//     inet_pton(AF_INET, SERVER_IP, &serv.sin_addr);

//     printf("[DEBUG] Attempting connect() to %s:%d...\n",
//            SERVER_IP, SERVER_PORT);

//     if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
//         perror("[ERROR] connect() failed");
//         exit(1);
//     }

//     // Get the actual client-side port
//     struct sockaddr_in local;
//     socklen_t len = sizeof(local);
//     getsockname(sock, (struct sockaddr*)&local, &len);

//     printf("[DEBUG] Connected successfully!\n");
//     printf("[DEBUG] Local client port = %d\n", ntohs(local.sin_port));
//     printf("[DEBUG] Socket FD = %d\n", sock);

//     return sock;
// }


void send_cmd(int sock, const char *cmd, char *response) {
    send(sock, cmd, strlen(cmd), 0);
    usleep(100000); // give server time
    recv(sock, response, BUF, 0);
}


void assert_contains(const char *response, const char *expected, const char *msg) {
    if (!strstr(response, expected)) {
        printf("❌ TEST FAILED: %s\nExpected substring: '%s'\nGot:\n%s\n",
               msg, expected, response);
        exit(1);
    }
    printf("✔ %s\n", msg);
}
