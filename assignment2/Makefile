all:
	make router
	make pktgen
router:
	gcc -std=c99 -m32 router.c -o router
test_router:
	make router
	./router 8585 RT_A.txt router_stats.txt
pktgen:
	gcc -std=c99 -m32 pktgen.c -o pktgen
test_pktgen:
	make pktgen
	./pktgen 8585 pktgen_stats.txt
clean:
	rm pktgen router pktgen_stats.txt router_stats.txt
package:
	tar -cvf dowling-asgn2a.tar router.c pktgen.c Makefile
