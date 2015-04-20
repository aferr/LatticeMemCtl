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
      
        // Lattice configuration used 
        int lattice_config;
        // Size of an epoch
        int epoch_length;
        // Time left in this epoch
        int epoch_remaining;
        // Array[num_pids] of maximum turns used per epoch per domain
        int *bandwidth_limit;
        // Array[num_pids] of bandwidth left in the epoch per domain
        int *bandwidth_remaining;
        // The domain which has been granted the current turn
        unsigned turn_owner;

        virtual void step();

        virtual unsigned select_turn_owner();
        virtual unsigned getCurrentPID();

        unsigned next_higher_tc(unsigned tcid);
        unsigned next_higher_tc_config1(unsigned tcid);
        unsigned next_higher_tc_config2(unsigned tcid);
        bool is_top(unsigned tcid);
        unsigned bottom();

        virtual int normal_deadtime(int);
        virtual int refresh_deadtime(int);

    };

}
