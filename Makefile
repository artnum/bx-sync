NAME=bxnet
CLFAGS=`pkg-config --cflags libcurl jansson libxxhash` -ggdb -I./src/include/
LIBS=`pkg-config --libs libcurl jansson libxxhash` -lpthread -ggdb
SRCFILES=$(wildcard src/*.c src/*/*.c)
OBJFILES=$(addprefix build/, $(addsuffix .o,$(basename $(notdir $(SRCFILES)))))
CC=gcc
RM=rm

all: $(NAME)

x:
	@echo $(OBJFILES)

$(NAME): $(OBJFILES)
	$(CC) $^ -o $(NAME) $(LIBS)

build/%.o: src/%.c
	$(CC) $(CLFAGS) -c $< -o $@

build/%.o: src/*/%.c
	$(CC) $(CLFAGS) -c $< -o $@


clean:
	$(RM) $(wildcard $(OBJFILES) $(NAME))