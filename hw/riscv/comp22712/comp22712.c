#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/core/boards.h"
#include "hw/intc/riscv_aclint.h"
#include "hw/riscv/riscv_hart.h"
#include "hw/core/sysbus.h"
#include "target/riscv/cpu.h"
#include "system/address-spaces.h"
#include "hw/riscv/comp22712/comp_board.h"

#define SYS_RAM_BASE 0x0  // Standard RISC-V RAM base
#define SYS_RAM_SIZE 0x4000   // 16MB

#define USER_RAM_BASE 0x00040000
#define USER_RAM_SIZE 0x00040000  // 0x40000 to 0x7FFFF (256KB)


// Define base address for CLINT-style timer
#define COMP22712_CLINT_BASE 0x710   // matches your boot code
#define COMP22712_CLINT_SIZE 0x20    // enough for mtime/mtimecmp

struct CompMachineState{
    MachineState parent;
};

#define TYPE_COMP_MACHINE MACHINE_TYPE_NAME("comp22712")
OBJECT_DECLARE_SIMPLE_TYPE(CompMachineState, COMP_MACHINE)

#define COMP22712_LED_ADDR    0x90000000
#define COMP22712_BUTTON_ADDR 0x90000008


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

    int base_hartid = 0;
    int num_harts = 1;

    riscv_aclint_mtimer_create(
        COMP22712_CLINT_BASE + RISCV_ACLINT_SWI_SIZE,
        0x10,                  // size for MTIME/MTIMECMP
        base_hartid,
        num_harts,
        0,                      // offset of MTIMECMP within region
        0x8,                    // offset of MTIME within region
        40000000,               // timebase frequency: 40 MHz
        false);                 // no custom callbacks
    
// Initialize the peripheral board
    DeviceState *dev = qdev_new(TYPE_COMP_BOARD);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);

    // IMPORTANT: Map the MMIO regions to the system bus!
    // Region 0 was initialized first in comp_board_init (LEDs)
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, COMP22712_LED_ADDR);
    // Region 1 was initialized second (Buttons)
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 1, COMP22712_BUTTON_ADDR);

    // Start the SDL Window
    comp_board_create_window(COMP_BOARD(dev));

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
    .instance_size = sizeof(CompMachineState),
};

static void comp22712_machine_init_register_types(void)
{
    type_register_static(&comp22712_machine_typeinfo);
}

type_init(comp22712_machine_init_register_types)
