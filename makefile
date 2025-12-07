CC = gcc

CFLAGS = -Wall -Wextra -pthread -g

SERVER_SRCS = server.c utils.c admin.c faculty.c student.c
CLIENT_SRCS = Client.c

SERVER_OBJS = $(SERVER_SRCS:.c=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

SERVER_EXEC = server
CLIENT_EXEC = client

all: $(SERVER_EXEC) $(CLIENT_EXEC) data

$(SERVER_EXEC): $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o $(SERVER_EXEC)

$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o $(CLIENT_EXEC)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

Client.o: Client.c
	$(CC) $(CFLAGS) -c Client.c -o Client.o

data:
	touch students.dat faculty.dat courses.dat

clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_EXEC) $(CLIENT_EXEC) \
	      *.dat temp_*.dat \
	      tests/test_utils_auth tests/test_utils_recv \
	      integration_tests \
	      tests/*.dat tests/temp_*.dat

.PHONY: all clean data test unit-test integration-test

TEST_CFLAGS = -Wall -Wextra -g -DTESTING


tests/test_utils_auth: tests/unit/test_utils_auth.c utils.c common.h utils.h
	$(CC) $(TEST_CFLAGS) -o $@ tests/unit/test_utils_auth.c utils.c -lpthread

tests/test_utils_recv: tests/unit/test_utils_recv.c utils.c common.h utils.h
	$(CC) $(TEST_CFLAGS) -o $@ tests/unit/test_utils_recv.c utils.c

unit-test: tests/test_utils_auth tests/test_utils_recv
	@cd tests/unit && ./reset_test_data.sh
	@cd tests && ./test_utils_auth
	@cd tests/unit && ./reset_test_data.sh
	@cd tests && ./test_utils_recv


integration-test:
	@echo "Compiling integration tests..."
	$(CC) $(CFLAGS) tests/integration/test_utils.c tests/integration/integration_tests.c -o integration_tests
	@echo "Running integration tests via run_all_tests.sh..."
	@bash run_all_tests.sh




test: unit-test integration-test
	@echo "All tests (unit + integration) completed."
