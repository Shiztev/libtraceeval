# SPDX-License-Identifier: MIT

include $(src)/scripts/utils.mk

bdir := $(obj)/utest
eval_lib := $(obj)/lib/libtraceeval.a

TARGETS = $(bdir)/eval-utest

OBJS := eval-utest.o
OBJS += traceeval-utest.o

LIBS += -lcunit				\
	-ldl				\
	$(eval_lib)

OBJS := $(OBJS:%.o=$(bdir)/%.o)

$(bdir):
	@mkdir -p $(bdir)

$(OBJS): | $(bdir)

$(bdir)/eval-utest: $(OBJS) $(eval_lib)
	$(Q)$(do_app_build)

$(bdir)/%.o: %.c
	$(Q)$(call do_fpic_compile)

-include .*.d

test: $(TARGETS)

clean:
	$(Q)$(call do_clean,$(TARGETS) $(bdir)/*.o $(bdir)/.*.d)
