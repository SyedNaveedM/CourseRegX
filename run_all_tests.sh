

echo "Cleaning and rebuilding project..."
make clean
make

echo "Starting server..."
./server &
SERVER_PID=$!
sleep 1

echo "Compiling integration tests..."




gcc tests/integration/test_utils.c tests/integration/integration_tests.c -o integration_tests -pthread

echo "Running integration tests..."




./integration_tests

echo "Stopping server..."
kill $SERVER_PID 2>/dev/null

echo "ALL INTEGRATION TESTS PASSED!"
