#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include "lib-header/stdtype.h"

#define MEMORY_FRAMEBUFFER (uint8_t *) 0xC00B8000
#define CURSOR_PORT_CMD    0x03D4
#define CURSOR_PORT_DATA   0x03D5

/**
 * Terminal framebuffer
 * Resolution: 80x25
 * Starting at MEMORY_FRAMEBUFFER,
 * - Even number memory: Character, 8-bit
 * - Odd number memory:  Character color lower 4-bit, Background color upper 4-bit
*/

/**
 * Set framebuffer character and color with corresponding parameter values.
 * More details: https://en.wikipedia.org/wiki/BIOS_color_attributes
 *
 * @param row Vertical location (index start 0)
 * @param col Horizontal location (index start 0)
 * @param c   Character
 * @param fg  Foreground / Character color
 * @param bg  Background color
 */
void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg);

/**
 * Set cursor to specified location. Row and column starts from 0
 * 
 * @param r row
 * @param c column
*/
void framebuffer_set_cursor(uint8_t r, uint8_t c);

/** 
 * Set all cell in framebuffer character to 0x00 (empty character)
 * and color to 0x07 (gray character & black background)
 * 
 */
void framebuffer_clear(void);

/** 
 * create the most ssusss..............fdsfakjapgmaAAAAAAAAAAAHHHHH HLEP
 * 
 */
void framebuffer_among_us(void);

/** 
 * Update cursor to the given x and y position
 * @param x x position of cursor
 * @param y y position of cursor
 */
void update_cursor(int x, int y);

/** 
 * Get the current cursor position
 * 
 */
uint16_t get_cursor_position(void);

/** 
 * Shift the framebuffer
 * 
 */
void framebuffer_shift(void);

/** 
 * Print string to shell
 * @param buffer buffer containing content to be printed
 * @param length length of contents to be printed
 * @param text_color color of content to be printed
 */
void puts(char* buffer, uint32_t length, uint32_t text_color);
#endif