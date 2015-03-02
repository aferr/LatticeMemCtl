#include "CommandQueueFA.h"

using namespace std;

namespace DRAMSim
{
    class CommandQueueDonor : public CommandQueueTP
    {
        public:
            CommandQueueDonor(vector< vector<BankState> > &states,
                    ostream &dramsim_log_, int num_pids);
            virtual void step();
    };
}
