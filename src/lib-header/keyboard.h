#ifndef _USER_ISR_H
#define _USER_ISR_H

#include "interrupt.h"
#include "stdtype.h"

#define EXT_SCANCODE_UP        0x48
#define EXT_SCANCODE_DOWN      0x50
#define EXT_SCANCODE_LEFT      0x4B
#define EXT_SCANCODE_RIGHT     0x4D

#define KEYBOARD_DATA_PORT     0x60
#define EXTENDED_SCANCODE_BYTE 0xE0
#define EXT_SCANCODE_UP        0x48
#define EXT_SCANCODE_DOWN      0x50
#define EXT_SCANCODE_LEFT      0x4B
#define EXT_SCANCODE_RIGHT     0x4D

#define KEYBOARD_DATA_PORT     0x60
#define EXTENDED_SCANCODE_BYTE 0xE0

#define KEYBOARD_BUFFER_SIZE   256

/**
 * keyboard_scancode_1_to_ascii_map[256], Convert scancode values that correspond to ASCII printables
 * How to use this array: ascii_char = k[scancode]
 * 
 * By default, QEMU using scancode set 1 (from empirical testing)
 */
extern const char keyboard_scancode_1_to_ascii_map[256];

/**
 * KeyboardDriverState - Contain all driver states
 * 
 * @param read_extended_mode Optional, can be used for signaling next read is extended scancode (ex. arrow keys)
 * @param keyboard_input_on  Indicate whether keyboard ISR is activated or not
 * @param buffer_index       Used for keyboard_buffer index
 * @param keyboard_buffer    Storing keyboard input values in ASCII
 */
struct KeyboardDriverState {
    bool    read_extended_mode;
    bool    keyboard_input_on;
    uint8_t buffer_index;
    char    keyboard_buffer[KEYBOARD_BUFFER_SIZE];
} __attribute((packed));





/* -- Driver Interfaces -- */

// Activate keyboard ISR / start listen keyboard & save to buffer
void keyboard_state_activate(void);

// Deactivate keyboard ISR / stop listening keyboard interrupt
void keyboard_state_deactivate(void);

// Get keyboard buffer values - @param buf Pointer to char buffer, recommended size at least KEYBOARD_BUFFER_SIZE
void get_keyboard_buffer(char *buf);

// Check whether keyboard ISR is active or not - @return Equal with keyboard_input_on value
bool is_keyboard_blocking(void);


/* -- Keyboard Interrupt Service Routine -- */

/**
 * Handling keyboard interrupt & process scancodes into ASCII character.
 * Will start listen and process keyboard scancode if keyboard_input_on.
 * 
 * Will only print printable character into framebuffer.
 * Stop processing when enter key (line feed) is pressed.
 * 
 * Note that, with keyboard interrupt & ISR, keyboard reading is non-blocking.
 * This can be made into blocking input with `while (is_keyboard_blocking());` 
 * after calling `keyboard_state_activate();`
 */
void keyboard_isr(void);
#define KEYBOARD_BUFFER_SIZE   256

/**
 * keyboard_scancode_1_to_ascii_map[256], Convert scancode values that correspond to ASCII printables
 * How to use this array: ascii_char = k[scancode]
 * 
 * By default, QEMU using scancode set 1 (from empirical testing)
 */
extern const char keyboard_scancode_1_to_ascii_map[256];

/**
 * KeyboardDriverState - Contain all driver states
 * 
 * @param read_extended_mode Optional, can be used for signaling next read is extended scancode (ex. arrow keys)
 * @param keyboard_input_on  Indicate whether keyboard ISR is activated or not
 * @param buffer_index       Used for keyboard_buffer index
 * @param keyboard_buffer    Storing keyboard input values in ASCII
 */
struct KeyboardDriverState{
    bool    read_extended_mode;
    bool    keyboard_input_on;
    uint8_t buffer_index;
    char    keyboard_buffer[KEYBOARD_BUFFER_SIZE];
} __attribute((packed));


static struct KeyboardDriverState keyboard_state = { 0 };


/* -- Driver Interfaces -- */

// Activate keyboard ISR / start listen keyboard & save to buffer
void keyboard_state_activate(void){
    keyboard_state.keyboard_input_on = TRUE;
}

// Deactivate keyboard ISR / stop listening keyboard interrupt
void keyboard_state_deactivate(void){
    keyboard_state.keyboard_input_on = FALSE;
}

// Get keyboard buffer values - @param buf Pointer to char buffer, recommended size at least KEYBOARD_BUFFER_SIZE
void get_keyboard_buffer(char *buf){
    for (uint8_t i = 0; i < keyboard_state.buffer_index; i++) {
        buf[i] = keyboard_state.keyboard_buffer[i];  
    }  
}

// Check whether keyboard ISR is active or not - @return Equal with keyboard_input_on value
bool is_keyboard_blocking(void){
    return keyboard_state.keyboard_input_on;
}


/* -- Keyboard Interrupt Service Routine -- */

/**
 * Handling keyboard interrupt & process scancodes into ASCII character.
 * Will start listen and process keyboard scancode if keyboard_input_on.
 * 
 * Will only print printable character into framebuffer.
 * Stop processing when enter key (line feed) is pressed.
 * 
 * Note that, with keyboard interrupt & ISR, keyboard reading is non-blocking.
 * This can be made into blocking input with `while (is_keyboard_blocking());` 
 * after calling `keyboard_state_activate();`
 */
void keyboard_isr(void){
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    if (keyboard_state.read_extended_mode) {
        keyboard_state.read_extended_mode = FALSE;

        switch (scancode) {
        case EXT_SCANCODE_UP:
            // Handle arrow up
            break;
        case EXT_SCANCODE_DOWN:
            // Handle arrow down
            break;
        case EXT_SCANCODE_LEFT:
            // Handle arrow left
            break;
        case EXT_SCANCODE_RIGHT:
            // Handle arrow right
            break;
        default:
            // Unknown extended scancode
            break;
        }
    } else if (scancode == EXTENDED_SCANCODE_BYTE) {
        keyboard_state.read_extended_mode = TRUE;
    } else {
        // Handle regular scancode
        char ascii_char = keyboard_scancode_1_to_ascii_map[scancode];

        if (ascii_char == '\n') {
            // Stop processing on enter key (line feed)
            keyboard_state.keyboard_input_on = FALSE;
        } else if (ascii_char >= ' ' && keyboard_state.buffer_index < KEYBOARD_BUFFER_SIZE) {
            // Only store printable characters in buffer
            keyboard_state.keyboard_buffer[keyboard_state.buffer_index++] = ascii_char;
        }
    }
}

#endif