all:connect

connect:ser cli

ser:
	gcc ser.c -o ser

cli:
	gcc cli.c -o cli

clean:
	rm -rf *o ser cli
