require 'socket'

sock = UNIXSocket.open(ARGV[0])

inputs = [STDIN, sock]

while true
  i, o, e = IO.select(inputs)
  i.each do |input|
    if input == STDIN
      c = STDIN.getc
      if c
        sock.putc(c)
        sock.flush
      else
        sock.close_write
        inputs = [sock]
      end
    elsif input == sock
      c = sock.getc
      if c
        STDOUT.putc(c)
      else
        exit
      end
    end
  end
end
