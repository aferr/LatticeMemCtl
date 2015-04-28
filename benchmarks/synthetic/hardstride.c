#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int main(int argc, char **argv) {
    register int MEM_SIZE = 10 * 1024 * 1024; // *32 bits (number of ints)
    register int DURATION = 60 * 1000 * 1000;
    register int DELAY_OPS = 1;
    register int count = 0;

    int * mem = ( int * ) malloc( sizeof( int ) * MEM_SIZE );
    register int tmp=0;

    while(1) { 
        register int read_addr  = count % MEM_SIZE;
        tmp = mem[read_addr];
        count += 16;
    }
}

