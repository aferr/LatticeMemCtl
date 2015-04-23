#include "CommandQueueTP.h"
#include "SystemConfiguration.h"

using namespace DRAMSim;

CommandQueueTP::CommandQueueTP(vector< vector<BankState> > &states, 
        ostream &dramsim_log_, unsigned tpTurnLength_,
        int num_pids_, bool fixAddr_,
        bool diffPeriod_, int p0Period_, int p1Period_, int offset_,
        map<int,int>* tp_config_
        ) : CommandQueue(states,dramsim_log_,num_pids_)
{
    fixAddr = fixAddr_;
    tpTurnLength = tpTurnLength_;
    diffPeriod = diffPeriod_;
    p0Period = p0Period_;
    p1Period = p1Period_;
	offset = offset_;

    securityPolicy = new TOLattice(this);

    map<int,int> tp_config = *tp_config_;
    switch(tp_config[1]){
        case 0: turnAllocationTimer = new TurnStartAllocationTimer(this); break;
        case 1: turnAllocationTimer = new DeadTimeAllocationTimer(this); break;
        default: turnAllocationTimer = new TurnStartAllocationTimer(this);
    }

    switch(tp_config[2]){
        case 0: turnAllocator = new TDMTurnAllocator(this); break;
        case 1: turnAllocator = new PreemptingTurnAllocator(this); break;
        case 2: turnAllocator = new PriorityTurnAllocator(this); break;
        default: turnAllocator = new TDMTurnAllocator(this);
    }
   
    switch(tp_config[3]){
        case 0: deadTimeCalc = new StrictDeadTimeCalc(this); break;
        case 1: deadTimeCalc = new MonotonicDeadTimeCalc(this); break;
        default: deadTimeCalc = new StrictDeadTimeCalc(this);
    }
}

CommandQueueTP::CommandQueueTP(vector< vector<BankState> > &states, 
        ostream &dramsim_log_, unsigned tpTurnLength_,
        int num_pids_, bool fixAddr_,
        bool diffPeriod_, int p0Period_, int p1Period_, int offset_
        ) : CommandQueueTP(states, dramsim_log_, tpTurnLength_,
            num_pids_, fixAddr_, diffPeriod, p0Period_, p1Period_,
            offset_, new map<int,int>()) {}

void
CommandQueueTP::step(){
   SimulatorObject::step();
   turnAllocationTimer->step();
   // PRINT("-----------------------------------------------------------------------------");
   // PRINT("Time" <<  currentClockCycle);
   // PRINT("Current pid" << getCurrentPID());
   // if(isBufferTime()) PRINT("It is buffer time\n");
   // print();
   // PRINT("-----------------------------------------------------------------------------");
   // PRINT("");
   // PRINT("");
   update_stats();
}

void
CommandQueueTP::update_stats(){
   for(int i=0; i < num_pids; i++){
       if( !tcidEmpty(i) && getCurrentPID()!=i ){
           (*incr_stat)(tmux_overhead,i,
                   qsbytc_ignore_ref(i),NULL);
           if( tcidEmpty(getCurrentPID()) ){
               (*incr_stat)(wasted_tmux_overhead,i,
                       qsbytc_ignore_ref(i),NULL);
           }
       }
   }

   if(isBufferTime() && !tcidEmpty(getCurrentPID())){
       (*incr_stat)(dead_time_overhead, getCurrentPID(),
               queueSizeByTcid(getCurrentPID()),NULL);
   }

    for(int i=0; i < num_pids; i++){
        if( qsbytc_has_nonref(i) && getCurrentPID()!=i &&
            !qsbytc_has_nonref(getCurrentPID()) &&
            qsbytc_has_nonref((new TDMTurnAllocator(this))->current()) ){
                    (*incr_stat)(donation_overhead,i,
                            qsbytc_ignore_ref(i),NULL);
        }
    }

}

int CommandQueueTP::normal_deadtime(int tlength){
    return deadTimeCalc->normal_deadtime();
}

int CommandQueueTP::refresh_deadtime(int tlength){
    return deadTimeCalc->refresh_deadtime();
}

void CommandQueueTP::enqueue(BusPacket *newBusPacket)
{
    unsigned rank = newBusPacket->rank;
    unsigned pid = newBusPacket->threadID;
    newBusPacket->enqueueTime = currentClockCycle;
    newBusPacket->enqueueTimeisSet = true;
#ifdef VALIDATE_STATS
    PRINT("enqueueing at " << currentClockCycle << " ");
    newBusPacket->print();
    print();
#endif /*VALIDATE_STATS*/

    queues[rank][pid].push_back(newBusPacket);
    if (queues[rank][pid].size()>CMD_QUEUE_DEPTH)
    {
        ERROR("== Error - Enqueued more than allowed in command queue");
        ERROR("						Need to call .hasRoomFor(int "
                "numberToEnqueue, unsigned rank, unsigned bank) first");
        exit(0);
    }
}

bool CommandQueueTP::hasRoomFor(unsigned numberToEnqueue, unsigned rank,
        unsigned bank, unsigned pid)
{
    vector<BusPacket *> &queue = getCommandQueue(rank, pid);
    return (CMD_QUEUE_DEPTH - queue.size() >= numberToEnqueue);
}

bool CommandQueueTP::isEmpty(unsigned rank)
{
    for(int i=0; i<num_pids; i++)
        if(!queues[rank][i].empty()) return false;
    return true;
}

bool CommandQueueTP::tcidEmpty(int tcid)
{
    for(int i=0; i<NUM_RANKS; i++)
        if(!queues[i][tcid].empty()) return false;
    return true;
}

int CommandQueueTP::queueSizeByTcid(unsigned tcid){
    int r = 0;
    for(int i=0; i<NUM_RANKS; i++)
        r += queues[i][tcid].size();
    return r;
}

int CommandQueueTP::qsbytc_ignore_ref(unsigned tcid){
    int r = 0;
    for(int i=0; i<NUM_RANKS; i++){
        BusPacket1D q = queues[i][tcid];
        for(int j=0; i<q.size(); j++){
            if(q[j]->busPacketType!=REFRESH) r+=1;
        }
    }
    return r;
}

bool CommandQueueTP::qsbytc_has_nonref(unsigned tcid){
    return qsbytc_ignore_ref(tcid) != 0;
}

vector<BusPacket *> &CommandQueueTP::getCommandQueue(unsigned rank, unsigned 
        pid)
{
    return queues[rank][pid];
}

void CommandQueueTP::refreshPopClosePage(BusPacket **busPacket, bool & 
        sendingREF)
{

    bool foundActiveOrTooEarly = false;
    //look for an open bank
    for (size_t b=0;b<NUM_BANKS;b++)
    {
        vector<BusPacket *> &queue = getCommandQueue(refreshRank,getCurrentPID());
        //checks to make sure that all banks are idle
        if (bankStates[refreshRank][b].currentBankState == RowActive)
        {
            foundActiveOrTooEarly = true;
            //if the bank is open, make sure there is nothing else
            // going there before we close it
            for (size_t j=0;j<queue.size();j++)
            {
                BusPacket *packet = queue[j];
                if (packet->row == 
                        bankStates[refreshRank][b].openRowAddress &&
                        packet->bank == b)
                {
                    if (packet->busPacketType != ACTIVATE && 
                            isIssuable(packet))
                    {
                        *busPacket = packet;
                        queue.erase(queue.begin() + j);
                        (*queue.begin())->beginHeadTime = currentClockCycle;
                        sendingREF = true;
                    }

                    break;
                }
            }

            break;
        }
        //	NOTE: checks nextActivate time for each bank to make sure tRP 
        //	is being
        //				satisfied.	the next ACT and next REF can be issued 
        //				at the same
        //				point in the future, so just use nextActivate field 
        //				instead of
        //				creating a nextRefresh field
        else if (bankStates[refreshRank][b].nextActivate > 
                currentClockCycle)
        {
            foundActiveOrTooEarly = true;
            break;
        }
        //}
    }

    //if there are no open banks and timing has been met, send out the refresh
    //	reset flags and rank pointer
    if (!foundActiveOrTooEarly && bankStates[refreshRank][0].currentBankState 
            != PowerDown)
    {
        *busPacket = new BusPacket(REFRESH, 0, 0, 0, refreshRank, 0, 0, 
                getCurrentPID(), dramsim_log);
        refreshRank = -1;
        refreshWaiting = false;
        sendingREF = true;
    }
}

bool CommandQueueTP::normalPopClosePage(BusPacket **busPacket, bool 
        &sendingREF)
{
    bool foundIssuable = false;
    unsigned startingRank = nextRank;
    unsigned startingBank = nextBank;

    while(true)
    {
        //Only get the queue for the PID with the current turn.
        vector<BusPacket *> &queue = getCommandQueue(nextRank, getCurrentPID());
        //make sure there is something in this queue first
        //	also make sure a rank isn't waiting for a refresh
        //	if a rank is waiting for a refesh, don't issue anything to it until 
        //	the
        //		refresh logic above has sent one out (ie, letting banks close)

        if (!queue.empty() && !((nextRank == refreshRank) && refreshWaiting))
        {

            //search from beginning to find first issuable bus packet
            for (size_t i=0;i<queue.size();i++)
            {

                if (isIssuable(queue[i]))
                {
                    //If a turn change is about to happen, don't
                    //issue any activates
                    
                    if(queue[i]->busPacketType==ACTIVATE){
                        if(isBufferTime())
                            continue;
                    }

                    //check to make sure we aren't removing a read/write that 
                    //is paired with an activate
                    if (i>0 && queue[i-1]->busPacketType==ACTIVATE &&
                            queue[i-1]->physicalAddress == 
                            queue[i]->physicalAddress)
                        continue;

                    *busPacket = queue[i];

                    queue.erase(queue.begin()+i);
                    (*queue.begin())->beginHeadTime = currentClockCycle;
                    foundIssuable = true;
                    break;
                }
            }
        }

        //if we found something, break out of do-while
        if (foundIssuable){
            check_donor_issue();
            break;
        }

        nextRankAndBank(nextRank, nextBank);
        if (startingRank == nextRank && startingBank == nextBank)
        {
            break;
        }
    }

    return foundIssuable;
}

void CommandQueueTP::print()
{
    PRINT("\n== Printing Timing Partitioning Command Queue" );

    for (size_t i=0;i<NUM_RANKS;i++)
    {
        PRINT(" = Rank " << i );
        for (int j=0;j<num_pids;j++)
        {
            PRINT("    PID "<< j << "   size : " << queues[i][j].size() );

            for (size_t k=0;k<queues[i][j].size();k++)
            {
                PRINTN("       " << k << "]");
                queues[i][j][k]->print();
            }
        }
    }
}

unsigned CommandQueueTP::getCurrentPID(){
  return turnAllocator->current();
}

bool CommandQueueTP::isBufferTime(){
  unsigned ccc_ = currentClockCycle - offset;
  unsigned current_tc = (new TDMTurnAllocator(this))->current();
  unsigned schedule_length = p0Period + p1Period * (num_pids - 1);
  unsigned schedule_start = ccc_ - ( ccc_ % schedule_length );

  unsigned turn_start = current_tc == 0 ?
    schedule_start :
    schedule_start + p0Period + p1Period * (current_tc-1);
  unsigned turn_end = current_tc == 0 ?
    turn_start + p0Period :
    turn_start + p1Period;

  // Time between refreshes to ANY rank.
  unsigned refresh_period = REFRESH_PERIOD/NUM_RANKS/tCK;
  unsigned next_refresh = ccc_ + refresh_period - (ccc_ % refresh_period);
 
  unsigned tlength = current_tc == 0 ? p0Period : p1Period;

  unsigned deadtime = (turn_start <= next_refresh && next_refresh < turn_end) ?
    refresh_deadtime( tlength ) :
    normal_deadtime( tlength );

  return ccc_ >= (turn_end - deadtime);
}

bool CommandQueueTP::isBufferTimePure(){
  unsigned ccc_ = currentClockCycle - offset;
  unsigned current_tc = CommandQueueTP::getCurrentPID();
  unsigned schedule_length = p0Period + p1Period * (num_pids - 1);
  unsigned schedule_start = ccc_ - ( ccc_ % schedule_length );

  unsigned turn_start = current_tc == 0 ?
    schedule_start :
    schedule_start + p0Period + p1Period * (current_tc-1);
  unsigned turn_end = current_tc == 0 ?
    turn_start + p0Period :
    turn_start + p1Period;

  // Time between refreshes to ANY rank.
  unsigned refresh_period = REFRESH_PERIOD/NUM_RANKS/tCK;
  unsigned next_refresh = ccc_ + refresh_period - (ccc_ % refresh_period);
 
  unsigned tlength = current_tc == 0 ? p0Period : p1Period;

  unsigned deadtime = (turn_start <= next_refresh && next_refresh < turn_end) ?
    TP_BUFFER_TIME:
    WORST_CASE_DELAY;

  return ccc_ >= (turn_end - deadtime);

}

//=============================================================================
// Turn Ownership Timers
//=============================================================================

//-----------------------------------------------------------------------------
// Turn Start
//-----------------------------------------------------------------------------
bool CommandQueueTP::TurnStartAllocationTimer::is_reallocation_time(){
    unsigned ccc_ = cc->currentClockCycle - cc->offset;
    int schedule_time = ccc_ %
        (cc->p0Period + (cc->num_pids-1) * cc->p1Period);
    bool is_turn_start = schedule_time==0 ||
        ((schedule_time-cc->p0Period)%cc->p1Period==0);
    return is_turn_start;
}

void CommandQueueTP::TurnStartAllocationTimer::step(){
    int p0Period = cc->p0Period;
    int p1Period = cc->p1Period;
    int num_pids = cc->num_pids;
    unsigned ccc_ = cc->currentClockCycle - cc->offset;
    int next_schedule_time = ccc_ % (p0Period + (num_pids-1) * p1Period) + 1;
    bool is_next_new_turn = (next_schedule_time==0) ||
        (((next_schedule_time - p0Period)%p1Period)==0);

    if(is_next_new_turn){
       cc->turnAllocator->allocate_next();
    }
    if(cc->is_turn_start()){
       cc->turnAllocator->allocate_turn();
    }
}

//-----------------------------------------------------------------------------
// Dead Time
//-----------------------------------------------------------------------------
bool CommandQueueTP::DeadTimeAllocationTimer::is_reallocation_time(){
    unsigned ccc_ = cc->currentClockCycle - cc->offset + 1;
    unsigned current_tc = (new TDMTurnAllocator(cc))->current();
    unsigned schedule_length = cc->p0Period + cc->p1Period * (cc->num_pids - 1);
    unsigned schedule_start = ccc_ - ( ccc_ % schedule_length );

    unsigned turn_start = current_tc == 0 ?
      schedule_start :
      schedule_start + cc->p0Period + cc->p1Period * (current_tc-1);
    unsigned turn_end = current_tc == 0 ?
      turn_start + cc->p0Period :
      turn_start + cc->p1Period;

    // Time between refreshes to ANY rank.
    unsigned refresh_period = REFRESH_PERIOD/NUM_RANKS/tCK;
    unsigned next_refresh = ccc_ + refresh_period - (ccc_ % refresh_period);
 
    unsigned deadtime = (turn_start <= next_refresh && next_refresh < turn_end)?
        TP_BUFFER_TIME:
        WORST_CASE_DELAY;

    return ccc_ == (turn_end - deadtime);
}

void CommandQueueTP::DeadTimeAllocationTimer::step(){
    if(cc->is_turn_start()){
        cc->turnAllocator->allocate_turn();
    }
    
    unsigned ccc_ = cc->currentClockCycle - cc->offset;
    unsigned current_tc = (new TDMTurnAllocator(cc))->current();
    unsigned schedule_length = cc->p0Period + cc->p1Period * (cc->num_pids - 1);
    unsigned schedule_start = ccc_ - ( ccc_ % schedule_length );

    unsigned turn_start = current_tc == 0 ?
      schedule_start :
      schedule_start + cc->p0Period + cc->p1Period * (current_tc-1);
    unsigned turn_end = current_tc == 0 ?
      turn_start + cc->p0Period :
      turn_start + cc->p1Period;

    // Time between refreshes to ANY rank.
    unsigned refresh_period = REFRESH_PERIOD/NUM_RANKS/tCK;
    unsigned next_refresh = ccc_ + refresh_period - (ccc_ % refresh_period);
 
    unsigned deadtime = (turn_start <= next_refresh && next_refresh < turn_end)?
        TP_BUFFER_TIME:
        WORST_CASE_DELAY;

    if( ccc_ == (turn_end - deadtime -1)){
         cc->turnAllocator->allocate_next();
    }
    
    //fprintf(stderr, "current: %d\n", cc->getCurrentPID());
}


//=============================================================================
// Dead Time Calculators
//=============================================================================

//-----------------------------------------------------------------------------
// Strict
//-----------------------------------------------------------------------------
int CommandQueueTP::StrictDeadTimeCalc::normal_deadtime(){
    return WORST_CASE_DELAY;
}

int CommandQueueTP::StrictDeadTimeCalc::refresh_deadtime(){
    return TP_BUFFER_TIME;
}

//-----------------------------------------------------------------------------
// Monotonic
//-----------------------------------------------------------------------------
int CommandQueueTP::MonotonicDeadTimeCalc::normal_deadtime(){
    // fprintf(stderr, "asked for dead time at time %lu with current %d"
    //         "and next %d\n",  cc->currentClockCycle,
    //         cc->turnAllocator->current(),
    //         cc->turnAllocator->next())
    return cc->securityPolicy->isLabelLEQ(
            cc->turnAllocator->current(),
            cc->turnAllocator->next()) ? 0 : WORST_CASE_DELAY;
}

int CommandQueueTP::MonotonicDeadTimeCalc::refresh_deadtime(){
    return cc->securityPolicy->isLabelLEQ(
            cc->turnAllocator->current(),
            cc->turnAllocator->next()) ? 0 : TP_BUFFER_TIME;
}

//=============================================================================
// Turn Ownership Decision
//=============================================================================

//-----------------------------------------------------------------------------
// TDM Turn Allocator
//-----------------------------------------------------------------------------
void CommandQueueTP::TDMTurnAllocator::allocate_turn(){}
void CommandQueueTP::TDMTurnAllocator::allocate_next(){}

unsigned CommandQueueTP::TDMTurnAllocator::current(){
    unsigned ccc_ = cc->currentClockCycle - cc->offset;
    unsigned schedule_time = ccc_ %
        (cc->p0Period + (cc->num_pids-1) * cc->p1Period);
    if( schedule_time < cc->p0Period ) return 0;
    return (schedule_time - cc->p0Period) / cc->p1Period + 1;
}

unsigned CommandQueueTP::TDMTurnAllocator::next(){
    unsigned next_in_schedule = CommandQueueTP::TDMTurnAllocator::current();
    if( next_in_schedule == cc->num_pids -1) return 0;
    return next_in_schedule+1;
}

//-----------------------------------------------------------------------------
// Preempting Turn Allocator
//-----------------------------------------------------------------------------
void CommandQueueTP::PreemptingTurnAllocator::allocate_turn(){
    turn_owner = next_owner;
    next_owner = TDMTurnAllocator::next();
}

unsigned CommandQueueTP::PreemptingTurnAllocator::next_nonempty(unsigned tcid){
    unsigned next = tcid;
    unsigned top = cc->securityPolicy->top();
    while(cc->tcidEmpty(next) && next!=top){
        next = cc->securityPolicy->nextHigherTC(tcid);
    }
    return next;
}

void CommandQueueTP::PreemptingTurnAllocator::allocate_next(){
    unsigned  nat_tcid = TDMTurnAllocator::next();
    next_owner = next_nonempty(nat_tcid);
    if(cc->tcidEmpty(nat_tcid)){
        PRINT("DONATED");
        (*(cc->incr_stat))(cc->donations,next_owner,1,NULL);
    } 
}

unsigned CommandQueueTP::PreemptingTurnAllocator::current(){ return turn_owner; }
unsigned CommandQueueTP::PreemptingTurnAllocator::next(){ return next_owner; }

//-----------------------------------------------------------------------------
// Priority Turn Allocator
//-----------------------------------------------------------------------------
CommandQueueTP::PriorityTurnAllocator::PriorityTurnAllocator(CommandQueueTP *cc)
    : TurnAllocator(cc)
{
    int num_pids = cc->num_pids;
    epoch_length = num_pids*(num_pids+1)/2;
    epoch_remaining = epoch_length;

    bandwidth_limit = ((int*) malloc(sizeof(int) * num_pids));
    // For now assume total order with 0 as bottom.
    bandwidth_limit[0] = num_pids;
    for(int i=1; i < num_pids; i++){
        bandwidth_limit[i] = bandwidth_limit[i-1] + num_pids - i;
    }

    bandwidth_remaining = ((int*) malloc(sizeof(int) * num_pids));
    for(int i=0; i < num_pids; i++) bandwidth_remaining[i]=bandwidth_limit[i];
    turn_owner = cc->securityPolicy->bottom();
    next_owner = cc->securityPolicy->bottom();
}

unsigned CommandQueueTP::PriorityTurnAllocator::highest_nonempty_wbw(){
    unsigned tcid_candidate = cc->securityPolicy->bottom();
    unsigned top = cc->securityPolicy->top();
    bool has_bw;
    bool is_empty;
    while(tcid_candidate !=top){
        has_bw = bandwidth_remaining[tcid_candidate];
        is_empty = cc->tcidEmpty(tcid_candidate);
        if(has_bw && !is_empty) break;
        tcid_candidate = cc->securityPolicy->nextHigherTC(tcid_candidate);
    }
    return tcid_candidate;
}

void CommandQueueTP::PriorityTurnAllocator::allocate_next(){
   // PRINT("-----------------------------------------------------------------------------");
   // PRINT("Priority Allocation time" << cc->currentClockCycle);
    int num_pids = cc->num_pids;
    //Reset bandwidth limits on an epoch change
    if(epoch_remaining == 0){
        epoch_remaining = epoch_length;
        for(int i=0; i<num_pids; i++){
            bandwidth_remaining[i] = bandwidth_limit[i];
        }
    }
    epoch_remaining -= 1;
    // PRINT("epoch remaining " << epoch_remaining);

    // for(int i=0; i<num_pids; i++)
    //     PRINT("bandwidth_remaining[" << i << "] " <<
    //            bandwidth_remaining[i])
    
    next_owner =  highest_nonempty_wbw();

    // Deduct bandwidth from the next owner and all those above it 
    for(int i=next_owner; i < num_pids; i++){
        bandwidth_remaining[i] -= 1;
    }
    //PRINT("-----------------------------------------------------------------------------");

}

void CommandQueueTP::PriorityTurnAllocator::allocate_turn(){
    turn_owner = next_owner;
    next_owner = cc->securityPolicy->bottom();
}

unsigned CommandQueueTP::PriorityTurnAllocator::current(){ return turn_owner; }
unsigned CommandQueueTP::PriorityTurnAllocator::next(){ return next_owner; }

//=============================================================================
// Lattice
//=============================================================================
unsigned CommandQueueTP::TOLattice::nextHigherTC(unsigned tcid){
    // if(tcid == num_pids - 1){
    //     return tcid;
    // } else {
    //     if(cc->tcidEmpty(tcid+1)) return nextHigherTC(tcid+1);
    //     else return tcid+1;
    // }
    if(tcid == num_pids -1) return tcid;
    else return tcid+1;
}

bool CommandQueueTP::TOLattice::isLabelLEQ(unsigned tc1, unsigned tc2){
    return tc1 <= tc2;
}

unsigned CommandQueueTP::TOLattice::bottom(){
    return 0;
}

unsigned CommandQueueTP::TOLattice::top(){
    return cc->num_pids - 1;
}
