#ifndef ccip
#include "CommandQueueInvPrio.h"
#define ccip
#endif

#include<cassert>

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
    epoch_length = 2; //num_pids*(num_pids+1)/2;
    epoch_remaining = epoch_length;

    bandwidth_limit = ((int*) malloc(sizeof(int) * num_pids));
    if(lattice_config == 1){
        for(int i=0; i < num_pids; i++) bandwidth_limit[i] = 1; //num_pids - i;
    } else {
        for(int i=0; i < num_pids; i++) bandwidth_limit[i] = 1; //i;
    }

    bandwidth_remaining = ((int*) malloc(sizeof(int) * num_pids));
    for(int i=1; i < num_pids; i++) bandwidth_remaining[i]=bandwidth_limit[i];

    turn_owner = bottom();
}

unsigned CommandQueueInvPrio::select_turn_owner(){
    unsigned tcid_candidate = bottom();
    while(!is_top(tcid_candidate)){
        bool has_bw = bandwidth_remaining[tcid_candidate] != 0;
        if(!tcidEmpty(tcid_candidate) && has_bw) break;
        else tcid_candidate = next_higher_tc(tcid_candidate); 
    }
    bandwidth_remaining[tcid_candidate] -= 1;
    return tcid_candidate;
}

void CommandQueueInvPrio::step(){
    SimulatorObject::step();
    
    unsigned ccc_ = currentClockCycle - offset;
    unsigned schedule_time = ccc_ % (p0Period + (num_pids-1) * p1Period);
    bool is_turn_start = schedule_time==0 ||
        ((schedule_time-p0Period) % p1Period == 0);

    if(is_turn_start){
        if(epoch_remaining == 0){
            epoch_remaining = epoch_length;
            for(int i=0; i < num_pids; i++){
                bandwidth_remaining[i] = bandwidth_limit[i];
            }
        }
        turn_owner = select_turn_owner();
        epoch_remaining -= 1;
    }

    CommandQueueTP::update_stats();

}

unsigned CommandQueueInvPrio::getCurrentPID(){
    assert(turn_owner < num_pids);
    return turn_owner;
}

unsigned CommandQueueInvPrio::next_higher_tc(unsigned tcid){
    switch (lattice_config){
        case 1: return next_higher_tc_config1(tcid);
        case 2: return next_higher_tc_config2(tcid);
        default: return next_higher_tc_config1(tcid);
    }
}

unsigned CommandQueueInvPrio::next_higher_tc_config1(unsigned tcid){
    if(tcid==0){
        return 0;
    } else {
        if(tcidEmpty(tcid-1)) return next_higher_tc(tcid-1);
        else return tcid-1;
    }
}

unsigned CommandQueueInvPrio::next_higher_tc_config2(unsigned tcid){
    if( tcid == num_pids - 1 ){
        return num_pids-1;
    } else {
        if(tcidEmpty(tcid+1)) return next_higher_tc(tcid+1);
        else return tcid+1;
    }
}

unsigned CommandQueueInvPrio::bottom(){
    switch (lattice_config){
        case 1: return num_pids - 1;
        case 2: return 0;
        default: return num_pids - 1;
    }
}

bool CommandQueueInvPrio::is_top(unsigned int i){
    switch (lattice_config){
        case 1: return i==0;
        case 2: return i == (num_pids - 1);
        default: return i==0;
    }
}

int CommandQueueInvPrio::normal_deadtime(int tlength){
    return 0;
}

int CommandQueueInvPrio::refresh_deadtime(int tlength){
    //return 0;
    return CommandQueueTP::refresh_deadtime(tlength);
}

