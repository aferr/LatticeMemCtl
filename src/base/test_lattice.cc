#include "lattice.hh"
#include <stdio.h>
int main(){
    Lattice l = *(Lattice::instance());
    SecurityClass* L = l[0];
    SecurityClass* H = l[1];
    SecurityClass* H2 = l[2];
    if(*L <=  *H) fprintf(stderr, "makes sense\n");
    if(*L <= *H2) fprintf(stderr, "makes sense\n");
    if(*L <=  *L) fprintf(stderr, "makes sense\n");
    if(*H <=  *L) fprintf(stderr, "1doesnt make sense\n");
    if(*H <= *H2) fprintf(stderr, "2doesnt make sense\n");
    if(*H2 <= *H) fprintf(stderr, "3doesnt make sense\n");
    if(*H2 <= *L) fprintf(stderr, "3doesnt make sense\n");

    // L->upper_list();
    // H->upper_list();
    // H2->upper_list();

    fprintf(stderr, "L: %p\n",  (void*) L);
    fprintf(stderr, "H: %p\n",  (void*) H);
    fprintf(stderr, "H2: %p\n", (void*) H2);

    std::vector<SecurityClass*>::iterator it = L->upper_list()->begin();
    for(;it != L->upper_list()->end(); it++){
        fprintf(stderr, "an upper for L: %p\n",  (void*) *it);
    }

    it = H->upper_list()->begin();
    for(;it != H->upper_list()->end(); it++){
        fprintf(stderr, "an upper for H: %p\n",  (void*) *it);
    }

    fprintf(stderr, "next: %p\n", (void*) L->next_upper());
    fprintf(stderr, "next: %p\n", (void*) L->next_upper());
    fprintf(stderr, "next: %p\n", (void*) L->next_upper());

    if(H->is_top()) fprintf(stderr, "yes\n");
}
