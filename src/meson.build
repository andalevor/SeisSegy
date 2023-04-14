sources = ['SeisISegy.c', 'SeisCommonSegy.c', 'SeisEncodings.c', 'SeisOSegy.c',
  'SeisOSegyRev0.c']
SeisSegy = library('seissegy', sources,
  include_directories : inc,
  dependencies : [seistrace_dep, m_dep],
  install : true)
