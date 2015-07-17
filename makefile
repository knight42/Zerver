.PHONY: all clean

all:zerver

zerver:main.c
	clang $< -o $@
clean:
	find . -maxdepth 1 -name *.out -delete
	find . -maxdepth 1 -type f -executable -delete

