#error
#define SIM_CPU CPUState
#define SIM_DESC int
#define CPU_STATE(cpu) 0
#define BFIN_CPU_STATE ((env)->state)
#define OPERATING_ENVIRONMENT 1
#define STATE_ENVIRONMENT(sd) OPERATING_ENVIRONMENT

#define sim_io_printf(sd, fmt, args...) printf(fmt, ## args)
#define sim_io_eprintf(sd, fmt, args...) fprintf(stderr, fmt, ## args)
#define sim_io_flush_stdout(sd) fflush (stdout)

#define read_map 0
#define write_map 1

#define _TRACE_STUB(cpu, fmt, args...) do { if (0) printf (fmt, ## args); } while (0)
#define TRACE_INSN(cpu, fmt, args...) qemu_log_mask(CPU_LOG_TB_IN_ASM, fmt "\n", ## args)
#define TRACE_DECODE(...) _TRACE_STUB(__VA_ARGS__)
#define TRACE_EXTRACT(...) _TRACE_STUB(__VA_ARGS__)
#define TRACE_CORE(...) _TRACE_STUB(__VA_ARGS__)
#define TRACE_EVENTS(...) _TRACE_STUB(__VA_ARGS__)
#define TRACE_BRANCH(...) _TRACE_STUB(__VA_ARGS__)
#define TRACE_REGISTER(...) _TRACE_STUB(__VA_ARGS__)

#include "bfin-sim.h"

#undef IFETCH
#define IFETCH(addr) lduw_code(addr)
#undef __PUT_MEM
#define __PUT_MEM(taddr, v, size) \
	do { \
		switch (size) { \
		case 8: stb(taddr, v); break; \
		case 16: stw(taddr, v); break; \
		case 32: stl(taddr, v); break; \
		} \
	} while (0)
#undef _GET_MEM
#define _GET_MEM(taddr, size) \
	(size == 8 ? ldub(taddr) : size == 16 ? lduw(taddr) : ldl(taddr))

#define CLAMP(a, b, c) MIN (MAX (a, b), c)

#define sim_engine_halt(...) abort()
#define SIM_SIGTRAP SIGTRAP
#define SIM_SIGILL SIGILL
#define SIM_SIGBUS SIGBUS
#define SIM_SIGSEGV SIGSEGV
#define sim_io_error(...) abort()
