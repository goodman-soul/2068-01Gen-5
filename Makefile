CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -O2 -g -Iinclude
LDFLAGS :=

SRCDIR  := src
BUILDDIR := build
TARGET  := $(BUILDDIR)/sensor_demo

SRCS := $(SRCDIR)/state_machine.c \
        $(SRCDIR)/sensor_driver.c \
        $(SRCDIR)/main.c

OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILDDIR)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILDDIR)
