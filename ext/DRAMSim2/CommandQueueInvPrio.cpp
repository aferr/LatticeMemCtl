#ifndef ccip
#include "CommandQueueInvPrio.h"
#define ccip
#endif
using namespace DRAMSim;
using namespace std;

CommandQueueInvPrio::CommandQueueInvPrio(vector< vector<BankState> > &states, 
        ostream &dramsim_log_, unsigned tpTurnLength_,
        int num_pids_, bool fixAddr_,
        bool diffPeriod_, int p0Period_, int p1Period_, int offset_,
        int lattice_config_) :
        CommandQueueTP(states,dramsim_log_,tpTurnLength_,num_pids_,fixAddr_,
          diffPeriod_, p0Period_, p1Period_, offset_),
        lattice_config(lattice_config_)
{
    fprintf(stderr, "Good evening from the inverse priority command queue :)\n");
}

