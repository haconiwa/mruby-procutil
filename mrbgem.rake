MRuby::Gem::Specification.new('mruby-procutil') do |spec|
  spec.license = 'MIT'
  spec.authors = 'Uchio Kondo'

  spec.add_test_dependency 'mruby-io', core: 'mruby-io'
  spec.add_test_dependency 'mruby-process'
end
