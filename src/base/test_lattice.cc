#include "lattice.hh"
#include <stdio.h>
int main(){
    Lattice l = *(Lattice::instance());
    if(l[0]<=l[1]) fprintf(stderr, "foo\n");
    if(l[1]<=l[0]) fprintf(stderr, "bar\n");
}
