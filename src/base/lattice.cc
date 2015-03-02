#include "lattice.hh"
Lattice* Lattice::lattice = 0;

SecurityClass::SecurityClass(int _val) : val(_val){
}

bool SecurityClass::operator<=(SecurityClass* that){
  if(this == that) return true;
  return Lattice::instance()->has_rule(this,that);
}

std::vector<SecurityClass*>::iterator
SecurityClass::upper_list(){
  if(!upper_list_){
    std::vector<SecurityClass*>* upper_list_ = new std::vector<SecurityClass*>();
    Lattice::iterator it = Lattice::instance()->begin();
    for(; it!=Lattice::instance()->end(); it++){
      if(this <= it->second) upper_list_->push_back(it->second);
    }
  }
  return upper_list_->begin();
}

SecurityClass* SecurityClass::next_upper(){
  if(is_top()) return this;
  if(*ul_circular == NULL) ul_circular = upper_list();
  if(ul_circular == upper_list_->end()){
    ul_circular = upper_list();
  } else {
    ul_circular = ul_circular++;
  }
  return *ul_circular;
}

bool SecurityClass::is_top(){
  return (upper_list()++) == upper_list_->end();
}

SecurityClass* SecurityClass::join(SecurityClass* that){
    // TODO this is not correct
    if(this->val <= that->val) return that;
    return this;
}

SecurityClass* SecurityClass::meet(SecurityClass* that){
     // TODO this is not correct
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

bool Lattice::has_rule(SecurityClass* x, SecurityClass* y){
  return Lattice::instance()->rules[x].find(y) !=
    Lattice::instance()->rules[x].end();
}

