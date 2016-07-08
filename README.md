# mruby-procutil   [![Build Status](https://travis-ci.org/haconiwa/mruby-procutil.svg?branch=master)](https://travis-ci.org/haconiwa/mruby-procutil)
Procutil class
## install by mrbgems
- add conf.gem line to `build_config.rb`

```ruby
MRuby::Build.new do |conf|

    # ... (snip) ...

    conf.gem :github => 'haconiwa/mruby-procutil'
end
```
## example
```ruby
p Procutil.hi
#=> "hi!!"
t = Procutil.new "hello"
p t.hello
#=> "hello"
p t.bye
#=> "hello bye"
```

## License
under the MIT License:
- see LICENSE file
