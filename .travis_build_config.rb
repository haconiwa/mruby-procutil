MRuby::Build.new do |conf|
  toolchain :gcc
  conf.gembox 'default'
  conf.gem '../mruby-procutil'
  conf.enable_test
end
