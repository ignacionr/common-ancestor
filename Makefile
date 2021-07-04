all: build/common-ancestor

build/common-ancestor: src/*
	mkdir -p build
	cd build && cmake ../src && make

build/test-common-ancestor: src/*
	mkdir -p build
	cd build && cmake ../src && make

clean:
	rm -rf build

run: build/common-ancestor
	cd build && ./common-ancestor

bg: build/common-ancestor
	cd build && ./common-ancestor &

kill: 
	pkill common-ancestor

test-integration: bg post-tree.pass kill

%.pass: src/test/integration/%.sh
	$<
test: build/test-common-ancestor
	cd build && make && make test
