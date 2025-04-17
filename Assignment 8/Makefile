all:
	g++ -Wall -o resource -pthread resource.cpp
	g++ -Wall -D_DLAVOID -o resource_nodeadlock -pthread resource.cpp

allow: all
	./resource>output.txt

avoid: all
	./resource_nodeadlock>output.txt

db: geninput.c
	gcc -Wall -o geninput geninput.c
	./geninput 10 20

clean:
	rm -f resource resource_nodeadlock geninput