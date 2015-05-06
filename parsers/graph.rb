require 'rubyvis'
require_relative 'parsers'
include Parsers

module Parsers

COLUMN_W = 288
COLUMN_H = 120

PAPER_DIM = {
    h: 140,
    w: 288,
    left: 20,

    font: "9px sans-serif",
        
    lower_text_margin: 10,
    group_space: 10,
    dot_size: 8,
    legend_margin: 10 
}

def grouped_csv data, o={}
  o = {
    x_labels: $specint,
    legend: %w[]
  }.merge o

  "%-15s" % "bench," +  "#{o[:legend].inject("") { |s, l| s += "%-15s" % "#{l}, " }} \n" +
    data.each_with_index.inject("") do |s, (bench_data, i)|
        s += "%-15s" % "#{o[:x_labels][i]}: " +
          bench_data.inject("") do |si, legend_data|
              si += "%-13f" % legend_data + ", "; si
          end + "\n"
        s
    end

end

def grouped_bar data, o={}
    o = {
        w: 700, 
        h: 300,
        left: 55,
        right: 40,
        
        value_format: "%.2f",
        font: "16px sans-serif",

        y_label: "Overhead",
        num_ticks: 8,
        
        lower_text_margin: 30,
        group_space: 20,
        dot_size: 20,
        legend_margin: 18,

    }.merge o

    h = o[:h]
    w = o[:w]


    vis = pv.Panel.new.
        width(w).
        height(h).
        left(o[:left]).
        right(o[:right])

    colors = pv.Colors.category20()
    data_max = data.flatten.max
    bar_width = o[:bar_width] ||
        ( w - data[0].size * o[:group_space] ) / data.flatten.size.to_f
    group_width = data.size * bar_width + o[:group_space]
    bar_scale = (o[:h] - o[:legend_margin] - o[:dot_size] -
                 o[:lower_text_margin]*2 )/ (o[:max_scale] || data_max).to_f

    #Outer Labels
    vis.add(pv.Label).
        data(data[0]).
        bottom(0).#o[:lower_text_margin]).
        text(lambda { o[:x_labels][index] } ).
        font(o[:font]).
        text_angle(-Math::PI/6).
        left(lambda { group_width * (index-0.5) +
                     (group_width - o[:group_space] - bar_width)/2.0 } )

    data.size.times do |group|
        bar = vis.add(pv.Bar).
            data(data[group]).
            width(bar_width).
            height(lambda { |d| d * bar_scale } ).
            left(lambda { group*bar_width + index * group_width} ).
            bottom(o[:lower_text_margin] * 2).
            fillStyle(lambda {colors.scale(group) } )

        if o[:numeric_labels]
          bar.anchor("top").add(pv.Label).
              text_style("white").
              font(o[:font]).
              text(lambda { |d| o[:value_format] % d } )
        end
    end


    unless o[:legend].nil?
        legend_space = group_width * data[0].size / o[:legend].size.to_f
        vis.add(pv.Dot).
            data(o[:legend]).
            top(o[:legend_margin]).
            shape_size(o[:dot_size]).
            fillStyle(lambda { colors.scale(index) }).
            left(lambda { o[:dot_size]/2.0 + legend_space * index } ).
            anchor("right").
        add(pv.Label).
            text(lambda { |d| d.to_s }).
            font(o[:font])
    end
    
    # Y-Axis Ticks
    y = pv.Scale.linear(0, o[:max_scale] || data_max).
        range(o[:lower_text_margin]*2, o[:h] - o[:legend_margin] -
             o[:dot_size])
    vis.add(pv.Rule).
        data(y.ticks(o[:num_ticks])).
        bottom(lambda { |d| y.scale(d)}).
        stroke_style(lambda { |d| d==0 ? "#000" : "rgba(255,255,255,.3)" } ).
        add(pv.Rule).
        left(0).
        width(8).
        stroke_style("#000").
        anchor("left").add(pv.Label).
        text(y.tick_format).
        font(o[:font])

    # Y-Axis Title
    vis.anchor("left").add(pv.Label).
        text(o[:y_label]).
        left(-o[:left] + 5).
        text_align("center").
        font(o[:font]).
        text_angle(-Math::PI/2)

    vis.render
    vis.to_svg

end

end

if __FILE__ == $0
    string_to_f (grouped_stacked_bar nil), "foo.svg"
end
