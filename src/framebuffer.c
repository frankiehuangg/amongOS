#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"

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
     where = (volatile uint16_t *)MEMORY_FRAMEBUFFER + (row * 80 + col) ;
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
    out(CURSOR_PORT_CMD, 0x0E);
    out(CURSOR_PORT_DATA, position >> 8);
    // Send the low byte of the position to the control register
    out(CURSOR_PORT_CMD, 0x0f);
    out(CURSOR_PORT_DATA, position & 0xFF);

}

void framebuffer_clear(void) {
    for(int i = 0; i < 25; i++){
        for(int j = 0; j < 80 ; j++){
            framebuffer_write(i, j, 0, 0x7, 0);
        }
    }
}
void update_cursor(int x, int y)
{
	uint16_t position = y * 80 + x;
 
	out(0x3D4, 0x0F);
	out(0x3D5, (uint8_t) (position & 0xFF));
	out(0x3D4, 0x0E);
	out(0x3D5, (uint8_t) ((position >> 8) & 0xFF));
}
uint16_t get_cursor_position(void)
{
    uint16_t pos = 0;
    out(0x3D4, 0x0F);
    pos |= in(0x3D5);
    out(0x3D4, 0x0E);
    pos |= ((uint16_t)in(0x3D5)) << 8;
    return pos;
}
void framebuffer_shift(void)
{
	volatile uint16_t *buf1;
	volatile uint16_t *buf2;
	
	for (uint32_t i = 0; i < 25; i++)
	{
		for (uint32_t j = 0; j < 80; j++)
		{
			buf1 = (volatile uint16_t *)MEMORY_FRAMEBUFFER + (i * 80 + j);
			buf2 = (volatile uint16_t *)MEMORY_FRAMEBUFFER + ((i + 1) * 80 + j);
			*buf1 = *buf2;
		}
	}

	for (uint32_t i = 0; i < 80; i++)
		framebuffer_write(24, i, 0, 0x7, 0);
	
	out(CURSOR_PORT_CMD, 0x0E);
	int pos = (in(CURSOR_PORT_DATA) << 8);
	out(CURSOR_PORT_CMD, 0x0F);
	pos = pos | in(CURSOR_PORT_DATA);

	uint8_t row = pos / 80;
	
	framebuffer_set_cursor(row, 0);
}

void puts(char* buffer, uint32_t length, uint32_t text_color)
{
	for (uint32_t i = 0; i < length; i++)
	{
		out(CURSOR_PORT_CMD, 0x0E);
		int pos = (in(CURSOR_PORT_DATA) << 8);
		out(CURSOR_PORT_CMD, 0x0F);
		pos = pos | in(CURSOR_PORT_DATA);

		uint8_t row = pos / 80;
		uint8_t col = pos % 80;

		uint8_t bg = (text_color >> 4) & 0xF;
		uint8_t fg = text_color & 0xF;

		if (buffer[i] == '\n')
		{
			if (row == 24)
				framebuffer_shift();
			else 
				framebuffer_set_cursor(row+1, 0);
		}
		else if (buffer[i] == '\b')
		{
			if (col != 0)
			{
				framebuffer_write(row, col-1, 0, 0xF, 0x0);
				framebuffer_set_cursor(row, col-1);
			}
		}
		else if (buffer[i] != '\0')
		{
			framebuffer_write(row, col, buffer[i], fg, bg);
			framebuffer_set_cursor(row, col+1);
		}
	}
}