#ifndef ccmono
#include "CommandQueueMonotonic.h"
#define ccmono
#endif
using namespace DRAMSim;
using namespace std;

CommandQueueMonotonic::CommandQueueMonotonic(vector< vector<BankState> > &states, 
        ostream &dramsim_log_, unsigned tpTurnLength_,
        int num_pids_, bool fixAddr_,
        bool diffPeriod_, int p0Period_, int p1Period_, int offset_, int lattice_config_) :
        CommandQueueTP(states,dramsim_log_,tpTurnLength_,num_pids_,fixAddr_,
          diffPeriod_, p0Period_, p1Period_, offset_),
        lattice_config(lattice_config_)
{
}

void CommandQueueMonotonic::step(){
    CommandQueueTP::step();
    if(!isBufferTime() && isBufferTimePure() && !tcidEmpty(getCurrentPID())){
           (*incr_stat)(monotonic_dead_time_recovered,getCurrentPID(),
                   queueSizeByTcid(getCurrentPID()),NULL);
    }
}

bool CommandQueueMonotonic::current_tcid_is_top(){
    switch(lattice_config){
        case 1: return getCurrentPID() == 0;
        case 2: return getCurrentPID() == num_pids-1;
        default: return getCurrentPID() == 0;
    }
}

int CommandQueueMonotonic::normal_deadtime(int tlength){
    if(current_tcid_is_top()){
        return CommandQueueTP::normal_deadtime(tlength);
    }
    return 0;
}

int CommandQueueMonotonic::refresh_deadtime(int tlength){
    //if(current_tcid_is_top()){
        return CommandQueueTP::refresh_deadtime(tlength);
    //}
    //return 0;
}
