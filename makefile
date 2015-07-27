.PHONY: all clean
CFLAGS := ""

all:zerver

zerver: main.c rio.c sockfd.c
	clang $^ -g -o $@

client:client.c sockfd.c
	clang $^ -o $@

clean:
	find . -maxdepth 2 -type f -executable -exec rm -Iv {} \;

