# Copyright (c) 2012 ARM Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2006-2008 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Steve Reinhardt

# Simple test script
#
# "m5 test.py"

import optparse
import sys

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.util import addToPath, fatal

addToPath('../common')
addToPath('../ruby')
addToPath('../topologies')

import Options
import Ruby
import Simulation
import CacheConfig
from Caches import *
from cpu2000 import *

parser = optparse.OptionParser()
Options.addCommonOptions(parser)
Options.addSEOptions(parser)
######################################################################
# custom options, help yourself to add any options for the convenience

### DRAMSim2
parser.add_option("--dramsim2", action="store_true")
if '--dramsim2' in sys.argv:
    parser.add_option("--devicecfg", type="string", default="",
            help="device configuration file to be used by DRAMSim2")
    parser.add_option("--systemcfg", type="string", default="", 
            help="system configuration file to be used by DRAMSim2")
    parser.add_option("--tpturnlength", type="string", default="12",
            help="Turn length for TP. Unused if another scheme is used.")
    parser.add_option("--outputfile", type="string", default="",
            help="output file for DRAMSim results."),
    parser.add_option("--p0", type="string", 
            help="workload for processor 0, usually the victim."),
    parser.add_option("--p1", type="string",
            help="workload for processor 1, usually the attacker.")
    parser.add_option("--gentrace", action="store_true", default=False,
            help="generate the trace for benchmarks.")
    parser.add_option("--tracefile", type="string", default="",
            help="trace file to be generated.")
    parser.add_option("--processid", type="int", default=0,
            help="the process pid")
######################################################################

######################################################################
# custom function

######################################################################

if '--ruby' in sys.argv:
    Ruby.define_options(parser)

(options, args) = parser.parse_args()

if args:
    print "Error: script doesn't take any positional arguments"
    sys.exit(1)

# Number of CPUs
options.num_cpus = 1

######################################################################
# Add DRAMSim2 into the system
### set memory capacity
np = options.num_cpus

memorysize = '4096MB'

### instantiate the DRAMSim2 model if we enable it
if options.dramsim2 :
    DRAM = DRAMSim2(    # memory capacity
                        range = AddrRange(memorysize),
                        # cpu clock to do synchronization
                        cpu_clock=options.clock,
                        # device (timing and power) configure file
                        deviceConfigFile = options.devicecfg,
                        # system (channel number, scheduling policy) configure file
                        systemConfigFile=options.systemcfg,
                        # output file for DRAMSim results
                        outputFile=options.outputfile,
                        #TP Turn Length
                        tpTurnLength=options.tpturnlength,
                        #Generate trace
                        genTrace=options.gentrace,
                        #Trace file 
                        traceFile=options.tracefile,
			#One CPU
			numPids=1
                    );
else: # or we just use the original memory model
    DRAM = SimpleMemory( range = AddrRange(memorysize) )
######################################################################

                  
multiprocesses = []
apps = []

if options.bench:
    apps = options.bench.split("-")
    if len(apps) != options.num_cpus:
        print "number of benchmarks not equal to set num_cpus!"
        sys.exit(1)

    for app in apps:
        try:
            if buildEnv['TARGET_ISA'] == 'alpha':
                exec("workload = %s('alpha', 'tru64', 'ref')" % app)
            else:
                exec("workload = %s(buildEnv['TARGET_ISA'], 'linux', 'ref')" % app)
            multiprocesses.append(workload.makeLiveProcess())
        except:
            print >>sys.stderr, "Unable to find workload for %s: %s" % (buildEnv['TARGET_ISA'], app)
            sys.exit(1)
elif options.cmd:
    process = LiveProcess()
    process.executable = options.cmd
    process.cmd = [options.cmd] + options.options.split()
    multiprocesses.append(process)

process0 = LiveProcess()
#process0.executable = "./tests/test-progs/test/arm/victim"
#process0.executable = options.p0
process0.cmd = options.p0.split()
process0.pid = options.processid
multiprocesses.append(process0)

#if len(multiprocesses) == 0:
#    print >> sys.stderr, "No workload specified. Exiting!\n"
#    sys.exit(1)

if options.input != "":
    process.input = options.input
if options.output != "":
    process.output = options.output
if options.errout != "":
    process.errout = options.errout


# By default, set workload to path of user-specified binary
workloads = options.cmd
numThreads = 1

if options.cpu_type == "detailed" or options.cpu_type == "inorder":
    #check for SMT workload
    workloads = options.cmd.split(';')
    if len(workloads) > 1:
        process = []
        smt_idx = 0
        inputs = []
        outputs = []
        errouts = []

        if options.input != "":
            inputs = options.input.split(';')
        if options.output != "":
            outputs = options.output.split(';')
        if options.errout != "":
            errouts = options.errout.split(';')

        for wrkld in workloads:
            smt_process = LiveProcess()
            smt_process.executable = wrkld
            smt_process.cmd = wrkld + " " + options.options
            if inputs and inputs[smt_idx]:
                smt_process.input = inputs[smt_idx]
            if outputs and outputs[smt_idx]:
                smt_process.output = outputs[smt_idx]
            if errouts and errouts[smt_idx]:
                smt_process.errout = errouts[smt_idx]
            process += [smt_process, ]
            smt_idx += 1
    numThreads = len(workloads)

(CPUClass, test_mem_mode, FutureClass) = Simulation.setCPUClass(options)
CPUClass.clock = options.clock
CPUClass.numThreads = numThreads;

system = System(cpu = [CPUClass(cpu_id=i) for i in xrange(np)],
                physmem = DRAM,
                membus = CoherentBus(), mem_mode = test_mem_mode, numPids = 1)

# Sanity check
if options.fastmem and (options.caches or options.l2cache):
    fatal("You cannot use fastmem in combination with caches!")

for i in xrange(np):
    if len(multiprocesses) == 1:
        system.cpu[i].workload = multiprocesses[0]
    else:
        system.cpu[i].workload = multiprocesses[i]

    if options.fastmem:
        system.cpu[i].fastmem = True

    if options.checker:
        system.cpu[i].addCheckerCpu()

if options.ruby:
    if not (options.cpu_type == "detailed" or options.cpu_type == "timing"):
        print >> sys.stderr, "Ruby requires TimingSimpleCPU or O3CPU!!"
        sys.exit(1)

    options.use_map = True
    Ruby.create_system(options, system)
    assert(options.num_cpus == len(system.ruby._cpu_ruby_ports))

    for i in xrange(np):
        ruby_port = system.ruby._cpu_ruby_ports[i]

        # Create the interrupt controller and connect its ports to Ruby
        system.cpu[i].createInterruptController()
        system.cpu[i].interrupts.pio = ruby_port.master
        system.cpu[i].interrupts.int_master = ruby_port.slave
        system.cpu[i].interrupts.int_slave = ruby_port.master

        # Connect the cpu's cache ports to Ruby
        system.cpu[i].icache_port = ruby_port.slave
        system.cpu[i].dcache_port = ruby_port.slave
else:
    system.system_port = system.membus.slave
    system.physmem.port = system.membus.master
    CacheConfig.config_cache(options, system)

root = Root(full_system = False, system = system)
Simulation.run(options, root, system, FutureClass, 1)
