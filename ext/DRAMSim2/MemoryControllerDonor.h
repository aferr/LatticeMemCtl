#include "MemoryControllerMonotonic.h"

using namespace std;

namespace DRAMSim
{
    class MemoryControllerDonor : public MemoryControllerTP
    {
        public:
            MemoryControllerDonor(MemorySystem* ms, CSVWriter &csvOut_, 
                    ostream &dramsim_log_, 
                    const string &outputFilename_,
                    unsigned tpTurnLength_,
                    bool genTrace_,
                    const string &traceFilename_,
                    int num_pids_, bool fixAddr,
                    bool diffPeriod, int p0Period, int p1Period,
                    int offset, int lattice_config);

            int lattice_config;

    };
}
