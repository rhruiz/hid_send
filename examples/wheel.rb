#!/usr/bin/env ruby

args = ["rgblight_color 0 255", "rgblight_reset"]
hid = File.expand_path('../../hid_send', __FILE__)

Signal.trap("INT") do
  %x[#{hid} rgblight_reset]
  exit(0)
end

angle = 0

loop do
  angle = (angle + 10) % 255
  %x[#{hid} rgblight_color #{angle} 255]
  sleep 0.2
end
