unoServer: server_uno.o server_main.o
	g++-10 -Wall -pedantic -g -o unoServer server_uno.o server_main.o 
server_uno.o: server_uno.cpp server_uno.h cards.h
	g++-10 -Wall -pedantic -g -std=c++2a -c server_uno.cpp
server_main.o: server_uno.h cards.h
	g++-10 -Wall -pedantic -g -std=c++2a -c server_main.cpp
clean:
	rm -f unoServer server_uno.o server_main.o
