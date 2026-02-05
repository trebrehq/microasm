src = main.c
target = microasm

$(target): $(src)
	gcc $(src) -o $(target) -Wall -Wextra -O2 -Wno-unused-parameter -Wno-unused-result

all: $(target)

run: $(target)
	./$(target) test.asm test.hex

clean:
	rm -f $(target)