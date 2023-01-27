project('SeisSegy', 'c',
  version : '0.0.1',
  license : 'LGPL')
inc = include_directories('include', 'extern/mlib')
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
seistrace_dep = dependency('seistrace')
subdir('include')
subdir('src')
subdir('test')
pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : SeisSegy,
  version : '0.0.1',
  name : 'libseissegy',
  filebase : 'seissegy',
  description : 'A library for SEGY reading and writing.')
