CFLAGS=`pkg-config --cflags libcurl jansson libxxhash mariadb` -Wall -fanalyzer -ggdb -DNO_LOG
LIBS=`pkg-config --libs libcurl jansson libxxhash mariadb` -lpthread -ggdb
CC=gcc
DB=gdb
VG=valgrind --leak-check=full
RM=rm


bxcache: ../src/bx_ids_cache.c ../src/bx_object_value.c ../src/bx_object.c bxcache.c
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

