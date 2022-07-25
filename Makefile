install:
	rm -rf $${HOME}/Scale
	cp -r Scale $${HOME}
	mkdir -p $${HOME}/Scale/bin
	clang++ -std=gnu++20 -o $${HOME}/Scale/bin/sclc src/compiler/Main.cpp
	clang -std=gnu18 -o $${HOME}/Scale/comp/scale.o -c src/stdlib/scale.c
	rm -rf $${HOME}/Scale/comp/scale.c
	python3 tests.py run

install-x64:
	rm -rf $${HOME}/Scale
	cp -r Scale $${HOME}
	mkdir -p $${HOME}/Scale/bin
	clang++ -std=gnu++20 -arch x86_64 -o $${HOME}/Scale/bin/sclc src/compiler/Main.cpp
	clang -std=gnu18 -arch x86_64 -o $${HOME}/Scale/comp/scale.o -c src/stdlib/scale.c
	rm -rf $${HOME}/Scale/comp/scale.c
	python3 tests.py run

tests:
	python3 tests.py run

reset-tests:
	python3 tests.py reset
