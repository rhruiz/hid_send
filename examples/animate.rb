#!/usr/bin/env ruby

args = ["rgblight_color 0 255", "rgblight_reset"]
hid = File.expand_path('../../hid_send', __FILE__)

Signal.trap("INT") do
  %x[#{hid} rgblight_reset]
  exit(0)
end

args.cycle do |arg|
  %x[#{hid} #{arg}]
  sleep 0.6
end
