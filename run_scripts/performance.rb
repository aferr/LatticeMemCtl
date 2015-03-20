#------------------------------------------------------------------------------
# Performance Evaluation
#------------------------------------------------------------------------------
require_relative 'runscripts'
include RunScripts

module RunScripts

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
