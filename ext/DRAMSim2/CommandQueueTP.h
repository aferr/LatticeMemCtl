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
    
    bool is_turn_start(){
        unsigned ccc_ = currentClockCycle - offset;
        unsigned schedule_time = ccc_ %
            (p0Period + (num_pids-1) * p1Period);
        return schedule_time==0 || ((schedule_time - p0Period)%p1Period==0);
    }

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
        CommandQueueTP *cc;
        TurnAllocator(CommandQueueTP *cc) : cc(cc) {}
        virtual void allocate_turn() = 0;
        virtual void allocate_next() = 0;
        virtual unsigned current() = 0;
        virtual unsigned next() = 0;
    };

    class TDMTurnAllocator : public TurnAllocator
    {
        public:
        TDMTurnAllocator(CommandQueueTP *cc) :
            TurnAllocator(cc) {}
        virtual void allocate_turn();
        virtual void allocate_next();
        virtual unsigned current();
        virtual unsigned next();
    };

    class PreemptingTurnAllocator : public TDMTurnAllocator
    {
        public:
        PreemptingTurnAllocator(CommandQueueTP *cc) :
            TDMTurnAllocator(cc) {}
        virtual void allocate_turn();
        virtual void allocate_next();
        virtual unsigned current();
        virtual unsigned next();
        private:
        unsigned turn_owner;
        unsigned next_owner;
    };

    // class PriorityTurnOwner : public PriorityTurnOwner
    // {
    //     virtual unsigned getCurrentPID();
    //     virtual unsigned getNextPID();
    // }

    TurnAllocator *turnAllocator;

    //-------------------------------------------------------------------------
    // Lattice
    //-------------------------------------------------------------------------
    class Lattice
    {
        public:
        int num_pids;
        CommandQueueTP* cc;
        Lattice(CommandQueueTP* cc) : num_pids(cc->num_pids), cc(cc) {}
        virtual unsigned nextHigherTC(unsigned tcid) = 0;
        virtual bool isLabelLEQ(unsigned tc1, unsigned tc2) = 0;
    };

    class TOLattice : public Lattice
    {
        public: 
        TOLattice(CommandQueueTP* cc) : Lattice(cc) {}
        virtual unsigned nextHigherTC(unsigned tcid);
        virtual bool isLabelLEQ(unsigned tc1, unsigned tc2);
    };

    Lattice *securityPolicy;


};

} //namespace DRAMSim
