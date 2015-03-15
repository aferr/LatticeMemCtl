#include <set>
#include <vector>
#include <iterator>
#include <map>
#include <stdio.h>
#include "lattice.hh"
using namespace std;

Lattice* Lattice::lattice = 0;

SecurityClass::SecurityClass(int _val) : val(_val){
}

bool SecurityClass::operator<=(SecurityClass &that){
  if(this == &that) return true;
  return Lattice::instance()->has_rule(this,&that);
}

std::vector<SecurityClass*>*
SecurityClass::upper_list(){
  if(upper_list_==NULL){
    upper_list_ = new std::vector<SecurityClass*>();
    Lattice::iterator it = Lattice::instance()->begin();
    for(; it!=Lattice::instance()->end(); ++it){
      if(*this <= *(it->second) && this!= it->second){
        upper_list_->push_back(it->second);
      }
    }
    ul_circular = upper_list_->begin();
  }

  return upper_list_;
}

SecurityClass* SecurityClass::next_upper(){
  // Initialized the circular list
  upper_list();

  if(is_top()) return this;
  ul_circular++;
  if(ul_circular == upper_list_->end()){
    ul_circular = upper_list()->begin();
  }
  return *ul_circular;
}

bool SecurityClass::is_top(){
  return upper_list()->size() == 0;
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

    SecurityClass* H = new SecurityClass(0);
    SecurityClass* L = new SecurityClass(1);

    (*this)[0] = H;
    (*this)[1] = L;

    rules = *(new map<SecurityClass*,set<SecurityClass*> >);
    rules[L] = *(new set<SecurityClass*>());
    rules[L].insert(H);
   
    rules[H] = *(new set<SecurityClass*>());

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
