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
    core_set: [2]#,4,6,8]
  )
  puts "abs_baseline".green
  puts r
  gb = grouped_bar r.transpose, o.merge(legend: [2])#,4,6,8])
  string_to_f gb, "#{o[:out_dir]}/baseline_#{o[:mname]}.svg"
end

def abs_ntc o={}
  r = o[:fun].call o.merge(
    scheme: "tp",
    core_set: [2] #,4,6,8],
  )
  puts "abs_ntc".green
  puts r
  gb = grouped_bar r.transpose, o.merge(legend: [2])#,4,6,8])
  string_to_f gb, "#{o[:out_dir]}/ntc_#{o[:mname]}.svg"
end

def abs_2tc o={}
  r = o[:fun].call o.merge(
    scheme: "tp",
    nametag: "2tc",
    core_set: [3,4]
  )
  gb = grouped_bar r.transpose, o.merge(legend: [2,3,4])
  string_to_f gb, "#{o[:out_dir]}/n_core_2_tc_#{o[:mname]}.svg"
end

def abs_donor o={}
  r = o[:fun].call o.merge(
    scheme: "donor",
    core_set: [2],
  )
  puts "abs_donor".green
  puts r
  gb = grouped_bar r.transpose, o.merge(legend: [2])
  string_to_f gb, "#{o[:out_dir]}/donor_#{o[:mname]}.svg"
end

def abs_monotonic o={}
  r = o[:fun].call o.merge(
    scheme: "monotonic",
    core_set: [2],
  )
  puts "abs_monotonic".green
  puts r
  gb = grouped_bar r.transpose, o.merge(legend: [2])
  string_to_f gb, "#{o[:out_dir]}/monotonic_#{o[:mname]}.svg"
end

#------------------------------------------------------------------------------
# Normalized Graphs
#------------------------------------------------------------------------------
def baseline o={}
  o[:fun].call o.merge(
    core_set: [2] #, 4, 6, 8]
  )
end

def ntc o={}
  o[:fun].call o.merge(
    scheme: "tp",
    core_set: [2] #, 4, 6, 8],
  )
end

def norm_ntc o={}
  r = normalized( ntc(o), baseline(o) )
  gb = grouped_bar r.transpose, o.merge( legend: %w[2 4 6 8] )
  string_to_f gb, "#{o[:out_dir]}/ntc_#{o[:mname]}_norm.svg"
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
    gb = grouped_bar r.transpose, o.merge(legend: %w[tp donor monotonic],
                                         legend_space: 58)
    string_to_f gb, "#{o[:out_dir]}/safe_schemes_#{o[:mname]}_norm.svg"

end


if __FILE__ == $0
  in_dir  = ARGV[0].to_s
  out_dir = ARGV[1].to_s
  FileUtils.mkdir_p(out_dir) unless File.directory?(out_dir)

  abs_o = {
    x_labels: $new_names,
    x_title: "System Throughput",
    core_set: [2],
    dir: in_dir,
    out_dir: out_dir,
    numcpus: 2,
    scheme: "none",
    fun: method(:stp_data_of),
    mname: "stp",
    # h: 360,
    # w: 864,
    # font: "18px arial"
  }

  abs_baseline abs_o
  abs_ntc abs_o
  abs_donor abs_o
  abs_monotonic abs_o
  
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

  safe_schemes normo

  # norm_ntc normo
  # norm_2tc normo
  #norm_breakdown normo
  
  # paramo = normo.merge(bar_width: 1)
  # norm_params paramo
  # norm_params_nocwf paramo

  # svg2pdf out_dir

end
