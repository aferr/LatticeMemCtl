#------------------------------------------------------------------------------
# Performance Evaluation
#------------------------------------------------------------------------------
require_relative 'runscripts'
include RunScripts

module RunScripts

    def synthetic

        wl_1 = {
            p0: "hardstride",
            p1: "nothing",
            lattice_config: 1
        }

        wl_2 = {
            p0: "nothing",
            p1: "hardstride",
            lattice_config: 1
        }

        o = {
            fastforward: 0,
            maxinsts: 10**7,
            runmode: :local,
            debug: true,
            cacheSize: 0
        }
        #baseline
        sav_script o.merge wl_1.merge(scheme: "none")
        sav_script o.merge wl_2.merge(scheme: "none")

        #donor
        sav_script o.merge wl_1.merge(scheme: "donor", addrpar: true)
        sav_script o.merge wl_2.merge(scheme: "donor", addrpar: true)

        #monotonic
        sav_script o.merge wl_1.merge(scheme: "monotonic", addrpar: true)
        sav_script o.merge wl_2.merge(scheme: "monotonic", addrpar: true)
        
    end

    def baseline
      iterate_mp(
        scheme: "none",
        skip3: false,
        num_wl: 2,
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
