#include "CommandQueue.h"
#ifndef lattice
#include "../ARM/base/lattice.hh"
#define lattice
#endif

#define BLOCK_TIME 12

#include <map>

using namespace std;

namespace DRAMSim
{

class CommandQueueTP : public CommandQueue
{
    public:
        CommandQueueTP(vector< vector<BankState> > &states,
                ostream &dramsim_log_,unsigned tpTurnLength,
                int num_pids, bool fixAddr_,
                bool diffPeriod_, int p0Period_, int p1Period_, int offset_,
                map<int,int>* tp_config);

        virtual void enqueue(BusPacket *newBusPacket);
        virtual bool hasRoomFor(unsigned numberToEnqueue, unsigned rank, 
                unsigned bank, unsigned pid);
        virtual bool isEmpty(unsigned rank);
        int qsbytc_ignore_ref(unsigned tcid);
        int queueSizeByTcid(unsigned tcid);
        bool qsbytc_has_nonref(unsigned tcid);
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
        bool partitioning;

        int pid_last_pop;
        int last_pid;

        virtual unsigned getCurrentPID();
        virtual bool isBufferTime();
        virtual bool isBufferTimePure();
        virtual int normal_deadtime(int tlength);
        virtual int refresh_deadtime(int tlength);

        virtual int worst_case_time();
        virtual int refresh_worst_case_time();

        virtual void update_stats();

        // Tell the donor scheme child to check if an issue was allowed 
        // because of a donated turn
    
    bool is_turn_start(){
        unsigned ccc_ = currentClockCycle - offset;
        int schedule_time = ccc_ % (p0Period + (num_pids-1) * p1Period);
        return (schedule_time==0) || (((schedule_time - p0Period)%p1Period)==0);
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
        virtual void step() = 0;
    };

    class TurnStartAllocationTimer : public TurnAllocationTimer
    {
        public:
        TurnStartAllocationTimer(CommandQueueTP *cc) :
            TurnAllocationTimer(cc) {}
        virtual bool is_reallocation_time();
        virtual void step();
    };

    class DeadTimeAllocationTimer: public TurnAllocationTimer
    {
        public:
            DeadTimeAllocationTimer(CommandQueueTP *cc) :
                TurnAllocationTimer(cc) {}
        virtual bool is_reallocation_time();
        virtual void step();
    };
    
    TurnAllocationTimer *turnAllocationTimer;

    //-------------------------------------------------------------------------
    // Dead Time Calculators
    //-------------------------------------------------------------------------
    class DeadTimeCalc
    {
        public:
        CommandQueueTP *cc;
        DeadTimeCalc(CommandQueueTP *cc) : cc(cc) {}
        virtual int normal_deadtime() = 0;
        virtual int refresh_deadtime() = 0;
    };

    class StrictDeadTimeCalc : public DeadTimeCalc
    {
        public:
        StrictDeadTimeCalc(CommandQueueTP *cc) : DeadTimeCalc(cc) {}
        virtual int normal_deadtime();
        virtual int refresh_deadtime();
    };

    class MonotonicDeadTimeCalc : public DeadTimeCalc
    {
        public:
        MonotonicDeadTimeCalc(CommandQueueTP *cc) : DeadTimeCalc(cc) {}
        virtual int normal_deadtime();
        virtual int refresh_deadtime();
    };
    
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
        static unsigned natural_turn(CommandQueueTP *cc);
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
            TDMTurnAllocator(cc) {
                turn_owner = cc->securityPolicy->bottom();
                next_owner = cc->securityPolicy->bottom();
        }
        virtual void allocate_turn();
        virtual void allocate_next();
        virtual unsigned current();
        virtual unsigned next();
        private:
        unsigned next_nonempty(unsigned tcid);
        unsigned turn_owner;
        unsigned next_owner;
    };

    class PriorityTurnAllocator : public TurnAllocator
    {
        public:
        PriorityTurnAllocator(CommandQueueTP *cc);
        virtual void allocate_turn();
        virtual void allocate_next();
        virtual unsigned current();
        virtual unsigned next();

        private:
        unsigned highest_nonempty_wbw();
        int epoch_length;
        int epoch_remaining;
        int *bandwidth_limit;
        int *bandwidth_remaining;
        unsigned turn_owner;
        unsigned next_owner;
    };
   
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
        virtual unsigned top() = 0;
        virtual unsigned bottom() = 0;
    };

    class TOLattice : public Lattice
    {
        public: 
        TOLattice(CommandQueueTP* cc) : Lattice(cc) {}
        virtual unsigned nextHigherTC(unsigned tcid);
        virtual bool isLabelLEQ(unsigned tc1, unsigned tc2);
        virtual unsigned top();
        virtual unsigned bottom();

    };

    Lattice *securityPolicy;


};

} //namespace DRAMSim
