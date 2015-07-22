.PHONY: all clean
CFLAGS := ""

all:zerver
zerver: main.c rio.c sockfd.c
	clang $? -g -o $@

client:client.c sockfd.c
	clang $? -o $@

#echo:echo.c rio.c
#	clang echo.c rio.c -o echo2

clean:
	find . -maxdepth 2 -type f -executable -exec rm -Iv {} \;

