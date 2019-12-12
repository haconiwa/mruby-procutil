MRuby::Build.new do |conf|
  toolchain :gcc
  conf.gembox 'default'
  conf.gem '../mruby-procutil'
  conf.gem core: 'mruby-io'
  conf.gem mgem: 'mruby-process'
  conf.gem github: 'haconiwa/mruby-exec'
  conf.enable_test
end
