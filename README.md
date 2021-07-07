# common-ancestor

## Requirements

You will need **Docker, make** for a Docker build.
For a local build, **cmake, make, an updated C++ compiler, curl**.

## Build and run

```shell
make docker
```

This will build and run a container, setup the code, compile, and run unit tests. Finally, it will create a container with the built image, and run the program exposing port 8080.

## Try it

### With curl

```shell
TREE=`curl http://localhost:8080/tree -s -f -d '[5<10>15][5>7][13<15][11<13>14]'` && \
curl http://localhost:8080/tree/$TREE/common-ancestor/11/14
```

### With the embedded Web page

Open the url [http://localhost:8080/](http://localhost:8080/) with your browser.

## With docker-compose

```shell
docker-compose build
docker-compose up
```

This will create three common-ancestor instances, on three containers, and an Nginx container acting as a smart 
load balancer.

The common-ancestor instances, as configured for load-balancing, will use a prefix for all trees created. The prefix (t1, t2, t3) is also the name of each host.

When an operation (other than create) is requested on a tree, the load-balancer will extract from the requested uri, the 
name of the server holding that particular tree. This will create affinity, but saves us from more expensive sync mechanisms.

IMHO it is a very simple way of scaling.

## Running Tests

### Unit Tests

Using Google Test. They run when building the container.
If you prefer to run on your host, you must have a compiler installed, and then:
```shell
build/test-common-ancestor
```
or alternatively,
```shell
make test
```

### Integration Tests

These tests require a local build and curl.

```shell
make test-integration
```
