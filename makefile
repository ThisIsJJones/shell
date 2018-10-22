C = gcc
CFLAGS = -lfl

shell: lex.yy.c myshell.c
	$(C) -o $@ $^ $(CFLAGS)

%.yy.c: lex.c
	flex lex.c

run: shell
	./shell

clean:
	rm -f *.yy.c shell
