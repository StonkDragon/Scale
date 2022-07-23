install:
	rm -rf $${HOME}/Scale
	cp -r Scale $${HOME}
	mkdir -p $${HOME}/Scale/bin
	clang++ -std=gnu++20 -o $${HOME}/Scale/bin/sclc src/Main.cpp
	gcc -std=gnu18 -o $${HOME}/Scale/comp/scale.o -c Scale/comp/scale.c
	rm -rf $${HOME}/Scale/comp/scale.c
