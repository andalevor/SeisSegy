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

read_trace_header = executable('read_trace_header', 'read_trace_header.c',
  include_directories : inc,
  link_with : SeisSegy,
  dependencies : seistrace_dep)
test('Test trace header reading from SEGY', read_trace_header,
  args : '../samples/ibm.sgy')

read_all_traces = executable('read_all_traces', 'read_all_traces.c',
  include_directories : inc,
  link_with : SeisSegy,
  dependencies : seistrace_dep)
test('Test reading all traces from SEGY', read_all_traces,
  args : '../samples/ibm.sgy')

read_all_headers = executable('read_all_headers', 'read_all_headers.c',
  include_directories : inc,
  link_with : SeisSegy,
  dependencies : seistrace_dep)
test('Test reading all trace headers from SEGY', read_all_headers,
  args : '../samples/ibm.sgy')

ebcdic_to_ascii = executable('ebcdic_to_ascii', 'ebcdic_to_ascii.c',
  include_directories : inc,
  link_with : SeisSegy,
  dependencies : seistrace_dep)
test('Test reading and converting text header', ebcdic_to_ascii,
  args : '../samples/ibm.sgy')

ascii_to_ebcdic = executable('ascii_to_ebcdic', 'ascii_to_ebcdic.c',
  include_directories : inc,
  link_with : SeisSegy,
  dependencies : seistrace_dep)
test('Test reading and converting text header', ascii_to_ebcdic,
  args : '../samples/ibm.sgy')

read_write = executable('read_write', 'read_write.c',
  include_directories : inc,
  link_with : SeisSegy,
  dependencies : seistrace_dep)
test('Test SEGY reading and writing IBM FP', read_write,
  args : '../samples/ibm.sgy')
test('Test SEGY reading and writing IEEE single', read_write,
  args : '../samples/ieee_single.sgy')
test('Test SEGY reading and writing 1I', read_write,
  args : '../samples/1I.sgy')
test('Test SEGY reading and writing 2I', read_write,
  args : '../samples/2I.sgy')
test('Test SEGY reading and writing 4I', read_write,
  args : '../samples/4I.sgy')

c_args = get_option('c_args')
if '-DSU_BIG_ENDIAN' in c_args
  su_file = '../samples/big_endian_suplane.su'
else
  su_file = '../samples/suplane.su'
endif

read_write_su = executable('read_write_su', 'read_write_su.c',
  include_directories : inc,
  link_with : SeisSegy,
  dependencies : seistrace_dep)
test('Test SU reading and writing', read_write_su,
  args : su_file)

read_su_write_segy = executable('read_su_write_segy', 'read_su_write_segy.c',
  include_directories : inc,
  link_with : SeisSegy,
  dependencies : seistrace_dep)
test('Test SU reading and SEGY writing', read_su_write_segy,
  args : su_file)

print_trace_header = executable('print_trace_header', 'print_trace_header.c',
  include_directories : inc,
  link_with : SeisSegy,
  dependencies : seistrace_dep)
test('Printing all standard SEGY headers', print_trace_header,
  args : '../samples/ieee_single.sgy')
