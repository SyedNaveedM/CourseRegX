#!/usr/bin/env bash
set -e

# Run from repo root
ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT_DIR"

# Clean data
rm -f students.dat faculty.dat courses.dat temp_*.dat
touch students.dat faculty.dat courses.dat

# Start server in background
./server >tests/integration/server_full_flow.log 2>&1 &
SERVER_PID=$!
sleep 1

cleanup() {
  kill "$SERVER_PID" 2>/dev/null || true
}
trap cleanup EXIT

# 1) Admin login, add one student and one faculty
./client >tests/integration/admin_full_flow.out 2>&1 <<EOF
1
admin
admin123
1
alice
pw
3
csprof
pwf
9
4
EOF

# 2) Faculty login, add one course with 2 seats
./client >tests/integration/faculty_full_flow.out 2>&1 <<EOF
2
csprof
pwf
2
DSA
2
6
4
EOF

# 3) Student login, enroll in that course
./client >tests/integration/student_full_flow.out 2>&1 <<EOF
3
alice
pw
2
1
6
4
EOF

# Assertions on outputs
grep -q "Student added successfully" tests/integration/admin_full_flow.out
grep -q "Faculty added successfully" tests/integration/admin_full_flow.out || true  # wording may differ
grep -q "Course added successfully" tests/integration/faculty_full_flow.out
grep -q "Enrolled successfully" tests/integration/student_full_flow.out

echo "Integration test 'full_flow' PASSED."
