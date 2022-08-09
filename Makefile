VERSION=2.8

dev:
	rm -rf $${HOME}/Scale
	cp -r Scale $${HOME}
	sudo clang++ -std=gnu++17 -o /usr/local/bin/sclc src/sclc/Main.cpp -DVERSION="\"$(VERSION)\""
	./makebin $(VERSION) macos-x86-64
	python3 tests.py run

comp:
	mkdir compile
	clang++ -std=gnu++17 -o compile/sclc src/sclc/Main.cpp -DVERSION="\"$(VERSION)\""

tests:
	python3 tests.py run

reset-tests:
	python3 tests.py reset
