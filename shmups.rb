#!/usr/bin/env ruby

# A joke software...

if $0[0] == ?/
  $: << File.dirname($0)
end

require 'sevil'

c = ARGV[0]
if c
  begin
    sevil = Sevil.new(ARGV[1])
    SPD = 20
    player = sevil.windows.find{|w|w.title == 'XClock'}
    exit(0) if !player
    File.open('/tmp/log','a'){|of|of.puts(ARGV[0])}
    case c
    when 's'
      system("xeyes -geometry +#{player.x}+#{player.y} &")
    when 'q'
      system("kill -INT #{ARGV[2]} &")
    when 'h'
      player.x -= SPD
      sevil.ctrl([player.cmd])
    when 'j'
      player.y += SPD
      puts player.cmd
      sevil.ctrl([player.cmd])
    when 'k'
      player.y -= SPD
      sevil.ctrl([player.cmd])
    when 'l'
      player.x += SPD
      sevil.ctrl([player.cmd])
    end
    exit(0)
  rescue
    File.open('/tmp/sevil_shmups.log','a') do |of|
      of.puts $!
      of.puts of.backtrace
    end
  end
end

sevil = Sevil.new

mx, my = sevil.resolution
puts "Display: #{mx}x#{my}"

SPR_W = 100
SPR_H = 100

windows = sevil.windows
cmds = windows.map{|w|w.cmd}
orig_key = sevil.keys

if windows.find{|w|w.title == 'XEyes' || w.title == 'XClock'}
  raise 'There are already xeyes or xclock'
end

system('xclock &')
player = Sevil::Window.new('XClock', 0,
                           mx / 2, my - SPR_H*2, SPR_W, SPR_H, -1)

shots = []

turn = 0
start = Time.now
begin
  winmap = {}
  windows.each do |w|
    w.x = rand(mx-SPR_W)
    w.y = rand(my-SPR_H)
    w.w = SPR_W
    w.h = SPR_H
    w.vd = -1
    vx = vy = 0
    while vx == 0 && vy == 0
      vx = (rand(3) - 1) * 5
      vy = (rand(3) - 1) * 5
    end

    winmap[[w.title, w.index]] = [w, vx, vy]
  end

  sevil.ctrl do |ctrl|
    myscript = File.join(Dir.pwd, $0)
    puts myscript
    dpy = ENV['DISPLAY']
    ['s', 'h', 'j', 'k', 'l', 'q'].each do |key|
      cmd = %Q(KEY(XK_#{key}, ev_exec_command, "#{myscript} #{key} #{dpy} #{$$}"))
      ctrl.puts(cmd)
    end
  end

  while true
  #10000.times do
    turn += 1
    sleep 0.01
    sevil.ctrl do |ctrl|
      if player
        if /^ok/ =~ player.emit(ctrl)
          player = nil
        end
      end

      if turn % 30 == 0
        sevil.windows.each do |w|
          if w.title == 'XEyes'
            shots << w
          end
        end
      end

      shots.delete_if do |s|
        s.y -= 10
        if s.y < 0
          s.y = 0
          s.vd = 3
        else
          delkeys = []
          winmap.each do |key, w|
            w = w[0]
            if (w.x - s.x).abs < SPR_W/2 && (w.y - s.y).abs < SPR_H/2
              delkeys << key
              w.vd = 3
              w.emit(ctrl)
            end
          end
          delkeys.each do |key|
            winmap.delete(key)
          end
        end

        s.emit(ctrl)
        s.y == 0
      end

      winmap.each do |_, win|
        w, vx, vy = *win
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

        win[1] = vx
        win[2] = vy
      end
    end
  end

ensure
  now = Time.now
  elapsed = now.to_i - start.to_i
  if elapsed != 0
    puts "#{turn / elapsed} FPS"
  end

  system('killall xeyes xclock')
  sevil.ctrl([cmds])
  sevil.ctrl([*orig_key])
  puts orig_key
  puts 'recovered'
end
