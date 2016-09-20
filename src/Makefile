all: server
server: main.o user.h field.h board.h board.o user.o field.o
	g++ -std=c++14 -g -o server main.o board.o user.o field.o -lboost_context -lboost_thread -lboost_system -pthread -Iinclude -Llib -Igoplus/include -lz
main.o:main.cpp user.h field.h board.h
	g++ -std=c++14 -c -g main.cpp -lboost_context -lboost_thread -lboost_system -pthread -Iinclude -Llib -Igoplus/include
board.o:board.cpp  field.h board.h
	g++ -std=c++14 -c -g board.cpp -lboost_context -lboost_thread -lboost_system -pthread -Iinclude -Llib -Igoplus/include
user.o:user.h user.cpp field.h board.h
	g++ -std=c++14 -c -g user.cpp -lboost_context -lboost_thread -lboost_system -pthread -Iinclude -Llib -Igoplus/include
field.o:user.h board.h field.cpp field.h
	g++ -std=c++14 -c -g field.cpp -lboost_context -lboost_thread -lboost_system -pthread -Iinclude -Llib -Igoplus/include
run:
	LD_LIBRARY_PATH=lib ./server
clean:
	rm *.o
