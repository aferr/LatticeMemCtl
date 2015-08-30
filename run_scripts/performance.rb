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
        debug: true,
        num_wl: 4,
        skip2: true,
        workloads: {
            hard: %w[hardstride]*4
        }
    }

    $cloud_policy_4 = {
        security_policy: 2,
        p0threadID: 0,
        p1threadID: 0,
        p2threadID: 1,
        p3threadID: 2,
        numpids: 3,
        epoch_length: 4,
        tl0: 60,
        tl1: 44,
        num_wl: 4
    }

    def baseline
      iterate_mp(
        scheme: "none",
        num_wl: 4,
        skip2: true,
      )
    end

    def single_core
      single()
    end
    
    def secure_spec
        # 4 cores
        secure $cloud_policy_4.merge(
            addrpar: true,
            scheme: "tp",
            num_wl: 4,
            skip2: true,
        )
    end

    #note only run with tdm
    def tp_even
        secure(
            addrpar: true,
            scheme: "tp",
            num_wl: 4,
            skip2: true,
            nametag: "even"
        )
    end

    def diamond_spec
        secure(
            addrpar: true,
            scheme: "tp",
            num_wl: 4,
            skip2: true,
            security_policy: 1,
            nametag: "diamond_",
            epoch_length: 4
        )
    end

    def total_spec
        secure(
            nametag: "total_",
            scheme: "tp",
            addrpar: true,
            num_wl: 4,
            skip2: true,
            security_policy: 0,
            epoch_length: 4,
        )
    end

    def cache_sweep
        %w[512kB 1MB 1536kB 2MB].each do |cache|
            secure $cloud_policy_4.merge(
                addrpar: true,
                scheme: "tp",
                num_wl: 4,
                cacheSize: cache,
                nametag: "#{cache}_LLC_"
            )
        end
    end

    def epoch_sweep
        [8, 12, 16].each do |epoch|
            secure $cloud_policy_4.merge(
                addrpar: true,
                scheme: "tp",
                num_wl: 4,
                epoch_length: epoch,
                nametag: "epoch_#{epoch}_"
            )
        end
    end

    $cloud_policy_8 = {
        security_policy: 3,
        p0threadID: 0,
        p1threadID: 0,
        p2threadID: 0,
        p3threadID: 0,
        p4threadID: 1,
        p5threadID: 2,
        p6threadID: 3,
        p7threadID: 4,
        numpids: 5,
        epoch_length: 6,
        tl0: 66,
        tl1: 44,
        num_wl: 8,
        skip2: true,
        skip4: true,
        skip6: true
    }

    def cloud8
        secure $cloud_policy_8.merge(
            addrpar: true,
            scheme: "tp",
        )
    end

    def secure_partitioned
        secure $cloud_policy_4.merge(
                addrpar: true,
                scheme: "tp",
                num_wl: 4,
                skip2: true,
                tl0: 38,
                tl1: 19,
                nametag: "part_",
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

        # # TDM, Monotonic, turn start
        iterate_mp o.merge(
            turn_allocation_policy: 0,
            turn_allocatrion_time: 0,
            dead_time_policy: 1,
            nametag: o[:nametag] + "tdm_monotonic_start"
        )
          
        # # Preempting, Strict, turn start
        # iterate_mp o.merge(
        #     turn_allocation_policy: 1,
        #     turn_allocation_time: 0,
        #     dead_time_policy: 0,
        #     nametag: o[:nametag] + "preempting_strict_start"
        # )

        # # Preempting,  Monotonic, Turn Start
        # iterate_mp o.merge(
        #     turn_allocation_policy: 1,
        #     turn_allocation_time: 0,
        #     dead_time_policy: 1,
        #     nametag: o[:nametag] + "preempting_monotonic_start"
        # )
        #  
        # # Preempting,  Monotonic, Dead Time
        # iterate_mp o.merge(
        #     turn_allocation_policy: 1,
        #     turn_allocation_time: 1,
        #     dead_time_policy: 1,
        #     nametag: o[:nametag] + "preempting_monotonic_dead"
        # )

        # #  # Priority, Strict, turn start
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
