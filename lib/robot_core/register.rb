module RobotCore
  class Register

    def self.all
      {
        :motor1_pulse_width       => 0x01,
        :motor2_pulse_width       => 0x02,
        :motor1_direction         => 0x03,
        :motor2_direction         => 0x04,
        :adc1                     => 0x05,
        :gpio1                    => 0x06
      }
    end

    def self.for_name(name)
      Register.all[name]
    end

  end
end
