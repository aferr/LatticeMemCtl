#include <map>
#include <iterator>
#include <vector>
#include <set>

class Lattice;

class SecurityClass {
    public:
    SecurityClass(int _val);
    bool operator<=(SecurityClass* that);
    SecurityClass* join(SecurityClass* that);
    SecurityClass* meet(SecurityClass* that);
    std::vector<SecurityClass*>::iterator upper_list();
    SecurityClass* next_upper();
    bool is_top();

    private:
    int val;
    std::vector<SecurityClass*>* upper_list_;
    std::vector<SecurityClass*>::iterator ul_circular;
};

class Lattice : public std::map<int,SecurityClass*>{
    public:
    static Lattice* instance();
    static bool has_rule(SecurityClass*, SecurityClass*);

    private:
    map<SecurityClass*,std::set<SecurityClass*> > rules;
    static Lattice* lattice;
    Lattice();
};
