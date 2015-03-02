#include <map>


class SecurityClass{
    public:
    SecurityClass(int _val) : val(_val){}

    bool operator<=(SecurityClass* that){
        return this->val <= that->val;
    }

    SecurityClass* join(SecurityClass* that){
        if(this->val <= that->val) return that;
        return this;
    }

    SecurityClass* meet(SecurityClass* that){
        if(this->val <= that->val) return this;
        return that;
    }

    private:
    int val;
};

class Lattice : public std::map<int,SecurityClass*>{

    private:
    static Lattice* lattice;

    Lattice(){
        for(int i=0; i<8; i++){
            (*this)[i] = new SecurityClass(i);
        }
    }

    //Lattice(Lattice const&){};

    public:
    static Lattice* instance(){
        if(!lattice){
            lattice = new Lattice();
        }
        return lattice;
    }

};

