#include "CommandQueue.h"

using namespace std;
namespace DRAMSim
{
    class SecureCommandQueue : public CommandQueue
    {
        public:
        SecureCommandQueue(vector< vector<BankState> > &states,
                ostream &dramsim_log_,unsigned tpTurnLength,
                int num_pids, bool fixAddr_,
                bool diffPeriod_, int p0Period_, int p1Period_, int offset_);


        
    };
}
