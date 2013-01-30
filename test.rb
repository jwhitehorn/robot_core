require 'pi_piper'

PiPiper::Spi.begin do |spi|
  spi.clock(0)
  (0..255).each do |i|
    puts i
    spi.write 0x00, 0x01, 0x01, i
    sleep 1
  end
end
