#------------------------------------------------------------------------------
# Performance Evaluation
#------------------------------------------------------------------------------
require_relative 'runscripts'
include RunScripts

module RunScripts

    def synthetic
        o = {
            addrpar: true,
            scheme: "tp",
            workloads: {
                nothing_hardstride: %w[nothing hardstride],
                hardstride_nothing: %w[hardstride nothing],
            },
            maxinsts: 10**5,
            fastforward: 10**3 ,
            num_wl: 8,
            skip4: true,
            skip6: true,
            runmode: :local,
            debug: true
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

        # TDM, strict, turn start
        iterate_mp o.merge(
            turn_allocation_policy: 0,
            turn_allocatrion_time: 0,
            dead_time_policy: 0,
            nametag: "tdm_strict_start"
        )

        # # TDM, Monotonic, turn start
        iterate_mp o.merge(
            turn_allocation_policy: 0,
            turn_allocatrion_time: 0,
            dead_time_policy: 1,
            nametag: "tdm_monotonic_start"
        )
        
        # Preempting, Strict, turn start
        iterate_mp o.merge(
            turn_allocation_policy: 1,
            turn_allocation_time: 0,
            dead_time_policy: 0,
            nametag: "preempting_strict_start"
        )

        #Preempting,  Monotonic, Turn Start
        iterate_mp o.merge(
            turn_allocation_policy: 1,
            turn_allocation_time: 0,
            dead_time_policy: 1,
            nametag: "preempting_monotonic_start"
        )
        
        #Preempting,  Monotonic, Dead Time
        iterate_mp o.merge(
            turn_allocation_policy: 1,
            turn_allocation_time: 1,
            dead_time_policy: 1,
            nametag: "preempting_monotonic_dead"
        )

        # Priority, Strict, turn start
        iterate_mp o.merge(
            turn_allocation_policy: 2,
            turn_allocation_time: 0,
            dead_time_policy: 0,
            nametag: "priority_strict_start"
        )

        # Priority, Monotonic, turn start
        iterate_mp o.merge(
            turn_allocation_policy: 2,
            turn_allocation_time: 0,
            dead_time_policy: 1,
            nametag: "priority_monotonic_start"
        )

        # Priority, Monotonic, Dead Time
        iterate_mp o.merge(
            turn_allocation_policy: 2,
            turn_allocation_time: 0,
            dead_time_policy: 1,
            nametag: "priority_monotonic_dead"
        )
        
    end

    $some_workloads = {
      mcf_bz2: %w[ mcf bzip2 ],
      mcf_ast: %w[mcf astar],
      ast_mcf: %w[astar mcf],
    }

    def baseline
      iterate_mp(
        scheme: "none",
        num_wl: 2,
        debug: true,
        runmode: :local,
        workloads: $some_workloads,
        nametag: "longrun"
      )
    end

    def single_core
      single(
        debug: true,
        runmode: :local,
        benchmarks: %w[mcf bzip2 astar],
        nametag: "longrun"
      )
    end
    
    def ncore_ntc
      iterate_mp $secure_opts.merge(
        num_wl: 2,
      )
    end

    def ncore_ntc_monotonic
        iterate_mp(
            schemes: %w[monotonic],
            scheme: 'monotonic',
            addrpar: true,
            num_wl: 2
        )
    end

    def ncore_ntc_donor
        iterate_mp(
            schemes: %w[donor],
            scheme: 'donor',
            addrpar: true,
            num_wl: 2
        )
    end

    def ncore_2tc
      o = $secure_opts.merge(
        nametag: "2tc"
      )

      # 3 Cores 2 TCs
      iterate_mp o.merge(
        num_wl: 3,
        skip2: true,
        skip3: false,
        p0threadID: 0,
        p1threadID: 0,
        p2threadID: 1,
      )

      # 4 Cores 2 TCs
      iterate_mp o.merge(
        num_wl: 4,
        skip2: true,
        p0threadID: 0,
        p1threadID: 0,
        p2threadID: 1,
        p3threadID: 1
      )

      # 6 Cores 2 TCs
      iterate_mp o.merge(
        num_wl: 6,
        skip2: true,
        skip4: true,
        p0threadID: 0,
        p1threadID: 0,
        p2threadID: 0,
        p3threadID: 1,
        p4threadID: 1,
        p5threadID: 1
      )

      # 8 Cores 2 TCs
      iterate_mp o.merge(
        num_wl: 8,
        skip2: true,
        skip4: true,
        skip6: true,
        p0threadID: 0,
        p1threadID: 0,
        p2threadID: 0,
        p3threadID: 0,
        p4threadID: 1,
        p5threadID: 1,
        p6threadID: 1,
        p7threadID: 1
      )

    end

end
