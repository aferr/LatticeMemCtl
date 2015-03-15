#ifndef cctp
#include "CommandQueueTP.h"
#define cctp
#endif

using namespace std;

namespace DRAMSim
{
    class CommandQueueDonor : public CommandQueueTP
    {
        public:
            CommandQueueDonor(vector< vector<BankState> > &states,
                    ostream &dramsim_log_,unsigned tpTurnLength,
                    int num_pids, bool fixAddr_,
                    bool diffPeriod_, int p0Period_,
                    int p1Period_, int offset_, int lattice_config_);
            virtual void step();
            virtual unsigned getCurrentPID();
            int nextHigherTC(unsigned tcid); 
            int nextHigherTC_config1(unsigned tcid); 
            int nextHigherTC_config2(unsigned tcid); 
        bool donated;
        unsigned tcid_donated_to;
        int lattice_config;
    };

}
