#include "CommandQueueDonor.h"

CommandQueueDonor::step(){

    SimulatorObject::step();

    unsigned ccc_ = currentClockCycle - offset;
    unsigned schedule_time = ccc_ % (p0Period + (num_pids-1) * p1Period);
    
    int nat_tcid = CommandQueueTP::getCurrentPID();
    bool is_turn_start = schedule_time==0 ||
        ((schedule_time-p0Period)%p1Period==0); 

    // If it's the start of a turn and the next TC can't fill its slot, try to 
    // replace it
    if(is_turn_start && tcidEmpty(nat_tcid)){

    }
}

CommandQueue::nextHigherTC(int tcid){
    //To start, assume the lattice is always linear
    next = tcid+1;
    //bool next_is_top = Lattice::instance()->is_top(next);
    bool next_is_top = next == (num_pids-1);

    if(next_is_top) return next;
    if(tcidEmpty(nat_tcid)) return nextHigherTC(next);
    return next;
}
