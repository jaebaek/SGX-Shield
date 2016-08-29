#!/bin/bash

g++ asm_add_br.cpp -o asm_add_br

file=`ls ../../musl-libc/crt/x86_64/Scrt1.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    cp ../../musl-libc/crt/x86_64/Scrt1.s ../../musl-libc/crt/x86_64/Scrt1.aslr.s
fi

file=`ls ../../musl-libc/crt/x86_64/crt1.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    cp ../../musl-libc/crt/x86_64/crt1.s ../../musl-libc/crt/x86_64/crt1.aslr.s
fi

file=`ls ../../musl-libc/crt/x86_64/crti.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    cp ../../musl-libc/crt/x86_64/crti.s ../../musl-libc/crt/x86_64/crti.aslr.s
fi

file=`ls ../../musl-libc/crt/x86_64/crtn.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    cp ../../musl-libc/crt/x86_64/crtn.s ../../musl-libc/crt/x86_64/crtn.aslr.s
fi

file=`ls ../../musl-libc/src/string/x86_64/memcpy.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/string/x86_64/memcpy.s > ../../musl-libc/src/string/x86_64/memcpy.aslr.s
fi

file=`ls ../../musl-libc/src/string/x86_64/memmove.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/string/x86_64/memmove.s > ../../musl-libc/src/string/x86_64/memmove.aslr.s
fi

file=`ls ../../musl-libc/src/string/x86_64/memset.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/string/x86_64/memset.s > ../../musl-libc/src/string/x86_64/memset.aslr.s
fi

file=`ls ../../musl-libc/src/fenv/x86_64/fenv.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/fenv/x86_64/fenv.s > ../../musl-libc/src/fenv/x86_64/fenv.aslr.s
fi

file=`ls ../../musl-libc/src/internal/x86_64/syscall.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/internal/x86_64/syscall.s > ../../musl-libc/src/internal/x86_64/syscall.aslr.s
fi

file=`ls ../../musl-libc/src/ldso/x86_64/dlsym.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/ldso/x86_64/dlsym.s > ../../musl-libc/src/ldso/x86_64/dlsym.aslr.s
fi

file=`ls ../../musl-libc/src/ldso/x86_64/start.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/ldso/x86_64/start.s > ../../musl-libc/src/ldso/x86_64/start.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/__invtrigl.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/__invtrigl.s > ../../musl-libc/src/math/x86_64/__invtrigl.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/acosl.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/acosl.s > ../../musl-libc/src/math/x86_64/acosl.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/asinl.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/asinl.s > ../../musl-libc/src/math/x86_64/asinl.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/atan2l.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/atan2l.s > ../../musl-libc/src/math/x86_64/atan2l.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/atanl.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/atanl.s > ../../musl-libc/src/math/x86_64/atanl.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/ceill.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/ceill.s > ../../musl-libc/src/math/x86_64/ceill.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/exp2l.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/exp2l.s > ../../musl-libc/src/math/x86_64/exp2l.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/expl.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/expl.s > ../../musl-libc/src/math/x86_64/expl.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/expm1l.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/expm1l.s > ../../musl-libc/src/math/x86_64/expm1l.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/fabs.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/fabs.s > ../../musl-libc/src/math/x86_64/fabs.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/fabsf.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/fabsf.s > ../../musl-libc/src/math/x86_64/fabsf.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/fabsl.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/fabsl.s > ../../musl-libc/src/math/x86_64/fabsl.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/floorl.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/floorl.s > ../../musl-libc/src/math/x86_64/floorl.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/fmodl.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/fmodl.s > ../../musl-libc/src/math/x86_64/fmodl.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/llrint.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/llrint.s > ../../musl-libc/src/math/x86_64/llrint.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/llrintf.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/llrintf.s > ../../musl-libc/src/math/x86_64/llrintf.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/llrintl.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/llrintl.s > ../../musl-libc/src/math/x86_64/llrintl.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/log10l.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/log10l.s > ../../musl-libc/src/math/x86_64/log10l.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/log1pl.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/log1pl.s > ../../musl-libc/src/math/x86_64/log1pl.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/log2l.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/log2l.s > ../../musl-libc/src/math/x86_64/log2l.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/logl.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/logl.s > ../../musl-libc/src/math/x86_64/logl.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/lrint.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/lrint.s > ../../musl-libc/src/math/x86_64/lrint.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/lrintf.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/lrintf.s > ../../musl-libc/src/math/x86_64/lrintf.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/lrintl.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/lrintl.s > ../../musl-libc/src/math/x86_64/lrintl.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/remainderl.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/remainderl.s > ../../musl-libc/src/math/x86_64/remainderl.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/rintl.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/rintl.s > ../../musl-libc/src/math/x86_64/rintl.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/sqrt.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/sqrt.s > ../../musl-libc/src/math/x86_64/sqrt.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/sqrtf.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/sqrtf.s > ../../musl-libc/src/math/x86_64/sqrtf.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/sqrtl.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/sqrtl.s > ../../musl-libc/src/math/x86_64/sqrtl.aslr.s
fi

file=`ls ../../musl-libc/src/math/x86_64/truncl.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/math/x86_64/truncl.s > ../../musl-libc/src/math/x86_64/truncl.aslr.s
fi

file=`ls ../../musl-libc/src/process/x86_64/vfork.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/process/x86_64/vfork.s > ../../musl-libc/src/process/x86_64/vfork.aslr.s
fi

file=`ls ../../musl-libc/src/setjmp/x86_64/longjmp.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/setjmp/x86_64/longjmp.s > ../../musl-libc/src/setjmp/x86_64/longjmp.aslr.s
fi

file=`ls ../../musl-libc/src/setjmp/x86_64/setjmp.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/setjmp/x86_64/setjmp.s > ../../musl-libc/src/setjmp/x86_64/setjmp.aslr.s
fi

file=`ls ../../musl-libc/src/signal/x86_64/restore.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/signal/x86_64/restore.s > ../../musl-libc/src/signal/x86_64/restore.aslr.s
fi

file=`ls ../../musl-libc/src/signal/x86_64/sigsetjmp.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/signal/x86_64/sigsetjmp.s > ../../musl-libc/src/signal/x86_64/sigsetjmp.aslr.s
fi

file=`ls ../../musl-libc/src/thread/x86_64/__set_thread_area.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/thread/x86_64/__set_thread_area.s > ../../musl-libc/src/thread/x86_64/__set_thread_area.aslr.s
fi

file=`ls ../../musl-libc/src/thread/x86_64/__unmapself.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/thread/x86_64/__unmapself.s > ../../musl-libc/src/thread/x86_64/__unmapself.aslr.s
fi

file=`ls ../../musl-libc/src/thread/x86_64/clone.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/thread/x86_64/clone.s > ../../musl-libc/src/thread/x86_64/clone.aslr.s
fi

file=`ls ../../musl-libc/src/thread/x86_64/syscall_cp.aslr.s 2> /dev/null`
if [[ "$file" == "" ]]; then
    ./asm_add_br ../../musl-libc/src/thread/x86_64/syscall_cp.s > ../../musl-libc/src/thread/x86_64/syscall_cp.aslr.s
fi

rm asm_add_br
