module RobotCore
  class ByteCode
  
    def self.all
      { :register_write         => 0x01, 
        :clear_host             => 0x02, 
        :raise_if_equal         => 0x03,
        :raise_if_less_than     => 0x04,
        :set_pin_mode           => 0x05,
        :raise_if_greater_than  => 0x06
       }
    end

    def self.for_name(name)
      ByteCode.all[name]
    end
  
  end
end
