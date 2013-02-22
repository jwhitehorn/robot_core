require 'rubygems'
require 'rake'
require 'echoe'

Echoe.new('robot_core', '0.1.0') do |p|
  p.description    = "Robot Core."
  p.url            = "http://github.com/jwhitehorn/robot_core"
  p.author         = "Jason Whitehorn"
  p.email          = "jason.whitehorn@gmail.com"
  p.ignore_pattern = ["hardware/*", "firmware/*"]
  p.development_dependencies =['pi_piper']
end

Dir["#{File.dirname(__FILE__)}/tasks/*.rake"].sort.each { |ext| load ext }
