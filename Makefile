# Compiler and Librarian
CC = gcc
AR = ar

# Compilation flags
CFLAGS = -I./include -I/usr/include/fastdds -I/usr/include/fastcdr -Wall -Wextra -Wpedantic -g
LDFLAGS = -L$(BUILD_DIR)
LIBS = -lauxfunc -lncurses -ltinfo -lm -lcjson -lfastcdr -lfastdds

# Directory
SRC_DIR = src
BIN_DIR = bin
BUILD_DIR = build
LOG_DIR = log

# File sorgente e oggetto
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
EXECUTABLES = $(patsubst $(SRC_DIR)/%.c,%,$(filter-out $(SRC_DIR)/auxfunc.c,$(SRC_FILES)))
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(filter-out $(SRC_DIR)/auxfunc.c,$(SRC_FILES)))
AUX_OBJ = $(BUILD_DIR)/auxfunc.o
LIBAUX = $(BUILD_DIR)/libauxfunc.a
BINS = $(addprefix $(BIN_DIR)/,$(EXECUTABLES))

# Predefined targets
all: directories $(LIBAUX) $(BINS)

# Compile static library
$(LIBAUX): $(AUX_OBJ)
	$(AR) rcs $@ $^

# Compilation rules for executables
$(BIN_DIR)/%: $(BUILD_DIR)/%.o $(LIBAUX)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

# Creation rules for object files (included library's files)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Directory management rules
.PHONY: directories
directories:
	mkdir -p $(BIN_DIR) $(BUILD_DIR) $(LOG_DIR)
	rm -f $(LOG_DIR)/*.txt
	rm -f $(LOG_DIR)/*.log

# Log cleaning
.PHONY: clean-logs
clean-logs:
	rm -f $(LOG_DIR)/*.txt
	rm -f $(LOG_DIR)/*.log

# Clean
.PHONY: clean
clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR) $(LOG_DIR) # Elimina le directory esistenti