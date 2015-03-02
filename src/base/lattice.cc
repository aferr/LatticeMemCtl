#include "lattice.hh"
Lattice* Lattice::lattice = 0;

SecurityClass::SecurityClass(int _val) : val(_val){}

bool SecurityClass::operator<=(SecurityClass* that){
    return this->val <= that->val;
}

SecurityClass* SecurityClass::join(SecurityClass* that){
    if(this->val <= that->val) return that;
    return this;
}

SecurityClass* SecurityClass::meet(SecurityClass* that){
    if(this->val <= that->val) return this;
    return that;
}


Lattice::Lattice(){
    for(int i=0; i<8; i++){
        (*this)[i] = new SecurityClass(i);
    }
}


Lattice* Lattice::instance(){
    if(!lattice){
        lattice = new Lattice();
    }
    return lattice;
}
