CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -I/opt/homebrew/opt/openssl/include
LDFLAGS = -L/opt/homebrew/opt/openssl/lib -lssl -lcrypto

SRCS = src/main.c src/server.c src/utils.c src/mime.c src/request_handler.c src/ssl_utils.c
OBJS = $(SRCS:.c=.o)
TARGET = server

TEST_SRCS = tests/test_main.c src/unity/unity.c src/mime.c src/utils.c src/request_handler.c src/ssl_utils.c
TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_TARGET = tests/test_runner

.PHONY: all clean test

all: $(TARGET) test

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_OBJS) $(TEST_TARGET) server.log