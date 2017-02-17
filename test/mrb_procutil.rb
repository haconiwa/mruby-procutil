##
## Procutil Test
##

assert("Procutil.sethostname") do
  if `whoami`.chomp == 'root'
    assert_nothing_raised("The root user can change the host name") do
      Procutil.sethostname("foo.jp")
    end
  else
    assert_raise(RuntimeError) do
      Procutil.sethostname("foo.jp")
    end
  end
end
assert("Procutil.system4") do
  devnull = File.open("/dev/null", "w")

  ret = Procutil.system4 'ls -l', nil, devnull, devnull
  assert_true ret.is_a?(Process::Status)
  assert_true ret.pid > 0
  assert_equal 0, ret.exitstatus
end
