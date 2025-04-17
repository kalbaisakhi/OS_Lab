run: LRU.cpp
	g++ -Wall -o runsearch LRU.cpp
	./runsearch>check.txt

vrun: LRU.cpp
	g++ -Wall -DVERBOSE -o runsearch LRU.cpp
	./runsearch>check.txt

db: gensearch.c
	gcc -Wall -o gensearch gensearch.c
	./gensearch

clean:
	-rm -f runsearch gensearch check.txt