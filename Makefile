NAME=bxsync
CLFAGS=`pkg-config --cflags libcurl jansson libxxhash mariadb` -Wall -fanalyzer -ggdb
LIBS=`pkg-config --libs libcurl jansson libxxhash mariadb` -lpthread -ggdb
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
	$(CC) $(CLFAGS) -c $< -o $@

build/%.o: src/*/%.c
	$(CC) $(CLFAGS) -c $< -o $@


clean:
	$(RM) $(wildcard $(OBJFILES) $(NAME))
