VERSION=2.5

install:
	rm -rf $${HOME}/Scale
	cp -r Scale $${HOME}
	mkdir -p $${HOME}/Scale/bin
	clang++ -std=gnu++17 -o $${HOME}/Scale/bin/sclc src/compiler/Main.cpp -DVERSION="\"$(VERSION)\""
	clang -std=gnu17 -o $${HOME}/Scale/comp/scale.o -c src/stdlib/scale.c -DVERSION="\"$(VERSION)\""
	rm -rf $${HOME}/Scale/comp/scale.c
	python3 tests.py run

build:
	./makebin $(VERSION)

tests:
	python3 tests.py run

reset-tests:
	python3 tests.py reset
