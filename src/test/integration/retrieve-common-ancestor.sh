#!/bin/bash

TREE=`curl http://localhost:8080/tree -s -f -d '[5<10>15][5>7][13<15][11<13>14]'`
echo tree: $TREE
ANCESTOR=`curl http://localhost:8080/tree/$TREE/common-ancestor/11/14 -s`
echo ancestor: $ANCESTOR

if [ "$ANCESTOR" = "13" ]
then
  echo OK
else
  echo NOT OK
  exit -1
fi
