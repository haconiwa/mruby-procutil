# mruby-procutil

Process controll utility for haconiwa

## install by mrbgems

- add conf.gem line to `build_config.rb`

```ruby
MRuby::Build.new do |conf|
  conf.gem :github => 'haconiwa/mruby-procutil'
end
```

## APIs

### `Procutil.sethostname`

* Calls `sethostname(2)`.

```ruby
Procutil.sethostname('example.haconiwa.io')
#=> 'example.haconiwa.io'
```

## License

under the MIT License:

- see LICENSE file
