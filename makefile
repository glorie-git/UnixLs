all:
	gcc -g -Wall -Werror UnixLs.c -o UnixLs

clean:
	rm -f UnixLs
