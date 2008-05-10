# Simple makefile for local tests

QEMU_USER = ../../../bfin-linux-user/qemu-bfin
SRCS      = $(wildcard *.[csS])
SIM_SRCS  = $(SRCS)
SIM       = $(SIM_SRCS:=.x)
HOST_SRCS = $(shell grep -L -e sim:.*operating $(SRCS))
HOST      = $(HOST_SRCS:=.X)

NORMAL = [m
GREEN  = [0;32m
RED    = [0;31m
BLUE   = [1;34m
PASS   = $(GREEN)PASS$(NORMAL)
FAIL   = $(RED)FAIL$(NORMAL)

all: sim # host
check: all

sim: $(SIM)
	@fail=0 pass=0 ; \
	timeout=`which timeout 2>/dev/null` && timeout="$$timeout 10s" ; \
	if addr=`cat /proc/sys/vm/mmap_min_addr 2>/dev/null` ; then \
		if [ $$addr -ne 0 ] ; then \
			printf '\n$(RED)Please run: sudo sysctl vm.mmap_min_addr=0$(NORMAL)\n\n' ; \
			exit 1 ; \
		fi ; \
	fi ; \
	[ "$(PWD)" = "`pwd`" ] && pwd="" || pwd="`pwd`/" ; \
	rm -f $@.log ; \
	printf 'Logging to $(BLUE)%s$(NORMAL)\n' "$${pwd}$@.log" ; \
	for x in $^ ; do \
		xx="$$timeout $(QEMU_USER) $$x" ; \
		x=$${x%.x} ; \
		printf '%80s\n' | tr ' ' '#' >> $@.log ; \
		printf 'Running: %s\n' "$$xx" >> $@.log ; \
		if ! out="`$$xx 2>&1`" ; then \
			res=FAIL ; \
			cres='$(FAIL)' ; \
			: $$(( fail += 1 )) ; \
		elif [ "$$out" != "pass" ] ; then \
			res=FAIL ; \
			cres='$(FAIL)' ; \
			: $$(( fail += 1 )) ; \
		else \
			res=PASS ; \
			cres='$(PASS)' ; \
			: $$(( pass += 1 )) ; \
		fi ; \
		printf '%s\n%s: %s\n\n' "$$out" "$$res" $$x >> $@.log ; \
		echo "$$cres: $$x" ; \
	done ; \
	printf '$(BLUE)SUMMARY$(NORMAL): %i/%i tests passed (see %s for details)\n' \
		$$pass $$(( pass + fail )) "$${pwd}$@.log" ; \
	exit $$(( fail != 0 ))

jtag: $(JTAG)
host: $(HOST)

FLAGS = -g -o $@ $<

DO_SIM = bfin-elf-gcc -msim $(FLAGS) -nostdlib
%.c.x: %.c ; $(DO_SIM) $(shell sed -n '/^\# cc:/s|.*cc:||p' $<)
%.s.x: %.s ; $(DO_SIM)
%.S.x: %.S ; $(DO_SIM)

DO_JTAG = bfin-elf-gcc $(FLAGS) -nostdlib
%.c.j: %.c ; $(DO_JTAG) $(shell sed -n '/^\# cc:/s|.*cc:||p' $<)
%.s.j: %.s ; $(DO_JTAG)
%.S.j: %.S ; $(DO_JTAG)

DO_HOST = bfin-linux-uclibc-gcc -Wa,--defsym,BFIN_HOST=1 $(FLAGS) -static
%.c.X: %.c ; $(DO_HOST)
%.s.X: %.s ; $(DO_HOST)
%.S.X: %.S ; $(DO_HOST)

clean:
	rm -f *.[xX] *.o

.PHONY: all clean
