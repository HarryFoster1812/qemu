#include "qemu/osdep.h"  // ALWAYS FIRST
#include "hw/riscv/comp22712/comp_board.h"
#include "hw/core/sysbus.h"
#include "exec/hwaddr.h"
#include <SDL.h>

#define WINDOW_W 400
#define WINDOW_H 200
#define BUTTON_SIZE 30
#define LED_SIZE 30

static SDL_Window *board_window;
static SDL_Renderer *board_renderer;
static CompBoardState *global_board;
static bool gui_running = true;
static pthread_t gui_thread;

/* ---------- MMIO Callbacks ---------- */
void comp22712_led_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    CompBoardState *s = opaque;
    s->state.leds = val & 0xFF; 
}

static uint64_t comp22712_button_read(void *opaque, hwaddr addr, unsigned size)
{
    CompBoardState *s = opaque;
    return s->state.buttons & 0xFF;
}

static const MemoryRegionOps comp22712_led_ops = {
    .write = comp22712_led_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static const MemoryRegionOps comp22712_button_ops = {
    .read = comp22712_button_read,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

/* ---------- GUI Logic ---------- */

static void handle_sdl_event(SDL_Event *ev)
{
    if (ev->type == SDL_QUIT) {
        gui_running = false;
    }
    // No key handling for now, just staying alive
}

static void board_draw(void)
{
    // Set color to Black (Red=0, Green=0, Blue=0, Alpha=255)
    SDL_SetRenderDrawColor(board_renderer, 0, 0, 0, 255);
    
    // Clear the entire window with that color
    SDL_RenderClear(board_renderer);
    
    // Present the empty black frame to the screen
    SDL_RenderPresent(board_renderer);
}

static void *board_gui_thread(void *arg)
{
    // Initialize SDL inside the thread
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL Init Failed: %s\n", SDL_GetError());
        return NULL;
    }

    board_window = SDL_CreateWindow("COMP22712 Board", 
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                   WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
    
    if (!board_window) {
        fprintf(stderr, "Window Creation Failed: %s\n", SDL_GetError());
        return NULL;
    }

    board_renderer = SDL_CreateRenderer(board_window, -1, SDL_RENDERER_ACCELERATED);

    printf("--- SDL Window is now ACTIVE ---\n");

    while (gui_running) {
        SDL_Event ev;
        // This MUST happen in the same thread that created the window
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) gui_running = false;
            handle_sdl_event(&ev); 
        }
        board_draw();
        SDL_Delay(33); // ~30 FPS
    }

    SDL_DestroyRenderer(board_renderer);
    SDL_DestroyWindow(board_window);
    SDL_Quit();
    return NULL;
}

// Logic to initialize the QOM Object
static void comp_board_init(Object *obj)
{
    CompBoardState *s = COMP_BOARD(obj);
    memory_region_init_io(&s->led_mmio, obj, &comp22712_led_ops, s, "led", 0x8);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->led_mmio);

    memory_region_init_io(&s->button_mmio, obj, &comp22712_button_ops, s, "button", 0x8);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->button_mmio);
}

// Called by the Machine Init
void comp_board_create_window(CompBoardState *s)
{
    global_board = s;
    // Just launch the thread. Let the thread handle the window.
    if (pthread_create(&gui_thread, NULL, board_gui_thread, NULL) != 0) {
        fprintf(stderr, "Failed to create GUI thread\n");
        return;
    }
    printf("GUI Thread launched...\n");
}

static const TypeInfo comp_board_info = {
    .name          = TYPE_COMP_BOARD,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(CompBoardState),
    .instance_init = comp_board_init,
};

static void comp_board_register_types(void) {
    type_register_static(&comp_board_info);
}

type_init(comp_board_register_types)
