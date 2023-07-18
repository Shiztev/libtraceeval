# Makefile for libtraceeval histograms
# Author: Stevie Alvarez

#
# Definitions
#
MD := mkdir -p
RM := rm -f

IDIR := include
BDIR := build
SDIR := src
TDIR := utests
EDIR := bin

CC := gcc
CFLAGS ?= -g -Wall -Wextra -pedantic -I$(IDIR)

#
# Dependencies
#
DIRS := $(patsubst $(SDIR)/%, %, $(wildcard src/*))
DIRS += $(patsubst $(TDIR)/%, %, $(wildcard utests/*))

_DEPS := histograms.h
DEPS := $(patsubst %,$(IDIR)/%,$(_DEPS))

LIB_SRCS := $(wildcard $(SDIR)/*.c)
TEST_SRCS := $(wildcard $(TDIR)/*.c)

OBJS := $(patsubst $(SDIR)/%.c, $(BDIR)/%.o, $(LIB_SRCS))
OBJS += $(patsubst $(TDIR)/%.c, $(BDIR)/%.o, $(TEST_SRCS))

TEST_EXECS := $(patsubst test/%.c, $(EDIR)/%, $(TEST_SRCS))
EXECUTABLES := $(TEST_EXECS)

#
# Targets
#
all: dirs $(EXECUTABLES)

# src objs
$(BDIR)/%.o: $(SDIR)%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# utests objs
$(BDIR)/%.o: $(TDIR)%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EDIR)/%: $(BDIR)/%.o
	$(CC) -o $@ $^ $(CFLAGS)

dirs:
	@$(MD) $(patsubst %, $(BDIR)/%, $(DIRS)) $(patsubst %, $(EDIR)/%, $(DIRS))

#
# Clean up
#
clean:
	@$(RM) $(EXECUTABLES) $(OBJS)

.PHONY: clean dirs
