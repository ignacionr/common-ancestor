all: build/common-ancestor

build/common-ancestor: src/*
	mkdir -p build
	cd build && cmake ../src && make -j

build/test-common-ancestor: src/*
	mkdir -p build
	cd build && cmake ../src && make

clean:
	rm -rf build

run: build/common-ancestor
	cd build && ./common-ancestor

bg: build/common-ancestor
	build/common-ancestor &
	sleep 3

kill: 
	pkill common-ancestor

test-integration: bg post-tree.pass retrieve-common-ancestor.pass index-page.pass kill

%.pass: src/test/integration/%.sh
	$<
test: build/test-common-ancestor
	cd build && make && make test

docker:
	docker build . -t common-ancestor
	docker run -p 8080:8080 common-ancestor
