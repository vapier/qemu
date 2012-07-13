bfin_ss = ss.source_set()
bfin_ss.add(files(
  'cpu.c',
  'gdbstub.c',
  'helper.c',
  'op_helper.c',
  'translate.c',
))

bfin_softmmu_ss = ss.source_set()
bfin_softmmu_ss.add(files(
  'machine.c',
  'monitor.c',
))

target_arch += {'bfin': bfin_ss}
target_softmmu_arch += {'bfin': bfin_softmmu_ss}
