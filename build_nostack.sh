#!/bin/bash
cc -O2 -Ilinux -Iposix -I. -DFLAGS_STR=\""-O2 -DPERFORMANCE_RUN=1 -lrt"\" \
    -DITERATIONS=10 -DPERFORMANCE_RUN=1 \
    core_list_join.c core_main.c core_matrix.c core_state.c core_util.c posix/core_portme.c \
    -o ./coremark_nolibc_nofp_nostack.exe \
    -static -DTIMER_RES_DIVIDER=1 -DHAS_FLOAT=0 -DHAS_PRINTF=0 -DMEM_METHOD=MEM_STATIC -DNO_STACK=1 -DMAIN_HAS_NOARGC=1 -DSEED_METHOD=SEED_VOLATILE \
    minic.c -nostdlib -mgeneral-regs-only "$@"

