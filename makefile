.PHONY: all clean

all:zerver

zerver:main.c rio.c client
	clang main.c rio.c -g -o $@

client:client.c
	clang $< -o $@

#echo:echo.c rio.c
#	clang echo.c rio.c -o echo2

clean:
	find . -maxdepth 2 -type f -executable -exec rm -Iv {} \;

