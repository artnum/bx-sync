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
TEST_CHAIN=build/test_sync_chain
TEST_LIBS=`pkg-config --libs jansson libxxhash` -lpthread
TEST_OBJ=$(filter-out build/main.o,$(OBJFILES))

.PHONY: test
test: $(TEST_BIN) $(TEST_CHAIN) build/test_oauth_decide
	./$(TEST_BIN)
	./$(TEST_CHAIN)
	./build/test_oauth_decide

$(TEST_BIN): tests/test_invoice_decode.c src/bxobjects/invoice.c src/bx_object.c \
		src/include/bxobjects/invoice.h
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ tests/test_invoice_decode.c src/bxobjects/invoice.c \
		src/bx_object.c $(TEST_LIBS)

$(TEST_CHAIN): tests/test_sync_chain.c src/bx_chain.c src/bx_walk.c \
		src/include/bx_chain.h src/include/bx_walk.h
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ tests/test_sync_chain.c src/bx_chain.c src/bx_walk.c \
		$(TEST_LIBS)

build/test_oauth_decide: test/oauth_decide.c $(TEST_OBJ)
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc test/oauth_decide.c $(TEST_OBJ) -o $@ $(LIBS)

clean:
	$(RM) $(wildcard $(OBJFILES) $(NAME) $(TEST_BIN) $(TEST_CHAIN) \
		build/test_oauth_decide)
