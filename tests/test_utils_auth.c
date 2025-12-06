#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>

#include "../common.h"
#include "../utils.h"
#include "unit_test.h"

// Define the extern semaphores from common.h
sem_t student_sem, faculty_sem, course_sem;

/* ---------- Small helpers for test data ---------- */

static void reset_files()
{
    int fd;

    fd = open("students.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0)
        close(fd);

    fd = open("faculty.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0)
        close(fd);
}

static void write_one_student(int id, const char *name, const char *pw, int active)
{
    int fd = open("students.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    Student s;
    s.id = id;
    strncpy(s.name, name, sizeof(s.name));
    s.name[sizeof(s.name) - 1] = '\0';
    strncpy(s.password, pw, sizeof(s.password));
    s.password[sizeof(s.password) - 1] = '\0';
    s.active = active;
    write(fd, &s, sizeof(Student));
    close(fd);
}

static void write_one_faculty(int id, const char *name, const char *pw)
{
    int fd = open("faculty.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    Faculty f;
    f.id = id;
    strncpy(f.name, name, sizeof(f.name));
    f.name[sizeof(f.name) - 1] = '\0';
    strncpy(f.password, pw, sizeof(f.password));
    f.password[sizeof(f.password) - 1] = '\0';
    write(fd, &f, sizeof(Faculty));
    close(fd);
}

/* ---------- Fake client helpers (talking to utils.c over socket) ---------- */

typedef struct
{
    int fd; // client-side socket
    const char *user;
    const char *pass;
    char last_msg[BUFFER_SIZE]; // last message from server
} auth_client_ctx;

static void recv_until_marker(int fd, char *buffer, size_t bufsize, const char *marker)
{
    size_t filled = 0;
    buffer[0] = '\0';

    while (1)
    {
        if (filled + 1 >= bufsize)
            break;
        ssize_t n = recv(fd, buffer + filled, bufsize - 1 - filled, 0);
        if (n <= 0)
            break;
        filled += (size_t)n;
        buffer[filled] = '\0';
        if (strstr(buffer, marker) != NULL)
            break;
    }

    char *p = strstr(buffer, marker);
    if (p)
        *p = '\0';
}

static void *auth_client_thread(void *arg)
{
    auth_client_ctx *ctx = (auth_client_ctx *)arg;
    char buf[BUFFER_SIZE * 2];

    // 1) username prompt (EXPECT)
    recv_until_marker(ctx->fd, buf, sizeof(buf), "EXPECT");

    // 2) send username + END
    char out[BUFFER_SIZE];
    snprintf(out, sizeof(out), "%sEND", ctx->user);
    send(ctx->fd, out, strlen(out), 0);

    // 3) password prompt (EXPECT)
    recv_until_marker(ctx->fd, buf, sizeof(buf), "EXPECT");

    // 4) send password + END
    snprintf(out, sizeof(out), "%sEND", ctx->pass);
    send(ctx->fd, out, strlen(out), 0);

    // 5) final message (END)
    recv_until_marker(ctx->fd, ctx->last_msg, sizeof(ctx->last_msg), "END");

    return NULL;
}

/* ---------- Test cases ---------- */

static void test_auth_admin_success()
{
    reset_files();

    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0,
                "socketpair should succeed");

    auth_client_ctx ctx = {
        .fd = sv[1],
        .user = "admin",
        .pass = "admin123"};
    ctx.last_msg[0] = '\0';

    pthread_t tid;
    pthread_create(&tid, NULL, auth_client_thread, &ctx);

    int id = authenticate(sv[0], 1); // role = 1 (admin)

    pthread_join(tid, NULL);
    close(sv[0]);
    close(sv[1]);

    ASSERT_EQ_INT(1, id, "Admin auth should return ID 1");
    ASSERT_TRUE(strstr(ctx.last_msg, "Authentication") != NULL,
                "Admin auth message should indicate success");
}

static void test_auth_admin_fail()
{
    reset_files();

    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0,
                "socketpair should succeed");

    auth_client_ctx ctx = {
        .fd = sv[1],
        .user = "wrong",
        .pass = "creds"};
    ctx.last_msg[0] = '\0';

    pthread_t tid;
    pthread_create(&tid, NULL, auth_client_thread, &ctx);

    int id = authenticate(sv[0], 1); // admin role

    pthread_join(tid, NULL);
    close(sv[0]);
    close(sv[1]);

    ASSERT_EQ_INT(-1, id, "Invalid admin creds should return -1");
    ASSERT_TRUE(strstr(ctx.last_msg, "Authentication") != NULL,
                "Admin auth message should indicate failure");
}

static void test_auth_student_inactive()
{
    reset_files();
    // student id 1, inactive
    write_one_student(1, "alice", "pw", 0);

    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0,
                "socketpair should succeed");

    auth_client_ctx ctx = {
        .fd = sv[1],
        .user = "alice",
        .pass = "pw"};
    ctx.last_msg[0] = '\0';

    pthread_t tid;
    pthread_create(&tid, NULL, auth_client_thread, &ctx);

    int id = authenticate(sv[0], 3); // role = 3 (student)

    pthread_join(tid, NULL);
    close(sv[0]);
    close(sv[1]);

    ASSERT_EQ_INT(-1, id, "Inactive student should not authenticate");
    ASSERT_TRUE(strstr(ctx.last_msg, "deactivated") != NULL ||
                    strstr(ctx.last_msg, "inactive") != NULL,
                "Should notify that account is deactivated");
}

static void test_auth_faculty_success()
{
    reset_files();
    write_one_faculty(42, "csprof", "pwf");

    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0,
                "socketpair should succeed");

    auth_client_ctx ctx = {
        .fd = sv[1],
        .user = "csprof",
        .pass = "pwf"};
    ctx.last_msg[0] = '\0';

    pthread_t tid;
    pthread_create(&tid, NULL, auth_client_thread, &ctx);

    int id = authenticate(sv[0], 2); // role = 2 (faculty)

    pthread_join(tid, NULL);
    close(sv[0]);
    close(sv[1]);

    ASSERT_EQ_INT(42, id, "Faculty auth should return correct ID");
    ASSERT_TRUE(strstr(ctx.last_msg, "Authentication") != NULL,
                "Faculty auth message should indicate success");
}

/* New: active student success */

static void test_auth_student_active_success()
{
    reset_files();
    write_one_student(7, "bob", "secret", 1); // active student

    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0,
                "socketpair should succeed");

    auth_client_ctx ctx = {
        .fd = sv[1],
        .user = "bob",
        .pass = "secret"};
    ctx.last_msg[0] = '\0';

    pthread_t tid;
    pthread_create(&tid, NULL, auth_client_thread, &ctx);

    int id = authenticate(sv[0], 3); // role = 3 (student)

    pthread_join(tid, NULL);
    close(sv[0]);
    close(sv[1]);

    ASSERT_EQ_INT(7, id, "Active student should authenticate and get correct ID");
    ASSERT_TRUE(strstr(ctx.last_msg, "Authentication") != NULL,
                "Student auth message should indicate success");
}

/* New: student wrong password */

static void test_auth_student_wrong_password()
{
    reset_files();
    write_one_student(3, "charlie", "rightpw", 1); // active

    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0,
                "socketpair should succeed");

    auth_client_ctx ctx = {
        .fd = sv[1],
        .user = "charlie",
        .pass = "wrongpw"};
    ctx.last_msg[0] = '\0';

    pthread_t tid;
    pthread_create(&tid, NULL, auth_client_thread, &ctx);

    int id = authenticate(sv[0], 3); // role = 3 (student)

    pthread_join(tid, NULL);
    close(sv[0]);
    close(sv[1]);

    ASSERT_EQ_INT(-1, id, "Wrong password should fail for student");
    ASSERT_TRUE(strstr(ctx.last_msg, "Authentication") != NULL,
                "Message should indicate authentication failure");
}

/* New: faculty wrong password */

static void test_auth_faculty_wrong_password()
{
    reset_files();
    write_one_faculty(10, "osprof", "correctpw");

    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0,
                "socketpair should succeed");

    auth_client_ctx ctx = {
        .fd = sv[1],
        .user = "osprof",
        .pass = "wrongpw"};
    ctx.last_msg[0] = '\0';

    pthread_t tid;
    pthread_create(&tid, NULL, auth_client_thread, &ctx);

    int id = authenticate(sv[0], 2); // role = 2 (faculty)

    pthread_join(tid, NULL);
    close(sv[0]);
    close(sv[1]);

    ASSERT_EQ_INT(-1, id, "Wrong password should fail for faculty");
    ASSERT_TRUE(strstr(ctx.last_msg, "Authentication") != NULL,
                "Message should indicate authentication failure");
}

/* New: invalid role is rejected immediately */

static void test_auth_invalid_role_rejected()
{
    reset_files();

    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0,
                "socketpair should succeed");

    // No client thread: authenticate(., invalid_role) should send an error and return -1
    int id = authenticate(sv[0], 99); // invalid role

    char buf[BUFFER_SIZE] = {0};
    ssize_t n = recv(sv[1], buf, sizeof(buf) - 1, 0);
    if (n > 0)
        buf[n] = '\0';

    close(sv[0]);
    close(sv[1]);

    ASSERT_EQ_INT(-1, id, "Invalid role should be rejected with -1");
    ASSERT_TRUE(strstr(buf, "Invalid") != NULL ||
                    strstr(buf, "login") != NULL,
                "Should send some invalid-role message to client");
}

/* New: faculty username not found */

static void test_auth_faculty_unknown_user()
{
    reset_files();
    // Note: do NOT create any faculty record matching "ghost"
    write_one_faculty(20, "someone_else", "pw123");

    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0,
                "socketpair should succeed");

    auth_client_ctx ctx = {
        .fd = sv[1],
        .user = "ghost",      // this username does NOT exist
        .pass = "pw123"};
    ctx.last_msg[0] = '\0';

    pthread_t tid;
    pthread_create(&tid, NULL, auth_client_thread, &ctx);

    int id = authenticate(sv[0], 2); // role = 2 (faculty)

    pthread_join(tid, NULL);
    close(sv[0]);
    close(sv[1]);

    ASSERT_EQ_INT(-1, id, "Unknown faculty username should fail authentication");
    ASSERT_TRUE(strstr(ctx.last_msg, "Authentication failed") != NULL ||
                    strstr(ctx.last_msg, "Incorrect") != NULL,
                "Should send an authentication failure message to client");
}


/* New: empty username should cause authentication failure */

static void test_auth_empty_username()
{
    reset_files();
    write_one_student(10, "realname", "pw123", 1); // real student exists

    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0,
                "socketpair should succeed");

    // Fake client context: empty username, valid password
    auth_client_ctx ctx = {
        .fd = sv[1],
        .user = "",          // EMPTY username
        .pass = "pw123"
    };
    ctx.last_msg[0] = '\0';

    pthread_t tid;
    pthread_create(&tid, NULL, auth_client_thread, &ctx);

    int id = authenticate(sv[0], 3); // student role

    pthread_join(tid, NULL);
    close(sv[0]);
    close(sv[1]);

    ASSERT_EQ_INT(-1, id, "Empty username should fail authentication");
    ASSERT_TRUE(strstr(ctx.last_msg, "Authentication") != NULL,
                "Should send authentication failure message to client");
}


int main()
{
    // Initialize semaphores used by utils.c
    sem_init(&student_sem, 0, 1);
    sem_init(&faculty_sem, 0, 1);
    // sem_init(&course_sem, 0, 1);

    RUN_TEST("Admin login with correct credentials should succeed",
             test_auth_admin_success);
    RUN_TEST("Admin login with wrong credentials should fail",
             test_auth_admin_fail);
    RUN_TEST("Inactive student should be prevented from logging in",
             test_auth_student_inactive);
    RUN_TEST("Faculty login with correct credentials should succeed",
             test_auth_faculty_success);
    RUN_TEST("Active student with correct credentials should login successfully",
             test_auth_student_active_success);
    RUN_TEST("Student with wrong password should fail authentication",
             test_auth_student_wrong_password);
    RUN_TEST("Faculty with wrong password should fail authentication",
             test_auth_faculty_wrong_password);
    RUN_TEST("Invalid login role should be rejected before prompts",
             test_auth_invalid_role_rejected);
    RUN_TEST("Faculty with unknown username should fail authentication",
    test_auth_faculty_unknown_user);
    RUN_TEST("Empty username should cause login failure",
         test_auth_empty_username);


    TEST_REPORT();

    sem_destroy(&student_sem);
    sem_destroy(&faculty_sem);
    sem_destroy(&course_sem);

    // Non-zero exit if any test case failed
    return (tests_run - tests_passed) ? 1 : 0;
}
