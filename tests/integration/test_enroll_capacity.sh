#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT_DIR"

rm -f students.dat faculty.dat courses.dat temp_*.dat
touch students.dat faculty.dat courses.dat

./server >tests/integration/server_capacity.log 2>&1 &
SERVER_PID=$!
sleep 1

cleanup() {
  kill "$SERVER_PID" 2>/dev/null || true
}
trap cleanup EXIT

# 1) Admin creates two students + one faculty
./client >tests/integration/admin_capacity.out 2>&1 <<EOF
1
admin
admin123
1
alice
pw
1
bob
pw2
3
csprof
pwf
9
4
EOF

# 2) Faculty creates a course with only 1 total seat
./client >tests/integration/faculty_capacity.out 2>&1 <<EOF
2
csprof
pwf
2
OS
1
6
4
EOF

# 3) Student alice enrolls
./client >tests/integration/alice_capacity.out 2>&1 <<EOF
3
alice
pw
2
1
6
4
EOF

# 4) Student bob attempts to enroll - should see "No available seats" or equivalent
./client >tests/integration/bob_capacity.out 2>&1 <<EOF
3
bob
pw2
2
1
6
4
EOF

grep -q "Enrolled successfully" tests/integration/alice_capacity.out
grep -qi "No available seats" tests/integration/bob_capacity.out

echo "Integration test 'enroll_capacity' PASSED."
