default:stencil stencil-cpp 

CC = mpiicc
#CC = mpigxx
CFLAGS = -O3 -xAVX -openmp
CLIBS  = -openmp

stencil:main.o
	$(CC) $(CLIBS) main.o wxl.a -o stencil

main.o:main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

stencil-cpp:main-cpp.o
	$(CC) $(CLIBS) main-cpp.o wxl.a -o stencil-cpp

main-cpp.o:main.cpp
	$(CC) $(CFLAGS) -c main.cpp -o main-cpp.o

stencil-bench:main-bench.o
	$(CC) $(CLIBS) main-bench.o wxl.a -o stencil-bench

main-bench.o:main-bench.cpp
	$(CC) $(CLIBS) -c main-bench.cpp -o main-bench.o

stencil-3d:main-3d.o
	$(CC) $(CLIBS) main-3d.o wxl.a -o stencil-3d

main-3d.o:main-3d.cpp
	$(CC) $(CLIBS) -c main-3d.cpp -o main-3d.o

bench:
	make stencil-bench

3d:
	make stencil-3d

clean:
	rm -rf stencil* *.o

