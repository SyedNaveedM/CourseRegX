#ifndef UNIT_TEST_H
#define UNIT_TEST_H

#include <stdio.h>

/* ANSI color codes */
#define GREEN "\033[1;32m"
#define RED "\033[1;31m"
#define RESET "\033[0m"

static int tests_run = 0;    // number of test *cases* run
static int tests_failed = 0; // number of failed assertions in total
static int tests_passed = 0; // number of test *cases* that fully passed

/* Assertion macros (per-assert) */

#define ASSERT_TRUE(cond, msg)                                     \
    do                                                             \
    {                                                              \
        if (!(cond))                                               \
        {                                                          \
            tests_failed++;                                        \
            fprintf(stderr, RED "[ASSERT FAIL] %s:%d: %s\n" RESET, \
                    __FILE__, __LINE__, msg);                      \
        }                                                          \
    } while (0)

#define ASSERT_EQ_INT(expected, actual, msg) \
    ASSERT_TRUE((expected) == (actual), msg)

/*
 * RUN_TEST
 * - desc: description string for the test case
 * - fn:   test function to call
 *
 * We snapshot tests_failed before and after fn().
 * If no new failures, the test case PASSED.
 */
#define RUN_TEST(desc, fn)                          \
    do                                              \
    {                                               \
        int before = tests_failed;                  \
        fn();                                       \
        if (tests_failed == before)                 \
        {                                           \
            printf(GREEN "PASS: %s\n" RESET, desc); \
            tests_passed++;                         \
        }                                           \
        else                                        \
        {                                           \
            printf(RED "FAIL: %s\n" RESET, desc);   \
        }                                           \
        tests_run++;                                \
    } while (0)

/* Final summary */

#define TEST_REPORT()                                                \
    do                                                               \
    {                                                                \
        printf("\n===== TEST SUMMARY =====\n");                      \
        printf(GREEN "PASSED : %d\n" RESET, tests_passed);           \
        printf(RED "FAILED : %d\n" RESET, tests_run - tests_passed); \
    } while (0)

#endif
