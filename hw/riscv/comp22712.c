#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/core/boards.h"
#include "hw/riscv/riscv_hart.h"
#include "hw/core/sysbus.h"
#include "target/riscv/cpu.h"
#include "system/address-spaces.h"

#define SYS_RAM_BASE 0x0  // Standard RISC-V RAM base
#define SYS_RAM_SIZE 0x4000   // 16MB

#define USER_RAM_BASE 0x00040000
#define USER_RAM_SIZE 0x00040000  // 0x40000 to 0x7FFFF (256KB)

struct CompBoardState{
    MachineState parent;
};

#define TYPE_COMP_MACHINE MACHINE_TYPE_NAME("comp22712")
OBJECT_DECLARE_SIMPLE_TYPE(CompBoardState, COMP_MACHINE)

static void comp22712_init(MachineState *machine)
{
    MemoryRegion *system_memory = get_system_memory();
    
    // System RAM
    memory_region_add_subregion(system_memory, SYS_RAM_BASE, machine->ram);

    // User RAM
    MemoryRegion *user_ram = g_new(MemoryRegion, 1);
    memory_region_init_ram(user_ram, NULL, "user.ram", USER_RAM_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, USER_RAM_BASE, user_ram);

    // Create CPU
    RISCVCPU *cpu = RISCV_CPU(object_new(machine->cpu_type));

    // Realize CPU
    if (!qdev_realize(DEVICE(cpu), NULL, &error_fatal)) {
        return;
    }

    // Set Reset Vector
    cpu->env.resetvec = SYS_RAM_BASE;
}

static void comp22712_machine_class_init(ObjectClass *oc, const void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);

    mc->desc = "COMP22712 Lab Board";
    mc->init = comp22712_init;
    
    /* Set a default CPU if none is provided via -cpu */
    mc->default_cpu_type = TYPE_RISCV_CPU_BASE32; 
    mc->default_ram_size = SYS_RAM_SIZE;
    mc->default_ram_id = "comp22712.priv_ram";
}

static const TypeInfo comp22712_machine_typeinfo = {
    .name          = TYPE_COMP_MACHINE,
    .parent        = TYPE_MACHINE,
    .class_init    = comp22712_machine_class_init,
    .instance_size = sizeof(CompBoardState),
};

static void comp22712_machine_init_register_types(void)
{
    type_register_static(&comp22712_machine_typeinfo);
}

type_init(comp22712_machine_init_register_types)
