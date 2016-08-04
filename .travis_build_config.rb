MRuby::Build.new do |conf|
  toolchain :gcc
  conf.gembox 'default'
  conf.gem '../mruby-procutil'
  conf.gem mgem: 'mruby-io'
  conf.gem mgem: 'mruby-process'
  conf.enable_test
end
