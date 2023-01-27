read_trace = executable('read_trace', 'read_trace.c',
  include_directories : inc,
  link_with : SeisSegy,
  dependencies : seistrace_dep)
test('Test trace reading from SEGY in IBM encoding', read_trace,
  args : '../samples/ibm.sgy')
test('Test trace reading from SEGY in IEEE single encoding', read_trace,
  args : '../samples/ieee_single.sgy')
test('Test trace reading from SEGY in 1b integer encoding', read_trace,
  args : '../samples/1I.sgy')
test('Test trace reading from SEGY in 2b integer encoding', read_trace,
  args : '../samples/2I.sgy')
test('Test trace reading from SEGY in 4b integer encoding', read_trace,
  args : '../samples/4I.sgy')
