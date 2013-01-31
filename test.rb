require 'pi_piper'

PiPiper::Spi.begin do |spi|
  spi.clock(0)
  (0..255).each do |i|
    puts 255-i
    spi.write 0x00, 0x01, 0x01, 255-i
    spi.write 0x00, 0x01, 0x02, i
    sleep 1
  end
end
