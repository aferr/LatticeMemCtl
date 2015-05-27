using namespace std;
namespace DRAMSim{
class CommandQueueStats
    {
        public:
        void * queueing_delay;
        void * tmux_overhead;
        void * wasted_tmux_overhead;
        void * dead_time_overhead;

        void * dropped;
        void * total_turns;

        void * donations;
        void * steals;
        void * donation_overhead;

    };

}
