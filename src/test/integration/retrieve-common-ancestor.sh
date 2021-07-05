TREE=`curl http://localhost:8080/tree -s -f -d '[5<10>15][5>7][13<15][11<13>14]'`
curl http://localhost:8080/tree/$TREE/common-ancestor/11/14
