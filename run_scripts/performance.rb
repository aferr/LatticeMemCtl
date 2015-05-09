#------------------------------------------------------------------------------
# Performance Evaluation
#------------------------------------------------------------------------------
require_relative 'runscripts'
include RunScripts

module RunScripts

    $test_run_opts = {
        # maxinsts: 100,
        # max_memory_accesses: 100,
        # fastforward: 100,
        runmode: :local,
        debug: true
    }

    def synthetic
        o = {
            addrpar: true,
            scheme: "tp",
            workloads: {
                hardstride_nothing: %w[hardstride nothing],
                nothing_hardstride: %w[nothing hardstride],
            },
            maxinsts: 10**4,
            fastforward: 10**3,
            num_wl: 2,
            skip4: true,
            skip6: true,
            runmode: :local,
            debug: true,
        }

        # single
        single o.merge(
            scheme: "none",
            benchmarks: %w[nothing hardstride]
        )

        # baseline
        iterate_mp o.merge(
            scheme: "none",
        )

        #secure
        secure o
        
    end

    $some_workloads = {
      mcf_bz2: %w[ mcf bzip2 ],
      mcf_ast: %w[mcf astar],
      ast_mcf: %w[astar mcf],
    }

    def baseline
      iterate_mp(
        scheme: "none",
        num_wl: 8,
        skip4: true,
        skip6: true,
        skip2: true
      )
    end

    def single_core
      single()
    end
    
    def ncore_ntc
      iterate_mp $secure_opts.merge(
        num_wl: 2,
      )
    end

    def secure_spec
        secure(
            addrpar: true,
            scheme: "tp",
            num_wl: 8,
            skip2: true,
            skip4: true,
            skip6: true,
            rank_bank_partitioning: true,
            tl0: 18,
            tl1: 18,
            nametag: ""
        )
    end

    def secure o={}
        # TDM, strict, turn start
        iterate_mp o.merge(
            turn_allocation_policy: 0,
            turn_allocatrion_time: 0,
            dead_time_policy: 0,
            nametag: o[:nametag] + "tdm_strict_start"
        )

        # TDM, Monotonic, turn start
        iterate_mp o.merge(
            turn_allocation_policy: 0,
            turn_allocatrion_time: 0,
            dead_time_policy: 1,
            nametag: o[:nametag] + "tdm_monotonic_start"
        )
        
        # Preempting, Strict, turn start
        iterate_mp o.merge(
            turn_allocation_policy: 1,
            turn_allocation_time: 0,
            dead_time_policy: 0,
            nametag: o[:nametag] + "preempting_strict_start"
        )

        # Preempting,  Monotonic, Turn Start
        iterate_mp o.merge(
            turn_allocation_policy: 1,
            turn_allocation_time: 0,
            dead_time_policy: 1,
            nametag: o[:nametag] + "preempting_monotonic_start"
        )
         
        # Preempting,  Monotonic, Dead Time
        iterate_mp o.merge(
            turn_allocation_policy: 1,
            turn_allocation_time: 1,
            dead_time_policy: 1,
            nametag: o[:nametag] + "preempting_monotonic_dead"
        )

        # Priority, Strict, turn start
        iterate_mp o.merge(
            turn_allocation_policy: 2,
            turn_allocation_time: 0,
            dead_time_policy: 0,
            nametag: o[:nametag] + "priority_strict_start"
        )

        # Priority, Monotonic, turn start
        iterate_mp o.merge(
            turn_allocation_policy: 2,
            turn_allocation_time: 0,
            dead_time_policy: 1,
            nametag: o[:nametag] + "priority_monotonic_start"
        )

        # Priority, Monotonic, Dead Time
        iterate_mp o.merge(
            turn_allocation_policy: 2,
            turn_allocation_time: 1,
            dead_time_policy: 1,
            nametag: o[:nametag] + "priority_monotonic_dead"
        )
    end

end
