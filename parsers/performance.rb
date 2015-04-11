#!/usr/bin/ruby
require 'colored'
require_relative 'parsers'
require_relative 'graph'
include Parsers


#------------------------------------------------------------------------------
# Absolute (Non-Normalized) Graphs
#------------------------------------------------------------------------------
def abs_baseline o={}
 r = o[:fun].call o.merge(
    core_set: [8],
  )
  puts "abs_baseline".green
  puts r
  gb = grouped_bar r, o #.merge(legend: [8])
  csv = grouped_csv r.transpose, o #.merge(legend: [8])
  string_to_f gb, "#{o[:out_dir]}/baseline_#{o[:mname]}.svg"
  string_to_f csv, "#{o[:out_dir]}/baseline_#{o[:mname]}.csv"
end

def abs_tp o={}
  r = o[:fun].call o.merge(
    scheme: "tp",
    core_set: [8]
  )
  puts "abs_ntc".green
  puts r
  gb = grouped_bar r, o.merge(legend: [8])
  csv = grouped_csv r, o.merge(legend: [8])
  string_to_f gb, "#{o[:out_dir]}/tp_#{o[:mname]}.svg"
  string_to_f csv, "#{o[:out_dir]}/tp_#{o[:mname]}.csv"
end

def abs_donor o={}
  r = o[:fun].call o.merge(
    scheme: "donor",
    core_set: [8],
  )
  puts "abs_donor".green
  puts r
  gb = grouped_bar r, o.merge(legend: [8])
  csv = grouped_csv r, o.merge(legend: [8])
  string_to_f gb, "#{o[:out_dir]}/donor_#{o[:mname]}.csv"
end

def abs_monotonic o={}
  r = o[:fun].call o.merge(
    scheme: "monotonic",
    core_set: [8],
  )
  puts "abs_monotonic".green
  puts r
  gb = grouped_bar r, o.merge(legend: [8])
  csv = grouped_bar r, o.merge(legend: [8])
  string_to_f gb, "#{o[:out_dir]}/monotonic_#{o[:mname]}.svg"
  string_to_f csv, "#{o[:out_dir]}/monotonic_#{o[:mname]}.csv"
end


#------------------------------------------------------------------------------
# Normalized Graphs
#------------------------------------------------------------------------------
def baseline o={}
  o[:fun].call o.merge(
    core_set: [2]
  )
end

def ntc o={}
  o[:fun].call o.merge(
    scheme: "tp",
    core_set: [2]
  )
end

def norm_ntc o={}
  r = normalized( ntc(o), baseline(o) )
  gb = grouped_bar r, o.merge(legend: %w[2 4 6 8])
  csv = gropuped_csv r.transpopse o.merge(legend: %w[2 4 6 8])
  string_to_f gb, "#{o[:out_dir]}/ntc_#{o[:mname]}_norm.svg"
  string_to_f csv, "#{o[:out_dir]}/ntc_#{o[:mname]}_norm.csv"
end

def safe_schemes o ={}
    schemes = [
        (o[:fun].call o.merge(
            scheme: "tp",
            core_set: [2]
        )).flatten,
        (o[:fun].call o.merge(
            scheme: "donor",
            core_set: [2]
        )).flatten,
        (o[:fun].call o.merge(
            scheme: "monotonic",
            core_set: [2]
        )).flatten,
    ]
    puts baseline(o).to_s.yellow
    r = normalized(schemes, [baseline(o)[0]]*3)
    gb = grouped_bar r, o.merge(legend: %w[tp donor monotonic])
    csv = grouped_csv r, o.merge(legend: %w[tp donor monotonic])
    string_to_f gb, "#{o[:out_dir]}/safe_schemes_#{o[:mname]}_norm.svg"
    string_to_f csv, "#{o[:out_dir]}/safe_schemes_#{o[:mname]}_norm.csv"

end

def safe_schemes_norm_tp o={}
    schemes = [
        (o[:fun].call o.merge(
            scheme: "donor",
            core_set: [2]
        )).flatten,
        (o[:fun].call o.merge(
            scheme: "monotonic",
            core_set: [2]
        )).flatten,
    ]
    r = normalized(schemes, [ntc(o)[0]]*2)
    gb = grouped_bar r, o.merge(legend: %w[donor monotonic])
    csv = grouped_csv r, o.merge(legend: %w[tp donor monotonic])
    string_to_f gb, "#{o[:out_dir]}/safe_schemes_#{o[:mname]}_norm_tp.svg"
    string_to_f csv, "#{o[:out_dir]}/safe_schemes_#{o[:mname]}_norm_tp.csv"
end

#------------------------------------------------------------------------------
# Latency breakdown
#------------------------------------------------------------------------------
def latency_data o={}
    o[:numcpus].times.map do |i|
        f =  m5out_file o 
        [
            (find_stat f, /system.physmem.wasted_tmux_overhead::#{i}\s*(\d*.\d)/),
            (find_stat f, /system.physmem.tmux_overhead::#{i}\s*(\d*.\d)/),
            (find_stat f, /system.physmem.dead_time_overhead::#{i}\s*(\d*.\d)/),
            (find_stat f, /system.physmem.queueing_delay::#{i}\s*(\d*.\d)/),
        ].map do |l|
            l / (find_stat f, /system.l3.overall_misses::total\s*(\d*.\d*)/)
        end
    end.transpose.map { |i| i.reduce(:+) }
end

def latency_data_of(p={}) data_of(p){ |o| latency_data o } end

def baseline_latency o={}
  r = (latency_data_of o)[0]
  gb = grouped_bar r.transpose, o.merge( legend: %w[wtmux tmux dead queueing])
  csv = grouped_csv r, o.merge( legend: %w[wtmux tmux dead queueing])
  string_to_f gb, "#{o[:out_dir]}/baseline_latency.svg"
  string_to_f csv, "#{o[:out_dir]}/baseline_latency.csv"
end

def tp_latency o={}
  r = (latency_data_of (o.merge scheme: "tp"))[0]
  gb = grouped_bar r.transpose, o.merge( legend: %w[wtmux tmux dead queueing])
  csv = grouped_csv r, o.merge( legend: %w[wtmux tmux dead queueing])
  string_to_f gb, "#{o[:out_dir]}/tp_latency.svg"
  string_to_f csv, "#{o[:out_dir]}/tp_latency.csv"
end

def donor_latency o={}
  r = (latency_data_of (o.merge scheme: "donor"))[0]
  gb = grouped_bar r.transpose, o.merge( legend: %w[wtmux tmux dead queueing])
  csv = grouped_csv r, o.merge( legend: %w[wtmux tmux dead queueing])
  string_to_f gb, "#{o[:out_dir]}/donor_latency.svg"
  string_to_f csv, "#{o[:out_dir]}/donor_latency.csv"
end

def monotonic_latency o={}
  r = (latency_data_of (o.merge scheme: "monotonic"))[0]
  gb = grouped_bar r.transpose, o.merge( legend: %w[wtmux tmux dead queueing])
  csv = grouped_csv r, o.merge( legend: %w[wtmux tmux dead queueing])
  string_to_f gb, "#{o[:out_dir]}/ monotonic_latency.svg"
  string_to_f csv, "#{o[:out_dir]}/monotonic_latency.csv"
end
#------------------------------------------------------------------------------
# "Main"
#------------------------------------------------------------------------------

if __FILE__ == $0
  in_dir  = ARGV[0].to_s
  out_dir = ARGV[1].to_s
  FileUtils.mkdir_p(out_dir) unless File.directory?(out_dir)

  abs_o = {
      x_labels: $new_names,
      y_title: "System Throughput",
      core_set: [2],
      dir: in_dir,
      out_dir: out_dir,
      numcpus: 2,
      scheme: "none",
      fun: method(:stp_data_of),
      mname: "stp",

      group_space: 2,
      lower_text_margin: 25,
      legend_margin: 0,
      dot_size: 0,
      # h: 360,
      # w: 864,
      # font: "18px arial"
  }

  # abs_baseline abs_o
  # abs_tp abs_o
  # abs_donor abs_o
  # abs_monotonic abs_o

  latency_o = abs_o.merge(
    group_space: 4,
    legend_margin: 10,
    dot_size: 10,
    numeric_labels: false,
    max_scale: 1800
  )

  baseline_latency latency_o
  tp_latency latency_o
  donor_latency latency_o
  monotonic_latency latency_o
  
  normo = {
      x_labels: $new_names,
      x_title: "Normalized STP",
      core_set: [2],
      dir: in_dir,
      out_dir: out_dir,
      numcpus: 2,
      scheme: "none",
      fun: method(:stp_data_of),
      mname: "stp",
  }

  # safe_schemes normo
  # safe_schemes_norm_tp normo

  # norm_ntc normo
  # norm_2tc normo
  # norm_breakdown normo
  
  # paramo = normo.merge(bar_width: 1)
  # norm_params paramo
  # norm_params_nocwf paramo

  # svg2pdf out_dir

end
