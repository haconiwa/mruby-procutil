##
## Procutil Test
##

assert("Procutil#hello") do
  t = Procutil.new "hello"
  assert_equal("hello", t.hello)
end

assert("Procutil#bye") do
  t = Procutil.new "hello"
  assert_equal("hello bye", t.bye)
end

assert("Procutil.hi") do
  assert_equal("hi!!", Procutil.hi)
end
