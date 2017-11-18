all:
	cc -Wall -Wextra -Wpedantic main.c notify.c server.c -o notifys
clean:
	rm -fr notifys
