/*
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Nathan Binkert
 */

#include <string>

#include "base/callback.hh"
#include "base/hostinfo.hh"
#include "sim/eventq.hh"
#include "sim/sim_events.hh"
#include "sim/sim_exit.hh"
#include "sim/stats.hh"
#include "sim/global_exit_count.hh"
#ifndef HAS_RESET
#include "sim/has_reset.cc"
#define HAS_RESET
#endif

using namespace std;
extern int term_cpu_val;
extern bool has_reset;

    SimLoopExitEvent::SimLoopExitEvent(const std::string &_cause, int c, Tick r)
: Event(Sim_Exit_Pri, IsExitEvent), cause(_cause), code(c), repeat(r)
{
}

//
// handle termination event
//
    void
SimLoopExitEvent::process()
{
    // if this got scheduled on a different queue (e.g. the committed
    // instruction queue) then make a corresponding event on the main
    // queue.
    if (!isFlagSet(IsMainQueue)) {
        exitSimLoop(cause, code);
        delete this;
    }

    // otherwise do nothing... the IsExitEvent flag takes care of
    // exiting the simulation loop and returning this object to Python

    // but if you are doing this on intervals, don't forget to make another
    if (repeat) {
        assert(isFlagSet(IsMainQueue));
        mainEventQueue.schedule(this, curTick() + repeat);
    }
}


const char *
SimLoopExitEvent::description() const
{
    return "simulation loop exit";
}

void
exitSimLoop(const std::string &message, int exit_code, Tick when, Tick repeat)
{
    // if( message.find("cpu0") == string::npos ) {
    //   cout << message << " @ " << curTick() << endl;
    //   return;
    // }
    
    Event *event = new SimLoopExitEvent(message, exit_code, repeat);
    mainEventQueue.schedule(event, when);
}

    CountedDrainEvent::CountedDrainEvent()
: count(0)
{ }

    void
CountedDrainEvent::process()
{
    if (--count == 0)
        exitSimLoop("Finished drain", 0);
}

//
// constructor: automatically schedules at specified time
//
    CountedExitEvent::CountedExitEvent(const std::string &_cause, int &counter,
        int reset_val)
: Event(Sim_Exit_Pri), cause(_cause), downCounter(counter), reset_val(reset_val)
{
    // catch stupid mistakes
    assert(downCounter > 0);
}

int cpu_match(const std::string &message){
    if(message.find("cpu0") != string::npos) return 0;
    else if(message.find("cpu1") != string::npos) return 1;
    else if(message.find("cpu2") != string::npos) return 2;
    else if(message.find("cpu3") != string::npos) return 3;
    else if(message.find("cpu4") != string::npos) return 4;
    else if(message.find("cpu5") != string::npos) return 5;
    else if(message.find("cpu6") != string::npos) return 6;
    else if(message.find("cpu7") != string::npos) return 7;
    return -1;
}

//
// handle termination event
//
void CountedExitEvent::process()
{
    term_cpu_val = cpu_match(cause);
    if(has_reset) Stats::dump();
    term_cpu_val = -1;
    if (--downCounter == 0) {
        downCounter = reset_val;
        has_reset = true;
        exitSimLoop(cause, 0);
    } else {
        cout << cause << " @ " << curTick() << endl;
    }
}

const char *
CountedExitEvent::description() const
{
    return "counted exit";
}
