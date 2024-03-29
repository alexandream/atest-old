CC=clang

CFLAGS=-pedantic -Wall -std=c89 -g

SOURCE_FILES=src/atest.c
OBJ_FILES=$(SOURCE_FILES:src/%.c=build/%.o)

all:build/libatest.a build/atest.h

-include $(patsubst %.o, %.d, $(OBJ_FILES))


build/atest.h: src/atest.h
	@cp src/atest.h build/atest.h

build/libatest.a: $(OBJ_FILES)
	@ar rcs build/libatest.a $^


build/%.o: src/%.c
	@${CC} -c $< -o $@ ${CFLAGS}
	@${CC} -MM -MT $@ $(CFLAGS) $< > $(@:.o=.d)


build/runner: runner.c build/libatest.a
	@${CC} runner.c -latest -L build/ ${CFLAGS} -o build/runner

run: build/runner
	build/runner

clean:
	@rm -f build/*
