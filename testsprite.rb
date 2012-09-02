#!/usr/bin/env ruby

# A joke software...

require 'sevil'

sevil = Sevil.new

mx, my = sevil.resolution
puts "Display: #{mx}x#{my}"

SPR_W = 100
SPR_H = 100

windows = sevil.windows
cmds = windows.map{|w|w.cmd}

turn = 0
start = Time.now
begin
  windows = windows.map do |w|
    w.x = rand(mx-SPR_W)
    w.y = rand(my-SPR_H)
    w.w = SPR_W
    w.h = SPR_H
    w.vd = -1
    vx = vy = 0
    while vx == 0 && vy == 0
      vx = rand(3) - 1
      vy = rand(3) - 1
    end
    [w, vx, vy]
  end

  sevil.ctrl do |ctrl|
    10000.times do
    #while true
      turn += 1
      windows.map! do |w, vx, vy|
        w.x += vx
        if w.x < 0 || w.x >= mx - SPR_W
          vx = -vx
          w.x += vx
        end
        w.y += vy
        if w.y < 0 || w.y >= my - SPR_H
          vy = -vy
          w.y += vy
        end

        w.emit(ctrl)

        [w, vx, vy]
      end
    end
  end

ensure
  now = Time.now
  elapsed = now.to_i - start.to_i
  if elapsed != 0
    puts "#{turn / elapsed} FPS"
  end

  sevil.ctrl(cmds)
  puts 'recovered'
end
