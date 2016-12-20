log = File.open("/tmp/log.txt", "a+")
Procutil.fd_reopen3(0, log.fileno, log.fileno)
loop do
  puts "Test"
  $stderr.puts "[Error] Test"
  sleep 1
end

