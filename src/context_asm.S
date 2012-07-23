/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012 Adap.tv, Inc.

    RIBS is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 2.1 of the License.

    RIBS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with RIBS.  If not, see <http://www.gnu.org/licenses/>.
*/
.extern current_ctx
.globl ribs_swapcurcontext
ribs_swapcurcontext:
        movq    current_ctx, %rsi
        /* Save the preserved registers. */
        movq    %rbx, 0(%rsi)
        movq    %rbp, 8(%rsi)
        movq    %r12, 16(%rsi)
        movq    %r13, 24(%rsi)
        movq    %r14, 32(%rsi)
        movq    %r15, 40(%rsi)
        movq    %rsp, 48(%rsi)

ribs_setcontext:
        /* Load the new stack pointer and the preserved registers.  */
        movq    %rdi, current_ctx
        movq    0(%rdi), %rbx
        movq    8(%rdi), %rbp
        movq    16(%rdi), %r12
        movq    24(%rdi), %r13
        movq    32(%rdi), %r14
        movq    40(%rdi), %r15
        movq    48(%rdi), %rsp
        ret

.globl __ribs_context_exit
__ribs_context_exit:
        movq    %rbx, %rdi
        jmp     ribs_setcontext
