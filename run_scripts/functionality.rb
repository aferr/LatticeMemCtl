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
        maxinsts: 10**4,
        num_wl: 2,
        runmode: :local,
        tl0: 44,
        tl1: 44
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

    def all_secure_test
        secure $test_opts.merge(
            security_policy: 0,
            num_wl: 4,
            addrpar: true,
            scheme: "tp",
            workloads: {
                hrd_hrd: (%w[hardstride] *4)
            }
        )
    end

    def diamond_test
        secure $test_opts.merge(
            security_policy: 1,
            num_wl: 4,
            skip2: true,
            addrpar: true,
            scheme: "tp",
            # workloads: {
            #     hrd_hrd: (%w[hardstride] *4)
            # }
        )
    end

end
