#!/bin/bash -x
arch=`arch`
echo $arch

case $arch in
    x86_64)
    ARCH_CFLAGS="-mgeneral-regs-only"
        ;;
    loongarch64)
#    ARCH_CFLAGS="-Ttext 0x400000"
        ;;
   *)
    ARCH_CFLAGS="-mgeneral-regs-only"
        ;;
esac

echo $ARCH_CFLAGS

x86_64-linux-gnu-gcc -O2 -Ilinux -Iposix -I. -DFLAGS_STR=\""-O2 -DPERFORMANCE_RUN=1 -lrt"\" \
    -DITERATIONS=0 -DPERFORMANCE_RUN=1 \
    core_list_join.c core_main.c core_matrix.c core_state.c core_util.c posix/core_portme.c \
    -o ./coremark_nolibc_nofp_x64.exe \
    -static -DTIMER_RES_DIVIDER=1 -DHAS_FLOAT=0 -DHAS_PRINTF=0 -DMEM_METHOD=MEM_STATIC \
    minic.c -nostdlib \
    -T vmlinux.lds -Wl,--build-id=none \
    $ARCH_CFLAGS \
    "$@"

loongarch64-linux-gnu-gcc -O2 -Ilinux -Iposix -I. -DFLAGS_STR=\""-O2 -DPERFORMANCE_RUN=1 -lrt"\" \
    -DITERATIONS=0 -DPERFORMANCE_RUN=1 \
    core_list_join.c core_main.c core_matrix.c core_state.c core_util.c posix/core_portme.c \
    -o ./coremark_nolibc_nofp_la64.exe \
    -static -DTIMER_RES_DIVIDER=1 -DHAS_FLOAT=0 -DHAS_PRINTF=0 -DMEM_METHOD=MEM_STATIC \
    minic.c -nostdlib \
    -T vmlinux.lds -Wl,--build-id=none \
    $ARCH_CFLAGS \
    "$@"

riscv64-linux-gnu-gcc -O2 -Ilinux -Iposix -I. -DFLAGS_STR=\""-O2 -DPERFORMANCE_RUN=1 -lrt"\" \
    -DITERATIONS=0 -DPERFORMANCE_RUN=1 \
    core_list_join.c core_main.c core_matrix.c core_state.c core_util.c posix/core_portme.c \
    -o ./coremark_nolibc_nofp_riscv64.exe \
    -static -DTIMER_RES_DIVIDER=1 -DHAS_FLOAT=0 -DHAS_PRINTF=0 -DMEM_METHOD=MEM_STATIC \
    minic.c -nostdlib \
    -Wl,--build-id=none \
    -fno-pie\
    "$@"

aarch64-linux-gnu-gcc -O2 -Ilinux -Iposix -I. -DFLAGS_STR=\""-O2 -DPERFORMANCE_RUN=1 -lrt"\" \
    -DITERATIONS=0 -DPERFORMANCE_RUN=1 \
    core_list_join.c core_main.c core_matrix.c core_state.c core_util.c posix/core_portme.c \
    -o ./coremark_nolibc_nofp_aarch64.exe \
    -static -DTIMER_RES_DIVIDER=1 -DHAS_FLOAT=0 -DHAS_PRINTF=0 -DMEM_METHOD=MEM_STATIC \
    minic.c -nostdlib \
    -Wl,--build-id=none \
    -fno-pie -mgeneral-regs-only \
    "$@"

# -DNO_STACK=1 -DMAIN_HAS_NOARGC=1 -DSEED_METHOD=SEED_VOLATILE -DITERATIONS=10

