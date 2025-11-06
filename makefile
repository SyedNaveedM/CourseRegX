# Define the compiler
CC = gcc

# Define compiler flags
# -Wall: Enable all common warnings
# -Wextra: Enable extra warnings
# -pthread: Link with the POSIX threads library
# -g: Include debugging information
CFLAGS = -Wall -Wextra -pthread -g

# Define the server source files
SERVER_SRCS = server.c utils.c admin.c faculty.c student.c

# Define the client source files
CLIENT_SRCS = Client.c

# Define the object files
SERVER_OBJS = $(SERVER_SRCS:.c=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

# Define the executables
SERVER_EXEC = server
CLIENT_EXEC = client

# Default target: build both server, client and data files
all: $(SERVER_EXEC) $(CLIENT_EXEC) data

# Rule to build the server executable
$(SERVER_EXEC): $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o $(SERVER_EXEC)

# Rule to build the client executable
$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o $(CLIENT_EXEC)

# Rule to compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Explicit rule for Client.o (still needed due to capitalization)
Client.o: Client.c
	$(CC) $(CFLAGS) -c Client.c -o Client.o

# Rule to create .dat files
data:
	touch students.dat faculty.dat courses.dat

# Clean target: remove object files and executables and .dat files
clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_EXEC) $(CLIENT_EXEC) *.dat temp_*.dat

# Phony targets
.PHONY: all clean data
