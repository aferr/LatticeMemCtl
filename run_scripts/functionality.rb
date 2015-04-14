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
        num_wl: 8,
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
            maxinsts: 10**5,
            fastforward: 10,
            num_wl: 2,
            addrpar: true
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
