#ifndef cctp
#include "CommandQueueTP.h"
#define cctp
#endif

using namespace std;

namespace DRAMSim
{
    class CommandQueueInvPrio: public CommandQueueTP
    {
        public:
            CommandQueueInvPrio(vector< vector<BankState> > &states,
                    ostream &dramsim_log_,unsigned tpTurnLength,
                    int num_pids, bool fixAddr_,
                    bool diffPeriod_, int p0Period_,
                    int p1Period_, int offset_, int lattice_config_);
            int lattice_config;
    };

}
