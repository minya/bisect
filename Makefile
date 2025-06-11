CC = gcc
CFLAGS = -Wall -Wextra -Werror -O0 -std=c99
TARGET = bisect
TEST_TARGET = test_bisect
MAIN_SOURCES = main.c bisect_lib.c
LIB_SOURCES = bisect_lib.c
TEST_SOURCES = test.c
MAIN_OBJECTS = $(MAIN_SOURCES:.c=.o)
LIB_OBJECTS = $(LIB_SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

.PHONY: all clean install test

all: $(TARGET)

$(TARGET): $(MAIN_OBJECTS)
	$(CC) $(MAIN_OBJECTS) -o $(TARGET)

$(TEST_TARGET): $(TEST_OBJECTS) $(LIB_OBJECTS)
	$(CC) $(TEST_OBJECTS) $(LIB_OBJECTS) -o $(TEST_TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(MAIN_OBJECTS) $(LIB_OBJECTS) $(TEST_OBJECTS) $(TARGET) $(TEST_TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

.DEFAULT_GOAL := all