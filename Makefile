CC = gcc
AR = ar
CFLAGS = -I./include -Wall -Wextra -Wpedantic -g
LDFLAGS = -L$(BUILD_DIR)
LIBS = -lauxfunc -lncurses -ltinfo -lm -lcjson

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

# Target predefinito
all: directories $(LIBAUX) $(BINS)

# Compila la libreria statica
$(LIBAUX): $(AUX_OBJ)
	$(AR) rcs $@ $^

# Regole per compilare gli eseguibili
$(BIN_DIR)/%: $(BUILD_DIR)/%.o $(LIBAUX)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

# Regole per creare file oggetto (compresi file di libreria)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regole per gestire le directory
.PHONY: directories
directories:
	mkdir -p $(BIN_DIR) $(BUILD_DIR) $(LOG_DIR)
	rm -f $(LOG_DIR)/*.txt
	rm -f $(LOG_DIR)/*.log

# Pulisce solo i log
.PHONY: clean-logs
clean-logs:
	rm -f $(LOG_DIR)/*.txt
	rm -f $(LOG_DIR)/*.log

# Unico bersaglio clean
.PHONY: clean
clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR) $(LOG_DIR) # Elimina le directory esistenti