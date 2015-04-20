#include "CommandQueue.h"
#ifndef lattice
#include "../ARM/base/lattice.hh"
#define lattice
#endif

#define BLOCK_TIME 12

using namespace std;

namespace DRAMSim
{

class CommandQueueTP : public CommandQueue
{
    public:
        CommandQueueTP(vector< vector<BankState> > &states,
                ostream &dramsim_log_,unsigned tpTurnLength,
                int num_pids, bool fixAddr_,
                bool diffPeriod_, int p0Period_, int p1Period_, int offset_);
        virtual void enqueue(BusPacket *newBusPacket);
        virtual bool hasRoomFor(unsigned numberToEnqueue, unsigned rank, 
                unsigned bank, unsigned pid);
        virtual bool isEmpty(unsigned rank);
        int queueSizeByTcid(unsigned tcid);
        virtual bool tcidEmpty(int tcid);
        virtual vector<BusPacket *> &getCommandQueue(unsigned rank, 
                unsigned pid);
        virtual void step();
        virtual void print();

    protected:
        virtual void refreshPopClosePage(BusPacket **busPacket, bool & sendingREF);
        virtual bool normalPopClosePage(BusPacket **busPacket, bool & sendingREF);

        unsigned lastPID;
        unsigned tpTurnLength;
        unsigned lastPopTime;
        bool fixAddr;
        bool diffPeriod;
        int p0Period;
        int p1Period;
		int offset;

        virtual unsigned getCurrentPID();
        virtual bool isBufferTime();
        virtual bool isBufferTimePure();
        virtual int normal_deadtime(int tlength);
        virtual int refresh_deadtime(int tlength);

        virtual void update_stats();

        // Tell the donor scheme child to check if an issue was allowed 
        // because of a donated turn
    
    private:

    //-------------------------------------------------------------------------
    // Turn Ownership Timer
    //-------------------------------------------------------------------------
    class TurnAllocationTimer
    {
        public:
        CommandQueueTP *cc;
        TurnAllocationTimer(CommandQueueTP *cc) : cc(cc) {}
        virtual bool is_reallocation_time() = 0;
    };

    class TurnStartAllocationTimer : public TurnAllocationTimer
    {
        public:
        TurnStartAllocationTimer(CommandQueueTP *cc) :
            TurnAllocationTimer(cc) {}
        virtual bool is_reallocation_time();
    };

    // class DeadTimeAllocator : public TurnAllocationTimer
    // {
    //     virtual bool is_reallocation_time();
    // }
    
    TurnAllocationTimer *turnAllocationTimer;

    //-------------------------------------------------------------------------
    // Dead Time Calculators
    //-------------------------------------------------------------------------
    class DeadTimeCalc
    {
        public:
        virtual int normal_deadtime() = 0;
        virtual int refresh_deadtime() = 0;
    };

    class StrictDeadTimeCalc : public DeadTimeCalc
    {
        virtual int normal_deadtime();
        virtual int refresh_deadtime();
    };

    // class MonotonicDeadTimeCalc : public MonotonicDeadTimeCalc
    // {
    //     virtual int normal_deadtime();
    //     virtual int refresh_deadtime();
    // }
    
    DeadTimeCalc *deadTimeCalc;

    //-------------------------------------------------------------------------
    // Turn Ownership Decider
    //-------------------------------------------------------------------------
    class TurnAllocator
    {
        public:
        unsigned current_tc;
        CommandQueueTP *cc;
        TurnAllocator(CommandQueueTP *cc) : cc(cc) {}
        virtual unsigned allocate_turn() = 0;
    };

    class TDMTurnAllocator : public TurnAllocator
    {
        public:
        TDMTurnAllocator(CommandQueueTP *cc) :
            TurnAllocator(cc) {}
        virtual unsigned allocate_turn();
    };

    //  class PreemptingTurnOwner : public TDMTurnOwner
    //  {
    //      virtual unsigned getCurrentPID();
    //      virtual unsigned getNextPID();
    //  }

    //  class PriorityTurnOwner : public PriorityTurnOwner
    //  {
    //      virtual unsigned getCurrentPID();
    //      virtual unsigned getNextPID();
    //  }

    TurnAllocator *turnAllocator;
    unsigned current_tc;

};

} //namespace DRAMSim
