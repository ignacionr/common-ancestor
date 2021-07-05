# common-ancestor

## Requirements

You will need **Docker, make** for a Docker build.

## Build and run

```shell
make docker
```

This will build and run a container, setup the code, compile, and run unit tests.

## Try it

### With cUrl

```shell
TREE=`curl http://localhost:8080/tree -s -f -d '[5<10>15][5>7][13<15][11<13>14]'` && \
curl http://localhost:8080/tree/$TREE/common-ancestor/11/14
```
