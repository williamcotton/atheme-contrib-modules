God.watch do |w|
  w.name = "atheme_services"
  w.start = "/home/ec2-user/atheme/bin/atheme-services"
  w.keepalive
end
