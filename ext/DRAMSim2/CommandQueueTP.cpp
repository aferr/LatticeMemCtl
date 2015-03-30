#include "CommandQueueTP.h"

using namespace DRAMSim;

CommandQueueTP::CommandQueueTP(vector< vector<BankState> > &states, 
        ostream &dramsim_log_, unsigned tpTurnLength_,
        int num_pids_, bool fixAddr_,
        bool diffPeriod_, int p0Period_, int p1Period_, int offset_):
    CommandQueue(states,dramsim_log_,num_pids_)
{
    fixAddr = fixAddr_;
    tpTurnLength = tpTurnLength_;
    diffPeriod = diffPeriod_;
    p0Period = p0Period_;
    p1Period = p1Period_;
	offset = offset_;
#ifdef DEBUG_TP
    cout << "TP Debugging is on." <<endl;
#endif
}

void
CommandQueueTP::step(){
    SimulatorObject::step();
  
   // If the active SD has no requests, but pid 0 does, this a queueing delay 
   // cycle that affects the memory latency.
   if( isEmpty(getCurrentPID()) && !isEmpty(0) && getCurrentPID()!=0 ){ 
       (*incr_stat)(queueing_delay_cycles,0,NULL,NULL);
   }

}

int CommandQueueTP::normal_deadtime(int tlength){
  int ret = tlength - (tlength - WORST_CASE_DELAY)/10;
  return ret;
}

int CommandQueueTP::refresh_deadtime(int tlength){
  return tlength - (tlength - TP_BUFFER_TIME)/10;
}

void CommandQueueTP::enqueue(BusPacket *newBusPacket)
{
    unsigned rank = newBusPacket->rank;
    unsigned pid = newBusPacket->threadID;
    queues[rank][pid].push_back(newBusPacket);
#ifdef DEBUG_TP
    if(newBusPacket->physicalAddress == interesting)
        cout << "Enqueued interesting @ "<< currentClockCycle <<endl;
#endif /*DEBUG_TP*/
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
#ifdef DEBUG_TP
            cout << "TooEarly because row is active with pid " << getCurrentPID()
                << " at time " << currentClockCycle <<endl;
            bankStates[refreshRank][b].print();
            print();
#endif /*DEBUG_TP*/
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
#ifdef DEBUG_TP
            cout << "TooEarly because nextActivate is "
                <<bankStates[refreshRank][b].nextActivate
                << " at time " << currentClockCycle <<endl;
#endif /*DEBUG_TP*/
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
                dramsim_log);
#ifdef DEBUG_TP
        // PRINTN("Refresh at " << currentClockCycle << " for rank " 
        //         << refreshRank << endl);
#endif /*DEBUG_TP*/
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
    if(lastPID!=getCurrentPID()){
        //if the turn changes, reset the nextRank, nextBank, and
        //starters. It seems to have no effect on interference.
        nextRank = nextBank =0;
        startingRank = nextRank;
        startingBank = nextBank;
#ifdef DEBUG_TP
        if( hasInteresting() ){
        cout << endl << "==========================================="<<endl;
        cout << "Starting turn of length 2**"<<tpTurnLength<<" with PID "<<
            getCurrentPID() <<" at cycle "<< currentClockCycle << endl;
        cout << endl;
        print();
        }
#endif /*DEBUG_TP*/
    }
    lastPID = getCurrentPID();

    while(true)
    {
        //Only get the queue for the PID with the current turn.
        vector<BusPacket *> &queue = getCommandQueue(nextRank, getCurrentPID());
        //make sure there is something in this queue first
        //	also make sure a rank isn't waiting for a refresh
        //	if a rank is waiting for a refesh, don't issue anything to it until 
        //	the
        //		refresh logic above has sent one out (ie, letting banks close)

#ifdef DEBUG_TP
        if(hasInteresting()){
            printf("nextRank %u refreshRank %u currentPID %u\n",nextRank,refreshRank,getCurrentPID());
        }
#endif /*DEBUG_TP*/

        if (!queue.empty() && !((nextRank == refreshRank) && refreshWaiting))
        {

            //search from beginning to find first issuable bus packet
            for (size_t i=0;i<queue.size();i++)
            {

                if (isIssuable(queue[i]))
                {
#ifdef DEBUG_TP
                    if(lastPopTime!=currentClockCycle &&
                            queue[i]->physicalAddress == interesting){
                        string bptype = (queue[i]->busPacketType==ACTIVATE) ?
                            "activate" : "r/w";
                        cout << "popped interesting "<< bptype << " @ "
                            << currentClockCycle << endl;
                        lastPopTime = currentClockCycle;
                    }
#endif /*DEBUG_TP*/
                    //If a turn change is about to happen, don't
                    //issue any activates
                    
                    if(queue[i]->busPacketType==ACTIVATE){
                        monotonic_check_deadtime();
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
					//cout << "popped " << queue[i]->physicalAddress << " @ " << currentClockCycle << endl;

                    queue.erase(queue.begin()+i);
                    foundIssuable = true;
                    break;
                }
#ifdef DEBUG_TP
                else if( queue[i]->physicalAddress==interesting
                        && lastPopTime!= currentClockCycle )
                {
                    string bptype = (queue[i]->busPacketType==ACTIVATE) ?
                        "activate" : "r/w";
                    cout << "interesting couldn't issue @ "<<
                        currentClockCycle << " as a "<<bptype <<endl;
                    cout << "nextRank "<<nextRank<< " nextBank "<<nextBank
                        << endl << "startingRank "<<startingRank
                        <<" startingBank " << startingBank << endl;
                    printf("refreshRank %u\n",refreshRank);
                    bankStates[queue[i]->rank][queue[i]->bank].print();
                    print();
                    lastPopTime = currentClockCycle;
                }
#endif /*DEBUG_TP*/
            }
        }
#ifdef DEBUG_TP 
        // else if(hasInteresting() && refreshWaiting && nextRank ==refreshRank){
        //     PRINTN("Blocked by refreshRank at "<<currentClockCycle<<
        //              " with turn "<<currentPID<<endl);
        // }
#endif /*DEBUG_TP*/

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
  unsigned ccc_ = currentClockCycle - offset;
  unsigned schedule_time = ccc_ % (p0Period + (num_pids-1) * p1Period);
  if( schedule_time < p0Period ) return 0;
  return (schedule_time - p0Period) / p1Period + 1;
}

bool CommandQueueTP::isBufferTime(){
  unsigned ccc_ = currentClockCycle - offset;
  unsigned current_tc = getCurrentPID();
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

  //TODO It returns a bool you tool
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

  //TODO It returns a bool you tool
  unsigned deadtime = (turn_start <= next_refresh && next_refresh < turn_end) ?
    CommandQueueTP::refresh_deadtime( tlength ) :
    CommandQueueTP::normal_deadtime( tlength );

  return ccc_ >= (turn_end - deadtime);
}

#ifdef DEBUG_TP
bool CommandQueueTP::hasInteresting(){
    vector<BusPacket *> &queue = getCommandQueue(nextRank, 1);
    for(size_t i=0; i<queue.size(); i++){
        if (queue[i]->physicalAddress == interesting)
            return true;
    }
    return false;
}
#endif /*DEBUG_TP*/

