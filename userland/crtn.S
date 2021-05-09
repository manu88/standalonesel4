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
.section .init
/* gcc will nicely put the contents of crtend.o's .init section here. */
	popq %rbp
	ret
.section .fini
/* gcc will nicely put the contents of crtend.o's .fini section here. */
	popq %rbp
	ret