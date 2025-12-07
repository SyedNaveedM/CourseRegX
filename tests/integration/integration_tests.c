#include "test_common.h"





void login_student(int sock, const char *username, char *buffer) {
    send_cmd(sock, "3END", buffer);
    send_cmd(sock, username, buffer);
    send_cmd(sock, "passEND", buffer);
    assert_contains(buffer, "Authentication successful", "Student login works");
}





void run_admin_test() {
    printf("\n=== ADMIN INTEGRATION TESTS ===\n");

    char res[BUF];
    int sock = connect_to_server();

    send_cmd(sock, "1END", res);
    assert_contains(res, "Enter username", "Asked username");

    send_cmd(sock, "adminEND", res);
    assert_contains(res, "Enter password", "Asked password");

    send_cmd(sock, "admin123END", res);
    assert_contains(res, "Authentication successful", "Admin login ok");

    
    send_cmd(sock, "1END", res);
    send_cmd(sock, "JohnEND", res);
    send_cmd(sock, "passEND", res);
    assert_contains(res, "Student added successfully", "Added student");

    
    send_cmd(sock, "3END", res);
    send_cmd(sock, "YashubEND", res);
    send_cmd(sock, "GoelEND", res);
    assert_contains(res, "Faculty added successfully", "Added faculty");

    
    send_cmd(sock, "6END", res);
    send_cmd(sock, "1END", res);
    assert_contains(res, "blocked successfully", "Blocked student");

    
    send_cmd(sock, "5END", res);
    send_cmd(sock, "1END", res);
    assert_contains(res, "activated successfully", "Activated student");

    
    send_cmd(sock, "9END", res);
    assert_contains(res, "Goodbye!", "Admin logout ok");

    printf("=== ADMIN TESTS PASSED ===\n");
    
}





void run_faculty_test() {
    printf("\n=== FACULTY INTEGRATION TESTS ===\n");

    char res[BUF];
    int sock = connect_to_server();

    
    send_cmd(sock, "2END", res);
    send_cmd(sock, "YashubEND", res);
    send_cmd(sock, "GoelEND", res);
    assert_contains(res, "Authentication successful", "Faculty login ok");

    
    send_cmd(sock, "2END", res);
    send_cmd(sock, "IntroToCSEND", res);
    send_cmd(sock, "2END", res);
    assert_contains(res, "Course added successfully", "Add course ok");

    
    send_cmd(sock, "4END", res);
    send_cmd(sock, "1END", res);
    send_cmd(sock, "CS50END", res);
    send_cmd(sock, "100END", res);
    assert_contains(res, "Course updated successfully", "Update ok");

    
    send_cmd(sock, "1END", res);
    assert_contains(res, "Your offered courses", "View offered");

    
    send_cmd(sock, "6END", res);
    assert_contains(res, "Goodbye!", "Faculty logout ok");

    printf("=== FACULTY TESTS PASSED ===\n");
    
}





void run_student_test() {
    printf("\n=== STUDENT INTEGRATION TESTS ===\n");

    char buf[BUF];
    int sock = connect_to_server();

    login_student(sock, "JohnEND", buf);

    send_cmd(sock, "1END", buf);
    assert_contains(buf, "Available courses", "browse ok");

    send_cmd(sock, "2END", buf);
    assert_contains(buf, "Available courses", "view ok");

    send_cmd(sock, "1END", buf);
    assert_contains(buf, "Enrolled successfully!", "enroll ok");

    send_cmd(sock, "4END", buf);
    assert_contains(buf, "CS50", "has enrolled course");

    send_cmd(sock, "3END", buf);
    assert_contains(buf, "CS50", "can drop");

    send_cmd(sock, "1END", buf);
    assert_contains(buf, "Course dropped successfully", "dropped ok");

    printf("=== STUDENT TESTS PASSED ===\n");
    
}





char resA[BUF], resB[BUF];

void run_concurrency_test() {
    printf("\n=== CONCURRENCY TEST (SERIALIZED) ===\n");

    char buf[BUF];

    
    
    
    int admin = connect_to_server();

    send_cmd(admin, "1END", buf);          
    send_cmd(admin, "adminEND", buf);
    send_cmd(admin, "admin123END", buf);
    assert_contains(buf, "Authentication successful", "Admin login");

    
    send_cmd(admin, "1END", buf);
    send_cmd(admin, "AliceEND", buf);
    send_cmd(admin, "passEND", buf);
    assert_contains(buf, "Student added successfully", "Added Alice");

    
    send_cmd(admin, "1END", buf);
    send_cmd(admin, "BobEND", buf);
    send_cmd(admin, "passEND", buf);
    assert_contains(buf, "Student added successfully", "Added Bob");

    
    send_cmd(admin, "9END", buf);
    



    
    
    
    int fac = connect_to_server();

    send_cmd(fac, "2END", buf);            
    send_cmd(fac, "YashubEND", buf);
    send_cmd(fac, "GoelEND", buf);
    assert_contains(buf, "Authentication successful", "Faculty login");

    
    send_cmd(fac, "2END", buf);
    send_cmd(fac, "RaceCourseEND", buf);   
    send_cmd(fac, "1END", buf);            
    assert_contains(buf, "Course added successfully", "Course add OK");

    
    send_cmd(fac, "6END", buf);
    



    
    
    
    int A = connect_to_server();

    send_cmd(A, "3END", buf);              
    send_cmd(A, "AliceEND", buf);
    send_cmd(A, "passEND", buf);
    assert_contains(buf, "Authentication successful", "Alice login");

    send_cmd(A, "2END", buf);              
    send_cmd(A, "2END", buf);              
    int alice_ok = strstr(buf, "Enrolled successfully") != NULL;

    



    
    
    
    int B = connect_to_server();

    send_cmd(B, "3END", buf);
    send_cmd(B, "BobEND", buf);
    send_cmd(B, "passEND", buf);
    assert_contains(buf, "Authentication successful", "Bob login");

    send_cmd(B, "2END", buf);
    send_cmd(B, "2END", buf);
    int bob_ok = strstr(buf, "Enrolled successfully") != NULL;

    



    
    
    
    int success_count = alice_ok + bob_ok;

    if (success_count != 1) {
        printf("❌ FAILED — Exactly one student should enroll, got %d\n", success_count);
        exit(1);
    }

    printf("CONCURRENCY TEST PASSED — only one student enrolled.\n");
}


int main() {
    run_admin_test();
    run_faculty_test();
    run_student_test();
    run_concurrency_test();
    return 0;
}
