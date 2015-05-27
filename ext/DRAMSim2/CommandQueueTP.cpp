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

    map<int,int> tp_config = *tp_config_;
    securityPolicy = new TOLattice(this);
    switch(tp_config[0]){
        case 0: securityPolicy = new TOLattice(this); break;
        case 1:
                securityPolicy = new DiamondLattice(this);
                if(num_pids!=4){
                    fprintf(stderr, "diamond only supports 4SDs\n");
                    exit(1);
                }
                break;
        default: securityPolicy = new TOLattice(this); break;
    }

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

    switch(tp_config[4]){
        case 0: partitioning = false; break;
        case 1: partitioning = true; break;
        default: partitioning = false;
    }

    pid_last_pop = 0;
    last_pid = 0;
}

void
CommandQueueTP::step(){
   SimulatorObject::step();
   turnAllocationTimer->step();
   update_stats();
}

void
CommandQueueTP::update_stats(){
   #ifdef debug_cctp
   PRINT("-----------------------------------------------------------------------------");
   PRINT("Time" <<  currentClockCycle);
   PRINT("Current pid" << getCurrentPID());
   if(isBufferTime()) PRINT("It is buffer time\n");
   print();
   PRINT("-----------------------------------------------------------------------------");
   PRINT("");
   PRINT("");
   #endif

   for(int i=0; i < num_pids; i++){
       if( !tcidEmpty(i) && getCurrentPID()!=i ){
           (*incr_stat)(stats->tmux_overhead,i,
                   qsbytc_ignore_ref(i),NULL);
           if( tcidEmpty(getCurrentPID()) ){
               (*incr_stat)(stats->wasted_tmux_overhead,i,
                       qsbytc_ignore_ref(i),NULL);
           }
       }
   }

   if(isBufferTime() && qsbytc_has_nonref(getCurrentPID())){
       (*incr_stat)(stats->dead_time_overhead, getCurrentPID(),
               qsbytc_ignore_ref(getCurrentPID()),NULL);
   }

    for(int i=0; i < num_pids; i++){
        if( qsbytc_has_nonref(i) && getCurrentPID()!=i &&
            !qsbytc_has_nonref(getCurrentPID()) &&
            qsbytc_has_nonref(TDMTurnAllocator::natural_turn(this)) ){
                    (*incr_stat)(stats->donation_overhead,i,
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
#ifdef debug_cctp
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
        for(int j=0; j<q.size(); j++){
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

    if(pid_last_pop!= getCurrentPID()){
        last_pid = pid_last_pop;
    }

    pid_last_pop = getCurrentPID();

    while(true)
    {
        //Only get the queue for the PID with the current turn.
        vector<BusPacket *> &queue = getCommandQueue(nextRank, getCurrentPID());
        vector<BusPacket *> &queue_last = getCommandQueue(nextRank, last_pid);

        if (partitioning &&!((nextRank == refreshRank) && refreshWaiting) &&
                !queue_last.empty())
        {
            //search from beginning to find first issuable bus packet
            for (size_t i=0; i<queue_last.size(); i++)
            {

                if (isIssuable(queue_last[i]))
                {
                    if(queue_last[i]->busPacketType==ACTIVATE){
                        continue;
                    }
                   
                    // Make sure a read/write that hasn't been activated yet 
                    // isn't removed. 
                    if (i>0 && queue_last[i-1]->busPacketType==ACTIVATE &&
                            queue_last[i-1]->physicalAddress == 
                            queue_last[i]->physicalAddress){
                        continue;
                    }

                    *busPacket = queue_last[i];

                    queue_last.erase(queue_last.begin()+i);
                    (*queue_last.begin())->beginHeadTime = currentClockCycle;
                    foundIssuable = true;
                    break;
                }
            }
        }
        
        if(!(partitioning && foundIssuable)){
            if (!queue.empty() && !((nextRank == refreshRank) && refreshWaiting))
            {

                //search from beginning to find first issuable bus packet
                for (size_t i=0;i<queue.size();i++)
                {

                    if (isIssuable(queue[i]))
                    {
                        if(queue[i]->busPacketType==ACTIVATE){
                            if(isBufferTime()) continue;
                        }

                        //check to make sure we aren't removing a read/write that 
                        //is paired with an activate
                        if (i>0 && queue[i-1]->busPacketType==ACTIVATE &&
                                queue[i-1]->physicalAddress == 
                                queue[i]->physicalAddress){
                            continue;
                        }

                        *busPacket = queue[i];

                        queue.erase(queue.begin()+i);
                        (*queue.begin())->beginHeadTime = currentClockCycle;
                        foundIssuable = true;
                        break;
                    }
                }
            }
        }

        //if we found something, break out of do-while
        if (foundIssuable){
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
  unsigned current_tc = (TDMTurnAllocator::natural_turn(this));
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
       if( cc->securityPolicy->isLabelLEQ( 
                   cc->turnAllocator->current(),
                   cc->turnAllocator->next()) ){
           (*(cc->incr_stat))(cc->stats->dropped,cc->turnAllocator->current(),1,0);
       }
       (*(cc->incr_stat))(cc->stats->total_turns,cc->turnAllocator->current(),1,0);
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
    unsigned current_tc = TDMTurnAllocator::natural_turn(cc);
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
        cc->refresh_worst_case_time():
        cc->worst_case_time();

    return ccc_ == (turn_end - deadtime);
}

void CommandQueueTP::DeadTimeAllocationTimer::step(){
    if(cc->is_turn_start()){
        cc->turnAllocator->allocate_turn();
        (*(cc->incr_stat))(cc->stats->total_turns,cc->turnAllocator->current(),1,0);
    }
    
    unsigned ccc_ = cc->currentClockCycle - cc->offset;
    unsigned current_tc = TDMTurnAllocator::natural_turn(cc);
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
        cc->refresh_worst_case_time():
        cc->worst_case_time();

    if( ccc_ == (turn_end - deadtime -1)){
         cc->turnAllocator->allocate_next();

         if( cc->securityPolicy->isLabelLEQ( 
                     cc->turnAllocator->current(),
                     cc->turnAllocator->next()) ){
             (*(cc->incr_stat))(cc->stats->dropped,cc->turnAllocator->current(),1,0);
        }
    }
    
}


//=============================================================================
// Partitioning
//=============================================================================
int CommandQueueTP::worst_case_time(){
    if(partitioning){
        return FIX_WORST_CASE_DELAY;
    } else {
        return WORST_CASE_DELAY;
    }
}

int CommandQueueTP::refresh_worst_case_time(){
    if(partitioning){
        return FIX_TP_BUFFER_TIME;
    } else {
        return TP_BUFFER_TIME;
    }
}

//=============================================================================
// Dead Time Calculators
//=============================================================================

//-----------------------------------------------------------------------------
// Strict
//-----------------------------------------------------------------------------
int CommandQueueTP::StrictDeadTimeCalc::normal_deadtime(){
    return cc->worst_case_time();
}

int CommandQueueTP::StrictDeadTimeCalc::refresh_deadtime(){
    return cc->refresh_worst_case_time();
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
            cc->turnAllocator->next()) ? 0 : cc->worst_case_time();
}

int CommandQueueTP::MonotonicDeadTimeCalc::refresh_deadtime(){
    return cc->securityPolicy->isLabelLEQ(
            cc->turnAllocator->current(),
            cc->turnAllocator->next()) ? 0 : cc->refresh_worst_case_time();
}

//=============================================================================
// Turn Ownership Decision
//=============================================================================

//-----------------------------------------------------------------------------
// TDM Turn Allocator
//-----------------------------------------------------------------------------
unsigned CommandQueueTP::TDMTurnAllocator::natural_turn(CommandQueueTP* cc){
    unsigned ccc_ = cc->currentClockCycle - cc->offset;
    unsigned schedule_time = ccc_ %
        (cc->p0Period + (cc->num_pids-1) * cc->p1Period);
    if( schedule_time < cc->p0Period ) return 0;
    return (schedule_time - cc->p0Period) / cc->p1Period + 1;
}
void CommandQueueTP::TDMTurnAllocator::allocate_turn(){}
void CommandQueueTP::TDMTurnAllocator::allocate_next(){}

unsigned CommandQueueTP::TDMTurnAllocator::current(){
    return natural_turn(cc);
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
        next = cc->securityPolicy->nextHigherTC(next);
    }
    return next;
}

void CommandQueueTP::PreemptingTurnAllocator::allocate_next(){
    unsigned  nat_tcid = TDMTurnAllocator::next();
    next_owner = next_nonempty(nat_tcid);
    if(cc->tcidEmpty(nat_tcid)){
        (*(cc->incr_stat))(cc->stats->donations,nat_tcid,1,NULL);
        (*(cc->incr_stat))(cc->stats->steals,next_owner,1,NULL);
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
    epoch_length = num_pids;
    epoch_remaining = epoch_length;

    bandwidth_minimum= ((int*) malloc(sizeof(int) * num_pids));
    bandwidth_minimum[0] = 0;
    for(int i=1; i<num_pids; i++){
        bandwidth_minimum[i] = 1;
    }
    block_time = ((int*) malloc(sizeof(int) * num_pids));

    reset_epoch();

    turn_owner = cc->securityPolicy->bottom();
    next_owner = cc->securityPolicy->bottom();
}

void CommandQueueTP::PriorityTurnAllocator::reset_epoch(){
    epoch_remaining = epoch_length;
    for(int i=0; i < cc->num_pids; i++){
        block_time[i] = 0;
        for(int j=0; j < cc->num_pids; j++){
            if(cc->securityPolicy->isLabelLEQ(i,j)
                && !(cc->securityPolicy->isLabelLEQ(j,i)) )
                block_time[i] += bandwidth_minimum[j];
        }
    }
}

unsigned CommandQueueTP::PriorityTurnAllocator::highest_nonempty_wbw(){
    unsigned tcid_candidate = cc->securityPolicy->bottom();
    unsigned top = cc->securityPolicy->top();
    while(tcid_candidate !=top){
        // has_bw = bandwidth_remaining[tcid_candidate];
        bool has_bw = epoch_remaining > block_time[tcid_candidate];
        bool is_empty = cc->tcidEmpty(tcid_candidate);
        if(has_bw && !is_empty) break;
        block_time[tcid_candidate] -= 1;
        tcid_candidate = cc->securityPolicy->nextHigherTC(tcid_candidate);
    }
    return tcid_candidate;
}

void CommandQueueTP::PriorityTurnAllocator::allocate_next(){
#ifdef debug_cctp
    PRINT("-----------------------------------------------------------------------------");
    PRINT("Priority Allocation time" << cc->currentClockCycle);
#endif
    //Reset bandwidth limits on an epoch change
    if(epoch_remaining == 0) reset_epoch();
    epoch_remaining -= 1;
    
#ifdef debug_cctp
    PRINT("epoch remaining " << epoch_remaining);
#endif

    
    next_owner =  highest_nonempty_wbw();

#ifdef debug_cctp
    PRINT("next_owner" << next_owner);
    for(int i=0; i<cc->num_pids; i++)
        PRINT("block_time[" << i << "] " << block_time[i])

    PRINT("-----------------------------------------------------------------------------");
#endif

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

//-----------------------------------------------------------------------------
// Totally Ordered Lattice
//-----------------------------------------------------------------------------
unsigned CommandQueueTP::TOLattice::nextHigherTC(unsigned tcid){
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

//-----------------------------------------------------------------------------
// Diamond Lattice
//-----------------------------------------------------------------------------
unsigned CommandQueueTP::DiamondLattice::nextHigherTC(unsigned tcid){
    if(tcid == num_pids -1) {
        return tcid;
    } else if(tcid == 1 || tcid == 2){
        return num_pids - 1;
    } else{
        if(next_incomp == 1){
            next_incomp = 2;
            return 1;
        } else {
            next_incomp = 1;
            return 2;
        }
    }
}

bool CommandQueueTP::DiamondLattice::isLabelLEQ(unsigned tc1, unsigned tc2){
    if(tc1 == 0) return true;
    if(tc2 == 3) return true;
    if(tc1 == tc2) return true;
    return false;
}

unsigned CommandQueueTP::DiamondLattice::top(){ return 3; }
unsigned CommandQueueTP::DiamondLattice::bottom(){ return 0; }
