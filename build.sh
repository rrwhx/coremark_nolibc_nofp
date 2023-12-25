#!/bin/bash

for arch in i686 x86_64 aarch64 riscv64 loongarch64
do
    # echo $arch
    CC=${arch}-linux-gnu-gcc
    ARCH_CFLAGS=""
    case $arch in
        x86_64|aarch64)
        ARCH_CFLAGS="-mgeneral-regs-only -T vmlinux.lds"
            ;;
        i686)
        ARCH_CFLAGS="-mgeneral-regs-only -T vmlinux.lds -UTIMER_RES_DIVIDER -DTIMER_RES_DIVIDER=1000"
            ;;
    # loongarch64)
    #    ARCH_CFLAGS="-Ttext 0x400000"
            # ;;
    # *)
    #     ARCH_CFLAGS="-mgeneral-regs-only"
    #         ;;
    esac

    if which ${CC} > /dev/null; then
        echo $arch
        ${CC} -O2 -Ilinux -Iposix -I. -DFLAGS_STR=\""-O2 -DPERFORMANCE_RUN=1 -lrt"\" \
            -DITERATIONS=0 -DPERFORMANCE_RUN=1 \
            core_list_join.c core_main.c core_matrix.c core_state.c core_util.c posix/core_portme.c \
            -o ./coremark_nolibc_nofp_${arch}.exe \
            -static -DTIMER_RES_DIVIDER=1 -DHAS_FLOAT=0 -DHAS_PRINTF=0 -DMEM_METHOD=MEM_STATIC \
            minic.c \
            -nostdlib -fno-pie \
            -Wl,--build-id=none -Wl,-z,max-page-size=65536 \
            $ARCH_CFLAGS \
            "$@"
    else
        echo ${CC} dose not exist, skip
    fi

done

# -DNO_STACK=1 -DMAIN_HAS_NOARGC=1 -DSEED_METHOD=SEED_VOLATILE -DITERATIONS=10

