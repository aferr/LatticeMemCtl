#!/usr/bin/ruby
require_relative 'runscripts'
include RunScripts

module RunScripts
def rerun
    gem5home = Dir.new(Dir.pwd)
    %w[
        run_priority_monotonic_dead_tp_8cpus_mix4_stats.txt
        run_priority_monotonic_dead_tp_8cpus_mix_12_stats.txt
        run_tdm_strict_start_tp_8cpus_mix_12_stats.txt
    ].each do |experiment|
        File.open(Dir.pwd+"/scriptgen/"+experiment) {|file|
            exp_abspath = File.expand_path file
            system "qsub -wd #{gem5home.path} -e stderr/ -o stdout/ #{exp_abspath}"
        }
    end
end
end
