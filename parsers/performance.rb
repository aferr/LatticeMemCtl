#!/usr/bin/ruby
require 'colored'
require_relative 'parsers'
require_relative 'graph'
include Parsers


#------------------------------------------------------------------------------
# Absolute (Non-Normalized) Graphs
#------------------------------------------------------------------------------
def abs_baseline o={}
    puts "abs_baseline".green
    if o[:parse]
        r = o[:fun].call o.merge(
           {}
        )
        puts r
        csv = grouped_csv r.transpose, o
        string_to_f csv, "#{o[:out_dir]}/baseline_#{o[:mname]}.csv"
    end
    r = csv_to_arr "#{o[:out_dir]}/baseline_#{o[:mname]}.csv", o
    gb = grouped_bar r, o
    string_to_f gb, "#{o[:out_dir]}/baseline_#{o[:mname]}.svg"
end

def abs_tp o={}
    o = {nametag: "tdm_strict_start"}.merge o
    puts o[:nametag].to_s.green
    if o[:parse]
        r = o[:fun].call o.merge(
          scheme: "tp",
        )
        puts r
        csv = grouped_csv r.transpose, o.merge(do_avg: false)
        string_to_f csv, "#{o[:out_dir]}/#{o[:nametag]}_#{o[:mname]}.csv"
    end
    r = csv_to_arr "#{o[:out_dir]}/#{o[:nametag]}_#{o[:mname]}.csv", o
    gb = grouped_bar r, o
    string_to_f gb,  "#{o[:out_dir]}/#{o[:nametag]}_#{o[:mname]}.svg"
end

%w[tdm preempting priority].product(
    %w[strict monotonic],
    %w[start dead]
).each do |alloc,dead,time|
    name = "#{alloc}_#{dead}_#{time}"
    eval "def abs_#{name}(o={}) abs_tp o.merge(nametag: \"#{name}\") end"
end

def safe_unnorm o ={}
    leg = %w[tdm tdm+m pre pre+m pre+m+d prio prio+m prio+m+d]
    if o[:parse]
        r = [
            "tdm_strict_start",
            "tdm_monotonic_start",
            "preempting_strict_start",
            "preempting_monotonic_start",
            "preempting_monotonic_dead",
            "priority_strict_start",
            "priority_monotonic_start",
            "priority_monotonic_dead",
        ].map do |name|
            (o[:fun].call o.merge(
                nametag: name, scheme: "tp"
            )).flatten
        end
        csv = grouped_csv r.transpose, o.merge( legend: leg )
        string_to_f csv, "#{o[:out_dir]}/safe_schemes_#{o[:mname]}.csv"
    end
    r = csv_to_arr "#{o[:out_dir]}/safe_schemes_#{o[:mname]}.csv", o
    gb = grouped_bar r, o.merge( legend: leg )
    string_to_f gb, "#{o[:out_dir]}/safe_schemes_#{o[:mname]}.svg"
end

#------------------------------------------------------------------------------
# Normalized Graphs
#------------------------------------------------------------------------------
def baseline o={}
  o[:fun].call o.merge(
      {}
  )
end

def ntc o={}
  o[:fun].call o.merge(
    scheme: "tp",
    nametag: "tdm_strict_start",
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
    leg = %w[tdm tdm+m pre pre+m pre+m+d prio prio+m prio+m+d]
    if o[:parse]
        schemes = [
            "tdm_strict_start",
            "tdm_monotonic_start",
            "preempting_strict_start",
            "preempting_monotonic_start",
            "preempting_monotonic_dead",
            "priority_strict_start",
            "priority_monotonic_start",
            "priority_monotonic_dead",
        ].map do |name|
            (o[:fun].call o.merge(
                nametag: name, scheme: "tp"
            )).flatten
        end
        r = normalized(schemes, [baseline(o)[0]]*8)
        csv = grouped_csv r.transpose, o.merge( legend: leg )
        string_to_f csv, "#{o[:out_dir]}/safe_schemes_#{o[:mname]}_norm.csv"
    end
    r = csv_to_arr "#{o[:out_dir]}/safe_schemes_#{o[:mname]}_norm.csv", o
    gb = grouped_bar r, o.merge( legend: leg )
    string_to_f gb, "#{o[:out_dir]}/safe_schemes_#{o[:mname]}_norm.svg"
end

def safe_schemes_norm_tp o={}
    puts "safe_schemes_norm_tp".to_s.green
    leg = %w[tdm+m pre pre+m pre+m+d prio prio+m prio+m+d]
    if o[:parse]
        schemes = [
            "tdm_monotonic_start",
            "preempting_strict_start",
            "preempting_monotonic_start",
            "preempting_monotonic_dead",
            "priority_strict_start",
            "priority_monotonic_start",
            "priority_monotonic_dead",
        ].map do |name|
            (o[:fun].call o.merge(
                nametag: name, scheme: "tp"
            )).flatten
        end
        r = normalized(schemes, [ntc(o)[0]]*7)
        csv = grouped_csv r.transpose, o.merge( legend: leg )
        string_to_f csv, "#{o[:out_dir]}/safe_schemes_#{o[:mname]}_norm_tp.csv"
    end
    r = csv_to_arr "#{o[:out_dir]}/safe_schemes_#{o[:mname]}_norm_tp.csv", o
    gb = grouped_bar r, o.merge( legend: leg )
    string_to_f gb, "#{o[:out_dir]}/safe_schemes_#{o[:mname]}_norm_tp.svg"
end

#------------------------------------------------------------------------------
# Latency breakdown
#------------------------------------------------------------------------------
def latency_data o={}
    f =  m5out_file o 
    safe_add = lambda { |x,y| x.nil? ? 0 : y.nil? ? 0 : x + y }
    num_requests = o[:numcpus].times.map do |i|
        ireg=/system.l3.overall_misses::switch_cpus#{i}.inst\s*(\d*.\d*)/
        dreg=/system.l3.overall_misses::switch_cpus#{i}.data\s*(\d*.\d*)/
        inst_req = (find_stat f, ireg) 
        d_req    = (find_stat f, dreg)
        safe_add.call(inst_req, d_req)
    end.flatten.reduce(&safe_add)

    o[:numcpus].times.map do |i|
        f =  m5out_file o 
        sp = "system.physmem"
        [
            (find_stat_cpu f, /#{sp}.wasted_tmux_overhead::#{i}\s*(\d*.\d)/, i),
            (find_stat_cpu f, /#{sp}.tmux_overhead::#{i}\s*(\d*.\d)/, i),
            (find_stat_cpu f, /#{sp}.dead_time_overhead::#{i}\s*(\d*.\d)/, i),
            (find_stat_cpu f, /#{sp}.queueing_delay::#{i}\s*(\d*.\d)/,i ),
        ]
    end.transpose.map { |i| i.nil? ? 0 : i.reduce(&safe_add) }.map do |lat|
        num_requests == 0 ? 0 : lat / num_requests
    end
end
def latency_data_of(p={}) data_of(p){|o| latency_data o} end

def baseline_latency o={}
    puts "baseline_latency".green
    legend = %w[wtmux tmux dead queueing]
    if o[:parse]
        r = (latency_data_of o)[0]
        csv = grouped_csv r, o.merge( legend: legend )
        string_to_f csv, "#{o[:out_dir]}/baseline_latency.csv"
    end
    r = csv_to_arr "#{o[:out_dir]}/baseline_latency.csv", o
    gb = grouped_bar r, o.merge( legend: legend )
    string_to_f gb,  "#{o[:out_dir]}/baseline_latency.svg"
end

def tp_latency o={}
    o = {nametag: "tdm_strict_start"}.merge o
    puts "#{o[:nametag]}_latency".green
    legend = %w[wtmux tmux dead queueing]
    if o[:parse]
        r = (latency_data_of o.merge(scheme: "tp"))[0]
        csv = grouped_csv r, o.merge( legend: legend )
        string_to_f csv, "#{o[:out_dir]}/#{o[:nametag]}_latency.csv"
    end
    r = csv_to_arr "#{o[:out_dir]}/#{o[:nametag]}_latency.csv", o
    gb = grouped_bar r, o.merge( legend: legend )
    string_to_f gb,  "#{o[:out_dir]}/#{o[:nametag]}_latency.svg"
end

%w[tdm preempting priority].product(
    %w[strict monotonic],
    %w[start dead]
).each do |alloc,dead,time|
    name = "#{alloc}_#{dead}_#{time}"
    eval "def #{name}_latency(o={}) tp_latency o.merge(nametag: \"#{name}\") end"
end

def latency_norm o={}
    legend = %w[wtmux tmux dead queueing]
    puts "#{o[:nametag]}_latency_norm".green
    if o[:parse]
        o = o.merge(scheme: "tp")
        r = normalized(
          (latency_data_of o)[0],
          (latency_data_of o.merge(nametag: "tdm_strict_start"))[0]
        )
        csv = grouped_csv r, o.merge( legend: legend )
        string_to_f csv, "#{o[:out_dir]}/#{o[:nametag]}_latency_norm.csv"
    end
    r = csv_to_arr "#{o[:out_dir]}/#{o[:nametag]}_latency_norm.csv", o
    gb = grouped_bar r, o.merge( legend: legend )
    string_to_f gb,  "#{o[:out_dir]}/#{o[:nametag]}_latency_norm.svg"
end

%w[tdm preempting priority].product(
    %w[strict monotonic],
    %w[start dead]
).each do |alloc,dead,time|
    name = "#{alloc}_#{dead}_#{time}"
    sig  = "#{name}_latency_norm(o={})"
    body = "latency_norm o.merge(nametag: \"#{name}\")"
    eval "def #{sig} #{body} end"
end

#------------------------------------------------------------------------------
# "Main"
#------------------------------------------------------------------------------

if __FILE__ == $0
  in_dir  = ARGV[0].to_s
  out_dir = ARGV[1].to_s
  FileUtils.mkdir_p(out_dir) unless File.directory?(out_dir)

  parse_o = {
      core_set: [8],
      dir: in_dir,
      out_dir: out_dir,
      numcpus: 8,
      scheme: "none",
      fun: method(:stp_data_of),
      mname: "stp",
      workloads: $workloads_8core,
  }

  abs_o = parse_o.merge(
      #graph options
      group_space: 2,
      lower_text_margin: 25,
      legend_margin: 10,
      dot_size: 0,
      numeric_labels: true,
      # max_scale: 2.5
      # h: 360,
      # w: 864,
      # font: "18px arial"
      keep_list: keep_list = %w[
          mcf_mcf
          mcf_lib
          lib_lib
          h264_hmm
          mix_1  
          mix_2  
          mix_3  
          mix_6  
          mix_8  
      ],
      x_labels: keep_list,
      y_label: "System Throughput",
  )

  abs_baseline                   abs_o
  abs_tdm_strict_start           abs_o
  abs_tdm_monotonic_start        abs_o
  abs_preempting_strict_start    abs_o
  abs_preempting_monotonic_start abs_o
  abs_preempting_monotonic_dead  abs_o
  abs_priority_strict_start      abs_o
  abs_priority_monotonic_start   abs_o
  abs_priority_monotonic_dead    abs_o
  # safe_unnorm abs_o

  latency_o = abs_o.merge(
    group_space: 15,
    legend_margin: 10,
    dot_size: 15,
    numeric_labels: false,
    y_label: "Avg Latency"
  )

  baseline_latency                   latency_o
  tdm_strict_start_latency           latency_o
  tdm_monotonic_start_latency        latency_o
  preempting_strict_start_latency    latency_o
  preempting_monotonic_start_latency latency_o
  preempting_monotonic_dead_latency  latency_o
  priority_strict_start_latency      latency_o
  priority_monotonic_start_latency   latency_o
  priority_monotonic_dead_latency    latency_o

  latency_norm_o = latency_o.merge(
    group_space: 15,
    y_label: "Normalized Avg Latency"
  )

  tdm_monotonic_start_latency_norm        latency_norm_o
  preempting_strict_start_latency_norm    latency_norm_o
  preempting_monotonic_start_latency_norm latency_norm_o
  preempting_monotonic_dead_latency_norm  latency_norm_o
  priority_strict_start_latency_norm      latency_norm_o
  priority_monotonic_start_latency_norm   latency_norm_o
  priority_monotonic_dead_latency_norm    latency_norm_o

  normo = parse_o.merge( 

      # graph options
      keep_list: keep_list = %w[
          mcf_mcf
          mcf_lib
          lib_lib
          h264_hmm
          mix_1  
          mix_2  
          mix_3  
          mix_6  
          mix_8  
      ],
      x_labels: keep_list,
      y_label: "Normalized STP",
  )

  safe_schemes normo
  safe_schemes_norm_tp normo

  svg2pdf out_dir

end
