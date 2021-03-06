#ifndef ccdonor
#include "CommandQueueDonor.h"
#define ccdonor
#endif
using namespace DRAMSim;
using namespace std;

CommandQueueDonor::CommandQueueDonor(vector< vector<BankState> > &states, 
        ostream &dramsim_log_, unsigned tpTurnLength_,
        int num_pids_, bool fixAddr_,
        bool diffPeriod_, int p0Period_, int p1Period_, int offset_, int lattice_config_) :
        CommandQueueTP(states,dramsim_log_,tpTurnLength_,num_pids_,fixAddr_,
          diffPeriod_, p0Period_, p1Period_, offset_),
        lattice_config(lattice_config_)
{
  donated = false;
  tcid_donated_to = 0;
}

void
CommandQueueDonor::step(){

    SimulatorObject::step();

    unsigned ccc_ = currentClockCycle - offset;
    unsigned schedule_time = ccc_ % (p0Period + (num_pids-1) * p1Period);
    
    unsigned nat_tcid = CommandQueueTP::getCurrentPID();
    bool is_turn_start = schedule_time==0 ||
        ((schedule_time-p0Period)%p1Period==0);

    // If it's the start of a turn and the next TC can't fill its slot, try to 
    // replace it
    if(is_turn_start){
      donated = false;
      if(tcidEmpty(nat_tcid)){
        donated = true;
        tcid_donated_to = nextHigherTC(nat_tcid);
        (*incr_stat)(donations,tcid_donated_to,1,NULL);
      }
    }

}

void CommandQueueDonor::update_stats(){
    CommandQueueTP::update_stats();
    for(int i=0; i < num_pids; i++){
        if( !tcidEmpty(i) && getCurrentPID()!=i &&
            tcidEmpty(getCurrentPID()) &&
            !tcidEmpty(CommandQueueTP::getCurrentPID()) ){
                    (*incr_stat)(donation_overhead,i,
                            queueSizeByTcid(i),NULL);
        }
    }
}

void CommandQueueDonor::check_donor_issue(){
    if(donated){
        unsigned ccc_ = currentClockCycle - offset;
        unsigned schedule_length = p0Period + p1Period * (num_pids - 1);
        unsigned schedule_time = ccc_ % (p0Period + (num_pids-1) * p1Period);
        unsigned time_saved = schedule_length - schedule_time;
        (*incr_stat)(donated_issue_cycles,getCurrentPID(),
                time_saved * (queueSizeByTcid(getCurrentPID()) +1),NULL);
    }
}

unsigned
CommandQueueDonor::getCurrentPID(){
  if(donated){
      return tcid_donated_to;
  }
  return CommandQueueTP::getCurrentPID();
}

int
CommandQueueDonor::nextHigherTC(unsigned tcid){
    switch(lattice_config){
        case 1: return nextHigherTC_config1(tcid);
        case 2: return nextHigherTC_config2(tcid);
        default: return nextHigherTC_config1(tcid);
    }

}

int
CommandQueueDonor::nextHigherTC_config1(unsigned tcid){
    if(tcid==0){
        return 0;
    } else {
        if(tcidEmpty(tcid-1)) return nextHigherTC(tcid-1);
        else return tcid-1;
    }
}

int
CommandQueueDonor::nextHigherTC_config2(unsigned tcid){
    if( tcid == num_pids - 1 ){
        return num_pids-1;
    } else {
        if(tcidEmpty(tcid+1)) return nextHigherTC(tcid+1);
        else return tcid+1;
    }
}
