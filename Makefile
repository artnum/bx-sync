NAME=bxnet
CLFAGS=`pkg-config --cflags libcurl jansson libxxhash mariadb ncurses` -Wall -fanalyzer -ggdb
LIBS=`pkg-config --libs libcurl jansson libxxhash mariadb ncurses` -lpthread -ggdb
SRCFILES=$(wildcard src/*.c src/*/*.c)
OBJFILES=$(addprefix build/, $(addsuffix .o,$(basename $(notdir $(SRCFILES)))))
CC=gcc
VG=valgrind --leak-check=full
RM=rm

all: $(NAME)

x:
	@echo $(OBJFILES)

vg: clean all
	$(VG) ./bxnet conf.json

run: clean all
	./bxnet conf.json

$(NAME): $(OBJFILES)
	$(CC) $^ -o $(NAME) $(LIBS)

build/%.o: src/%.c
	$(CC) $(CLFAGS) -c $< -o $@

build/%.o: src/*/%.c
	$(CC) $(CLFAGS) -c $< -o $@


clean:
	$(RM) $(wildcard $(OBJFILES) $(NAME))
