echo Accepts a valid tree
curl http://localhost:8080/tree -f -s -d '[5<10>15][5>7][13<15][11<13>14]' >/dev/null
echo OK
