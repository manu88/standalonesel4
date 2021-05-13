/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

.section .text
.global _sel4_start
_sel4_start:
	leaq __stack_top, %rsp
    movq    %rbx, %rdi
    call    __sel4_start_root

	/* should not return */
1:
	jmp  1b

.section .bss
__stack_base:
	.align 16
	.space 16384
__stack_top:
