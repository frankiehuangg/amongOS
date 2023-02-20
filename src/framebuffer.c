#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"
#include "stdmem.c"
#include "portio.c"
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

unsigned short *textmemptr;


void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
     uint16_t attrib = (bg << 4) | (fg);
     volatile uint16_t * where;
     where = (volatile uint16_t *)0xB8000 + (row * 80 + col) ;
     *where = c | (attrib << 8);
}

/**
 * Set cursor to specified location. Row and column starts from 0
 * 
 * @param r row
 * @param c column
*/
void framebuffer_set_cursor(uint8_t r, uint8_t c){

    uint16_t position = r * 80 + c;
    // Send the high byte of the position to the control register
    out(0x3D4, 0x0E);
    out(0x3D5, position >> 8);
    // Send the low byte of the position to the control register
    out(0x3D4, 0x0f);
    out(0x3D5, position & 0xFF);

}

void framebuffer_clear(void) {
    unsigned blank;
    int i;
    blank = 0;

    for(i = 0; i < 1000; i++)
        memset(textmemptr + i * 400, blank, 800);
}