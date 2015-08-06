.PHONY: all clean
CFLAGS := ""

all:zerver

zerver: main.c rio.c sockfd.c
	clang $^ -g -o $@

client:client.c sockfd.c
	clang $^ -o $@

clean:
	@rm zerver client 2>/dev/null || true

