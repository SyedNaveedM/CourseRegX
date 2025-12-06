# # Define the compiler
# CC = gcc

# # Define compiler flags
# # -Wall: Enable all common warnings
# # -Wextra: Enable extra warnings
# # -pthread: Link with the POSIX threads library
# # -g: Include debugging information
# CFLAGS = -Wall -Wextra -pthread -g

# # Define the server source files
# SERVER_SRCS = server.c utils.c admin.c faculty.c student.c

# # Define the client source files
# CLIENT_SRCS = Client.c

# # Define the object files
# SERVER_OBJS = $(SERVER_SRCS:.c=.o)
# CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

# # Define the executables
# SERVER_EXEC = server
# CLIENT_EXEC = client

# # Default target: build both server, client and data files
# all: $(SERVER_EXEC) $(CLIENT_EXEC) data

# # Rule to build the server executable
# $(SERVER_EXEC): $(SERVER_OBJS)
# 	$(CC) $(CFLAGS) $(SERVER_OBJS) -o $(SERVER_EXEC)

# # Rule to build the client executable
# $(CLIENT_EXEC): $(CLIENT_OBJS)
# 	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o $(CLIENT_EXEC)

# # Rule to compile .c files into .o files
# %.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# # Explicit rule for Client.o (still needed due to capitalization)
# Client.o: Client.c
# 	$(CC) $(CFLAGS) -c Client.c -o Client.o

# # Rule to create .dat files
# data:
# 	touch students.dat faculty.dat courses.dat

# # Clean target: remove object files and executables and .dat files
# clean:
# 	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_EXEC) $(CLIENT_EXEC) *.dat temp_*.dat

# # Phony targets
# .PHONY: all clean data


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
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_EXEC) $(CLIENT_EXEC) *.dat temp_*.dat tests/test_utils_auth tests/test_utils_recv


.PHONY: all clean data test unit-test integration-test


TEST_CFLAGS = -Wall -Wextra -g -DTESTING



tests/test_utils_auth: tests/test_utils_auth.c utils.c common.h utils.h
	$(CC) $(TEST_CFLAGS) -o $@ tests/test_utils_auth.c utils.c -lpthread

tests/test_utils_recv: tests/test_utils_recv.c utils.c common.h utils.h
	$(CC) $(TEST_CFLAGS) -o $@ tests/test_utils_recv.c utils.c


unit-test: tests/test_utils_auth tests/test_utils_recv
	@cd tests && ./reset_test_data.sh && ./test_utils_auth
	@cd tests && ./reset_test_data.sh && ./test_utils_recv



integration-test:
	@echo "Running integration tests..."
	@bash tests/integration/test_full_flow.sh
	@bash tests/integration/test_enroll_capacity.sh


test: unit-test integration-test
	@echo "All tests (unit + integration) completed."
