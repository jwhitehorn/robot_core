require 'pi_piper'

module RobotCore
  class Firmware

    def send_command(op_code, options)
      data = [0x00, RobotCore::ByteCode.for_name(op_code)].merge options
      PiPiper::Spi.begin
        write data
      end
    end

  end
end
