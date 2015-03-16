all:
	gcc -std=c99 -m32 router.c pktgen.c -0 simulator
test:
	gcc -std=c99 -m32 test.c router.c -o tester
	./tester
clean:
	rm tester
package:
	tar -cvf dowling-asgn1.tar router.c router.h pktgen.c Makefile
