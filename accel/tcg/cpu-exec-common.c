/*
 *  emulator main execution loop
 *
 *  Copyright (c) 2003-2005 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "sysemu/cpus.h"
#include "sysemu/tcg.h"
#include "qemu/plugin.h"
#include "internal-common.h"

#ifdef CONFIG_LINUX_USER
#ifdef CONFIG_CANNOLI
#include "tcg/tcg.h"
#endif /* CONFIG_CANNOLI */
#endif /* CONFIG_LINUX_USER */

bool tcg_allowed;

/* exit the current TB, but without causing any exception to be raised */
void cpu_loop_exit_noexc(CPUState *cpu)
{
    cpu->exception_index = -1;
    cpu_loop_exit(cpu);
}

void cpu_loop_exit(CPUState *cpu)
{
#ifdef CANNOLI
    // TODO CANNOLI REMOVE THIS BLOCK
    if (cannoli && cannoli->jit_entry && cannoli->jit_drop &&
            cpu->env_ptr->cannoli_r12 == CANNOLI_POISON) {
        fprintf(stderr,
            "[\x1b[31m!\x1b[0m] Cannoli: Cannoli hit a poisoned JIT exit. \
Droppping buffer and continuing...\n");
        cannoli->jit_drop();
        size_t s[3];
        cannoli->jit_entry(s);
        cpu->env_ptr->cannoli_r12 = s[0];
        cpu->env_ptr->cannoli_r13 = s[1];
        cpu->env_ptr->cannoli_r14 = s[2];
    }

    /* If we ever exit the CPU loop, perform a JIT exit */
    if(cannoli && cannoli->jit_exit) {
        CPUArchState *env = cpu->env_ptr;

        cannoli->jit_exit(
                env->cannoli_r12, env->cannoli_r13, env->cannoli_r14);
    }
#endif

    /* Undo the setting in cpu_tb_exec.  */
    cpu->neg.can_do_io = true;
    /* Undo any setting in generated code.  */
    qemu_plugin_disable_mem_helpers(cpu);
    siglongjmp(cpu->jmp_env, 1);
}

void cpu_loop_exit_restore(CPUState *cpu, uintptr_t pc)
{
    if (pc) {
        cpu_restore_state(cpu, pc);
    }
    cpu_loop_exit(cpu);
}

void cpu_loop_exit_atomic(CPUState *cpu, uintptr_t pc)
{
    /* Prevent looping if already executing in a serial context. */
    g_assert(!cpu_in_serial_context(cpu));
    cpu->exception_index = EXCP_ATOMIC;
    cpu_loop_exit_restore(cpu, pc);
}
