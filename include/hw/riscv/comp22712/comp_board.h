#ifndef COMP_BOARD_H
#define COMP_BOARD_H

#include "qemu/osdep.h"
#include "hw/core/sysbus.h"
#include <SDL.h>

#define TYPE_COMP_BOARD "comp22712.board"
OBJECT_DECLARE_SIMPLE_TYPE(CompBoardState, COMP_BOARD)

#define NUM_LEDS    8
#define NUM_BUTTONS 8

typedef struct {
    uint8_t leds;       // 8 LEDs
    uint8_t buttons;    // 8 buttons
} BoardState;

struct CompBoardState {
    SysBusDevice parent_obj;

    MemoryRegion led_mmio;
    MemoryRegion button_mmio;

    BoardState state;
};


/* Create GUI window */
void comp_board_create_window(CompBoardState *s);


void board_draw_leds(void);


void comp22712_led_write(void *opaque,
                                    hwaddr addr, uint64_t val, unsigned size);

#endif /* COMP_BOARD_H */

