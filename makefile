CC=g++

includepath=/usr/include
libpath=/usr/bin
vpath=./

CFLAGS=-I$(includepath) -std=c++11 -O2 -Wall
LINKFLAGS=-L$(libpath) -lodbc -lyaml-cpp -L/usr/local/lib -lboost_system -lpthread -Wall -Wl,-rpath $(libpath)

%.o:%.c
	$(CC) -g -c $(CFLAGS) $< -o $@

all : main

.PHONY : all clean rebuild

main : src/main.cpp src/cpp_odbc.cpp include/cpp_odbc.h
	$(CC) $(CFLAGS) -g include/cpp_odbc.h include/data.h include/thread_pool.h src/cpp_odbc.cpp src/thread_pool.cpp src/main.cpp $(LINKFLAGS) -o main
	@echo make ok.

clean :
	@rm -rf *.o
	@rm -rf main

rebuild : clean all
