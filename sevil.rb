#!/usr/bin/env ruby

require 'socket'
require 'readline'

class Sevil

  @@dpy = nil
  def Sevil.display
    return @@dpy if @@dpy
    @@dpy = `xdpyinfo`[/name of display:\s*(.*)/, 1]
    @@dpy ||= ENV['DISPLAY']
    @@dpy
  end

  def initialize(dpy = Sevil.display)
    @sock_dir = '/tmp/sevilwm_' + dpy
  end

  def sock_path(sock)
    File.join(@sock_dir, sock)
  end

  def read(sock)
    if sock == 'ctrl'
      raise 'Cannot read from ctrl'
    end
    sock = sock_path(sock)
    if !File.exist?(sock)
      raise "#{sock}: Socket not exist"
    end
    UNIXSocket.open(sock) do |s|
      s.read
    end
  end

  def ctrl(cmds = nil, &proc)
    UNIXSocket.open(sock_path('ctrl')) do |s|
      if cmds
        r = []
        cmds.each do |c|
          s.puts c
          r.push s.gets
        end
        return r
      end
      if proc
        proc[s]
      end
    end
  end

  def info
    read('info')
  end

  def windows
    w = []
    read('windows').each_line do |line|
      w << Sevil::Window.recover(line)
    end
    w
  end

  def keys
    # TODO: Add key bind structure.
    read('keys')
  end

  def ignores
    read('ignores').map
  end

  def orig_keys
    read('config_key.def')
  end

  def orig_ignores
    read('config_ign.def')
  end

  def orig_windows
    read('config_win.def')
  end

  def argv
    read('argv')
  end

  def edit_argv
    prev_argv = read('argv')
    puts 'original argv (hit ^p to use this value): ' + prev_argv
    0 while Readline::HISTORY.pop
    Readline::HISTORY << prev_argv
    next_argv = Readline.readline('> ')
    ctrl('CLEARARGV()')
    ctrl('ADDARGV("' + next_argv + '")')
  end

  def vdesk
    info[/vdesk=(\d+)/, 1].to_i
  end

  def resolution
    i = info
    [i[/x=(\d+)/, 1].to_i, i[/y=(\d+)/, 1].to_i]
  end


  class Window
    attr_reader :title, :index
    attr_accessor :x, :y, :w, :h, :vd, :focus, :mark, :wid

    def Window.recover(s)
      if /WIN\("([^"]+)", (\d+), (-?\d+), (-?\d+), (-?\d+), (-?\d+), (-?\d+), (\d+), '(.*)', 0x(.*)\)/ !~ s
        raise 'Invalid window format: ' + s
      end
      Window.new($1, $2.to_i,
                 $3.to_i, $4.to_i, $5.to_i, $6.to_i, $7.to_i, $8, $9, $10.hex)
    end

    def initialize(t, i, x, y, w, h, vd, f = 0, m = '\\0', wid = 0)
      @title = t
      @index = i
      @x = x
      @y = y
      @w = w
      @h = h
      @vd = vd
      @focus = f
      @mark = m
      @wid = wid
    end

    def to_s
      s = "#@title:#@index #{@x}x#@y+#@w+#@h vdesk=#@vd wid=#{'0x%x'%@wid}"
      if focused?
        s += ' FOCUSED'
      end
      s
    end

    def cmd
      "WIN(\"#@title\", #@index, #@x, #@y, #@w, #@h, #@vd, #@focus, '#@mark')"
    end

    def focused?
      @focus == '1'
    end

    def emit(sock)
      sock.puts(cmd)
      res = sock.gets
      if /^ok/ !~ res
        puts res
      end
      res
    end
  end

end

if $0 == __FILE__
  sevil = Sevil.new
  case ARGV[0]
  when 'info'
    puts sevil.info
  when 'vdesk'
    puts sevil.vdesk
  when 'windows'
    puts sevil.windows
  when 'ignores'
    puts sevil.ignores
  when 'keys'
    puts sevil.keys
  when 'res'
    puts sevil.resolution * 'x'
  when 'orig_keys'
    puts sevil.orig_keys
  when 'orig_ignores'
    puts sevil.orig_ignores
  when 'orig_windows'
    puts sevil.orig_windows
  when 'argv'
    puts sevil.argv
  when 'edit_argv'
    sevil.edit_argv
  when 'ctrl'
    puts 'Entering interactive mode'
    while STDIN.gets
      puts sevil.ctrl([$_])
    end
  else
    puts "Usage: #$0 subcommand"
    puts "Available subcommands:"
    puts "\tinfo"
    puts "\tvdesk"
    puts "\twindows"
    puts "\tignores"
    puts "\tkeys"
    puts "\tres"
    puts "\torig_keys"
    puts "\torig_ignores"
    puts "\torig_windows"
    puts "\targv"
    puts "\tedit_argv"
  end
end
