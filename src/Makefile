# Makefile - PL/0编译器 (Linux/Mac/GCC/Clang)
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I.
LDFLAGS = -lm

SRC = main.c lexical_analyzer.c syntax_analyzer.c \
      semantic_analyzer.c symbol_table.c four_address_code.c \
      nfa_dfa.c lr_parser.c

OBJ = $(SRC:.c=.o)
TARGET = pl0_compiler

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)
	@echo ""
	@echo "✓ 编译成功！"
	@echo "使用: ./$(TARGET) source.pl0"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
