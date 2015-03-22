#ifndef cctp
#include "CommandQueueTP.h"
#define cctp
#endif

using namespace std;

namespace DRAMSim
{
    class CommandQueueMonotonic: public CommandQueueTP
    {
        public:
            CommandQueueMonotonic(vector< vector<BankState> > &states,
                    ostream &dramsim_log_,unsigned tpTurnLength,
                    int num_pids, bool fixAddr_,
                    bool diffPeriod_, int p0Period_,
                    int p1Period_, int offset_, int lattice_config_);
        int lattice_config;
        virtual int normal_deadtime(int tlength);
        virtual int refresh_deadtime(int tlength);
        bool current_tcid_is_top();
    };

}


