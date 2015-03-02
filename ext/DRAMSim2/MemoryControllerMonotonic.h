#include "MemoryControllerTPD.h"

using namespace std;

namespace DRAMSim
{
    class MemoryControllerMonotonic : public MemoryControllerTP
    {
        public:
            MemoryControllerMonotonic(MemorySystem* ms, CSVWriter &csvOut_, 
                    ostream &dramsim_log_, 
                    const string &outputFilename_,
                    unsigned tpTurnLength_,
                    bool genTrace_,
                    const string &traceFilename_,
                    int num_pids_, bool fixAddr,
                    bool diffPeriod, int p0Period, int p1Period,
                    int offset);

    };
}
