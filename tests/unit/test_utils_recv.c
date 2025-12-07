#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <semaphore.h>
#include "../../common.h"
#include "../../utils.h"
#include "unit_test.h"

extern void reset_recv_buffer(void);



// Define extern semaphores (unused here but required by utils.c)
sem_t student_sem, faculty_sem, course_sem;

static void write_raw(int fd, const char *s)
{
    send(fd, s, strlen(s), 0);
}

/* -------------------- TEST CASE 1 -------------------- */
static void test_recv_single_end()
{
    reset_recv_buffer();
    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0,
                "socketpair should succeed");

    write_raw(sv[1], "HelloEND");

    char buf[BUFFER_SIZE];
    int t = recv_message(sv[0], buf);

    ASSERT_EQ_INT(0, t, "Return value should be 0 for END marker");
    ASSERT_TRUE(strcmp(buf, "Hello") == 0,
                "Message content before END should be 'Hello'");

    close(sv[0]);
    close(sv[1]);
}

/* -------------------- TEST CASE 2 -------------------- */
static void test_recv_two_messages_in_one_buffer()
{
    reset_recv_buffer();
    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0,
                "socketpair should succeed");

    write_raw(sv[1], "FirstENDSecondEXPECT");

    char buf[BUFFER_SIZE];
    int t1 = recv_message(sv[0], buf);
    ASSERT_EQ_INT(0, t1, "First marker END should give 0");
    ASSERT_TRUE(strcmp(buf, "First") == 0,
                "First message should be 'First'");

    int t2 = recv_message(sv[0], buf);
    ASSERT_EQ_INT(1, t2, "Second marker EXPECT should give 1");
    ASSERT_TRUE(strcmp(buf, "Second") == 0,
                "Second message should be 'Second'");

    close(sv[0]);
    close(sv[1]);
}

/* -------------------- TEST CASE 3 -------------------- */
static void test_recv_exit_priority()
{
    reset_recv_buffer();
    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0,
                "socketpair should succeed");

    write_raw(sv[1], "ByeEXITignoredEND");

    char buf[BUFFER_SIZE];
    int t = recv_message(sv[0], buf);

    ASSERT_EQ_INT(2, t, "Return value should be 2 for EXIT marker");
    ASSERT_TRUE(strcmp(buf, "Bye") == 0,
                "Message content before EXIT should be 'Bye'");

    close(sv[0]);
    close(sv[1]);
}

/* -------------------- NEW TEST CASE 4 -------------------- */
static void test_recv_expect_marker()
{
    reset_recv_buffer();
    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0, "socketpair");

    write_raw(sv[1], "Enter your nameEXPECT");

    char buf[BUFFER_SIZE];
    int t = recv_message(sv[0], buf);

    ASSERT_EQ_INT(1, t, "EXPECT should return 1");
    ASSERT_TRUE(strcmp(buf, "Enter your name") == 0,
                "EXPECT should strip text correctly");

    close(sv[0]); close(sv[1]);
}

/* -------------------- NEW TEST CASE 5 -------------------- */
static void test_recv_empty_message_before_marker()
{
    reset_recv_buffer();
    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0, "socketpair");

    write_raw(sv[1], "END");

    char buf[BUFFER_SIZE];
    int t = recv_message(sv[0], buf);

    ASSERT_EQ_INT(0, t, "END should return 0");
    ASSERT_TRUE(strcmp(buf, "") == 0, "Message should be empty string");

    close(sv[0]); close(sv[1]);
}

/* -------------------- NEW TEST CASE 6 -------------------- */
static void test_recv_no_marker()
{
    reset_recv_buffer();
    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0, "socketpair");

    char buf[BUFFER_SIZE];
    write_raw(sv[1], "Just some random text without marker");
    shutdown(sv[1], SHUT_WR); // force recv() to return 0

    int t = recv_message(sv[0], buf);

    ASSERT_EQ_INT(-1, t, "Without markers, recv_message should return -1");

    close(sv[0]); close(sv[1]);
}

/* -------------------- NEW TEST CASE 7 -------------------- */
static void test_recv_multiple_expect()
{
    reset_recv_buffer();
    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0, "socketpair");

    write_raw(sv[1], "EnterEXPECTPasswordEXPECT");

    char buf[BUFFER_SIZE];
    
    int t1 = recv_message(sv[0], buf);
    ASSERT_EQ_INT(1, t1, "EXPECT #1 should return 1");
    ASSERT_TRUE(strcmp(buf, "Enter") == 0, "First message is 'Enter'");

    int t2 = recv_message(sv[0], buf);
    ASSERT_EQ_INT(1, t2, "EXPECT #2 should return 1");
    ASSERT_TRUE(strcmp(buf, "Password") == 0, "Second message is 'Password'");

    close(sv[0]); close(sv[1]);
}

/* -------------------- NEW TEST CASE 8 -------------------- */
static void test_recv_end_then_exit()
{
    reset_recv_buffer();
    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0, "socketpair");

    write_raw(sv[1], "OkENDQuitEXIT");

    char buf[BUFFER_SIZE];

    int t1 = recv_message(sv[0], buf);
    ASSERT_EQ_INT(0, t1, "END should take priority over later EXIT");
    ASSERT_TRUE(strcmp(buf, "Ok") == 0, "First message is 'Ok'");

    int t2 = recv_message(sv[0], buf);
    ASSERT_EQ_INT(2, t2, "NEXT marker should be EXIT");
    ASSERT_TRUE(strcmp(buf, "Quit") == 0, "Second message is 'Quit'");

    close(sv[0]); close(sv[1]);
}

/* -------------------- NEW TEST CASE 9 -------------------- */
static void test_recv_large_message_split_over_two_chunks()
{
    reset_recv_buffer();
    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0, "socketpair");

    write_raw(sv[1], "Hello this is part 1 ");
    usleep(1000);
    write_raw(sv[1], "and this is part 2END");

    char buf[BUFFER_SIZE];

    int t = recv_message(sv[0], buf);
    ASSERT_EQ_INT(0, t, "END should return 0");
    ASSERT_TRUE(strcmp(buf, "Hello this is part 1 and this is part 2") == 0,
                "Message should combine chunks correctly");

    close(sv[0]); close(sv[1]);
}

/* -------------------- NEW TEST CASE 10 -------------------- */
static void test_recv_end_then_expect()
{
    reset_recv_buffer();
    int sv[2];
    ASSERT_TRUE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0, "socketpair");

    // END appears before EXPECT → END should fire first
    write_raw(sv[1], "HelloENDMore textEXPECT");

    char buf[BUFFER_SIZE];

    int t1 = recv_message(sv[0], buf);
    ASSERT_EQ_INT(0, t1, "First marker should be END");
    ASSERT_TRUE(strcmp(buf, "Hello") == 0,
                "First message should be 'Hello'");

    int t2 = recv_message(sv[0], buf);
    ASSERT_EQ_INT(1, t2, "Second marker should be EXPECT");
    ASSERT_TRUE(strcmp(buf, "More text") == 0,
                "Second message should be 'More text'");

    close(sv[0]); close(sv[1]);
}



/* ========================= MAIN ========================= */

int main()
{
    sem_init(&student_sem, 0, 1);
    sem_init(&faculty_sem, 0, 1);
    sem_init(&course_sem, 0, 1);

    RUN_TEST("Single END-terminated message", test_recv_single_end);
    RUN_TEST("Two messages in one buffer", test_recv_two_messages_in_one_buffer);
    RUN_TEST("EXIT takes priority over END", test_recv_exit_priority);
    RUN_TEST("EXPECT marker works", test_recv_expect_marker);
    RUN_TEST("Empty message before END", test_recv_empty_message_before_marker);
    RUN_TEST("No marker returns -1", test_recv_no_marker);
    RUN_TEST("Two EXPECT markers in a row", test_recv_multiple_expect);
    RUN_TEST("END followed by EXIT", test_recv_end_then_exit);
    RUN_TEST("Message split across chunks", test_recv_large_message_split_over_two_chunks);
    RUN_TEST("END then EXPECT", test_recv_end_then_expect);


    TEST_REPORT();

    sem_destroy(&student_sem);
    sem_destroy(&faculty_sem);
    sem_destroy(&course_sem);

    return (tests_run - tests_passed) ? 1 : 0;
}
