default:
	gcc -O3 -g -c sha256/sha256.c -o sha256.o
	gcc -O3 -g -c base58/base58.c -o base58.o
	gcc -O3 -g -c rmd160/rmd160.c -o rmd160.o
	gcc -O3 -g -c queue.c -o queue.o
	gcc -O3 -g -c threadpool.c -o threadpool.o
	gcc -O3 -g -c gmpecc.c -o gmpecc.o
	gcc -O3 -g -c util.c -o util.o
	gcc -g -o keysubtracter keysubtracter.c gmpecc.o util.o sha256.o base58.o rmd160.o queue.o threadpool.o -lgmp -lpthread -lrt
pool:
	gcc -O3 -g -c queue.c -o queue.o
	gcc -o -g threadpool threadpool.c queue.o -lpthread

clean:
	rm -r *.o
