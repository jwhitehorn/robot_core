require 'pi_piper'

PiPiper::Spi.begin do |spi|
  (0..255).each do |i|
    puts i
    spi.write i
    sleep 1
  end
end
