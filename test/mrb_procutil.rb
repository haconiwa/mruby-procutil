##
## Procutil Test
##

assert("Procutil.sethostname") do
  assert_raise(RuntimeError) do
    Procutil.sethostname("foo.jp")
  end
end
