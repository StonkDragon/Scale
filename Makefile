install:
	rm -rf $${HOME}/Scale
	cp -r Scale $${HOME}
	mkdir -p $${HOME}/Scale/bin
	clang++ -std=gnu++20 -o $${HOME}/Scale/bin/sclc src/compiler/Main.cpp
	gcc -std=gnu18 -o $${HOME}/Scale/comp/scale.o -c src/stdlib/scale.c
	rm -rf $${HOME}/Scale/comp/scale.c
