# Simple makefile for local tests

QEMU_USER = ../../bfin-linux-user/qemu-bfin
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
	rm -f $@.log ; \
	for x in $^ ; do \
		xx="timeout 10s $(QEMU_USER) $$x" ; \
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
	printf '$(BLUE)SUMMARY$(NORMAL): %i/%i tests passed\n' $$pass $$(( pass + fail )) ; \
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
