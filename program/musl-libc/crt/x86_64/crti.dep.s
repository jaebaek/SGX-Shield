.section .init
.global _init
_init:
    .p2align    6, 0x90
	push %rax

.section .fini
.global _fini
_fini:
    .p2align    6, 0x90
	push %rax
