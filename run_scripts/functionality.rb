#------------------------------------------------------------------------------
# Security Tests
#------------------------------------------------------------------------------
require_relative 'runscripts'
require_relative 'performance'
require 'colored'
include RunScripts

module RunScripts
    # Default options for testing
    $test_opts = {
        debug: true,
        fastforward: 10,
        maxinsts: 10**5,
        num_wl: 2,
        runmode: :local
    }

    # Single test
    def single_short
        single $test_opts
    end
    
    # Baseline test
    def baseline_short
        iterate_mp $test_opts
    end

    # Secure test
    def secure_short
        iterate_mp $test_opts.merge(
            scheme: "tp",
            addrpar: true
        )
    end

    def tp_config_test
        o = $test_opts.merge(
            addrpar: true,
            scheme: "tp",
            workloads: {
                nothing_hardstride: %w[nothing hardstride],
                # hardstride_hardstride: %w[nothing hardstride],
                # hardstride_nothing: %w[hardstride nothing],
            },
            maxinsts: 100
        )

        # TDM, strict, turn start
        iterate_mp o.merge(
            nametag: "tdm_strict_start"
        )

        # TDM, Monotonic, turn start
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
            dead_time_policy: 0,
            nametag: "priority_monotonic_dead"
        )




    end

    # Donor test
    def donor_short
      iterate_mp $test_opts.merge(
        scheme: "donor",
        maxinsts: 10**5,
        fastforward: 10,
        num_wl: 2,
        addrpar: true
      )
    end

    def monotonic_short
        iterate_mp $test_opts.merge(
            scheme: "monotonic",
            maxinsts: 10**5,
            fastforward: 10,
            num_wl: 2,
            addrpar: true
        )
    end
    
    def inv_prio_short 
      iterate_mp $test_opts.merge(
        scheme: "invprio",
        maxinsts: 10**5,
        fastforward: 10,
        num_wl: 2,
        addrpar: true
      )
    end

end
