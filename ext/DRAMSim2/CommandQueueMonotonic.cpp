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

void CommandQueueMonotonic::monotonic_check_deadtime(){
    if(!isBufferTime() && isBufferTimePure() && getCurrentPID()==0){
        unsigned ccc_ = currentClockCycle - offset;
        unsigned schedule_length = p0Period + p1Period * (num_pids - 1);
        unsigned schedule_time = ccc_ % (p0Period + (num_pids-1) * p1Period);
        unsigned time_saved = schedule_length - schedule_time;
        for( int i=0; i<time_saved; i++ ){
            (*incr_stat)(monotonic_undead_cycles,0,NULL,NULL);
        }
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
