#------------------------------------------------------------------------------
# Performance Evaluation
#------------------------------------------------------------------------------
require_relative 'runscripts'
include RunScripts

module RunScripts

    $test_run_opts = {
        maxinsts: 100,
        fastforward: 100,
        runmode: :local,
        debug: true
    }

    def baseline
      iterate_mp(
        scheme: "none",
        num_wl: 8,
        skip6: true,
      )
    end

    def single_core
      single(
      )
    end
    
    def secure_spec
        secure(
            addrpar: true,
            scheme: "tp",
            num_wl: 8,
            skip6: true,
        )
    end

    def diamond_spec
        secure(
            addrpar: true,
            scheme: "tp",
            num_wl: 4,
            skip2: true,
            security_policy: 1,
            nametag: "diamond_"
        )
    end

    def cache_sweep
        %w[512KB 1.5MB 2MB].each do |cache|
            secure(
                addrpar: true,
                scheme: "tp",
                num_wl: 8,
                skip6: true,
                skip2: true,
                skip4: true,
                cacheSize: cache,
                nametag: "#{cache}_LLC_"
            )
            iterate_mp(
                addrpar: true,
                scheme: "tp",
                num_wl: 8,
                skip2: true,
                skip4: true,
                skip6: true,
                cacheSize: cache,
                nametag: "#{cache}_LLC"
            )
        end
    end

    def secure_partitioned
        secure(
                addrpar: true,
                scheme: "tp",
                num_wl: 8,
                skip6: true,
                skip2: true,
                skip4: true,
                tl0: 19,
                tl1: 19,
                rank_bank_partitioning: true
        )
    end
        

    def secure o={}
        o = {nametag: ""}.merge o
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
