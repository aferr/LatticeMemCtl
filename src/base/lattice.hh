#include <map>


class SecurityClass{
    public:
    SecurityClass(int _val);
    bool operator<=(SecurityClass* that);
    SecurityClass* join(SecurityClass* that);
    SecurityClass* meet(SecurityClass* that);

    private:
    int val;
};

class Lattice : public std::map<int,SecurityClass*>{

    private:
    static Lattice* lattice;

    Lattice();
    
    static Lattice* instance();

};

