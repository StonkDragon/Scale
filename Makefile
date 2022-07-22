install:
	clear
	cp -r Scale $${HOME}
	clang++ -std=gnu++20 -o $${HOME}/Scale/bin/sclc src/Main.cpp

hello-example:
	sclc examples/hello.scale
	./examples/hello.scl
