#include "MemoryControllerInvPrio.h"
#ifndef ccip 
#include "CommandQueueInvPrio.h"
#define ccip
#endif

using namespace DRAMSim;

MemoryControllerInvPrio::MemoryControllerInvPrio(MemorySystem *parent, 
        CSVWriter &csvOut, ostream &dramsim_log, 
        const string &outputFilename,
        unsigned tpTurnLength,
        bool genTrace,
        const string &traceFilename,
        int num_pids,
        bool fixAddr,
        bool diffPeriod,
        int p0Period,
        int p1Period,
        int offset,
        int lattice_config_) :
  lattice_config(lattice_config_),
  MemoryControllerTP(
      parent,
      csvOut,
      dramsim_log,
      outputFilename,
      tpTurnLength,
      genTrace,
      traceFilename,
      num_pids,
      fixAddr,
      diffPeriod,
      p0Period,
      p1Period,
      offset)
{
    commandQueue = new CommandQueueInvPrio(bankStates,dramsim_log,tpTurnLength,num_pids, fixAddr, diffPeriod, p0Period, p1Period, offset, lattice_config_); 
}
