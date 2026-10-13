NAME=bxsync
CFLAGS=`pkg-config --cflags libcurl jansson libxxhash mariadb liboauth2 libcrypto` -Wall -ggdb
LIBS=`pkg-config --libs libcurl jansson libxxhash mariadb liboauth2 libcrypto` -lpthread -ggdb -lrt
SRCFILES=$(wildcard src/*.c src/*/*.c)
OBJFILES=$(addprefix build/, $(addsuffix .o,$(basename $(notdir $(SRCFILES)))))
CC=gcc
DB=gdb
VG=valgrind --leak-check=full
RM=rm

all: $(NAME)

x:
	@echo $(OBJFILES)

vg: clean all
	$(VG) ./$(NAME) conf.json

dbg: clean all
	$(DB) -ex "r conf.json" $(NAME)

run: clean all
	./$(NAME) conf.json

$(NAME): $(OBJFILES)
	$(CC) $^ -o $(NAME) $(LIBS)

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/*/%.c
	$(CC) $(CFLAGS) -c $< -o $@


TEST_BIN=build/test_invoice_decode
TEST_LIBS=`pkg-config --libs jansson libxxhash` -lpthread

.PHONY: test
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): tests/test_invoice_decode.c src/bxobjects/invoice.c src/bx_object.c \
		src/include/bxobjects/invoice.h
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ tests/test_invoice_decode.c src/bxobjects/invoice.c \
		src/bx_object.c $(TEST_LIBS)

clean:
	$(RM) $(wildcard $(OBJFILES) $(NAME) $(TEST_BIN))
