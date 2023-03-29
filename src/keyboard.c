#include "lib-header/keyboard.h"
#include "lib-header/portio.h"
#include "lib-header/framebuffer.h"
#include "lib-header/stdmem.h"

const char keyboard_scancode_1_to_ascii_map[256] = {
      0, 0x1B, '1', '2', '3', '4', '5', '6',  '7', '8', '9',  '0',  '-', '=', '\b', '\t',
    'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',  'o', 'p', '[',  ']', '\n',   0,  'a',  's',
    'd',  'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\',  'z', 'x',  'c',  'v',
    'b',  'n', 'm', ',', '.', '/',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '-',    0,    0,   0,  '+',    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,

      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};



static struct KeyboardDriverState keyboard_state = {FALSE,FALSE, 0 ,{0} };


/* -- Driver Interfaces -- */

// Activate keyboard ISR / start listen keyboard & save to buffer
void keyboard_state_activate(void){
	activate_keyboard_interrupt();
    keyboard_state.keyboard_input_on = 1;
}

// Deactivate keyboard ISR / stop listening keyboard interrupt
void keyboard_state_deactivate(void){
    keyboard_state.keyboard_input_on = 0;
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

void keyboard_isr(void)
{
	if (!keyboard_state.keyboard_input_on)
		keyboard_state.buffer_index = 0;
	else
	{
		uint8_t	scancode	= in(KEYBOARD_DATA_PORT);
		char 	mapped_char	= keyboard_scancode_1_to_ascii_map[scancode];

		// TODO: Implement scancode processing
		keyboard_state.buffer_index = 0;


		uint16_t pos = get_cursor_position();

		int row = pos / 80;
		int col = pos % 80;
		if (scancode == EXTENDED_SCANCODE_BYTE) {
            keyboard_state.read_extended_mode = TRUE;
        } else if (keyboard_state.read_extended_mode) {
            keyboard_state.read_extended_mode = FALSE;
            switch (scancode) {
                case EXT_SCANCODE_UP:
                    if (row>=1){
                        framebuffer_set_cursor(row-1,col);
                    }
                    break;
                case EXT_SCANCODE_DOWN:
					if (row<24){
                        framebuffer_set_cursor(row+1,col);
					}
                    break;
                case EXT_SCANCODE_LEFT:
                    if(col>=1){
                        framebuffer_set_cursor(row,col-1);
                    }
                    break;
                case EXT_SCANCODE_RIGHT:
					if(col<=80){		
    			        framebuffer_set_cursor(row,col+1);
					}
                    break;
                default:
                    break;
			}
		}
		else if (mapped_char == '\n')
		{
			if (row <24){
				framebuffer_set_cursor(row + 1, 0);
				keyboard_state.buffer_index = 0;
			}
		}

		else if (mapped_char == '\b')
		{
			if (col != 0)
			{
				framebuffer_write(row, col-1, 0, 0xF, 0);
				framebuffer_set_cursor(row, col-1);

				keyboard_state.buffer_index--;
			}
		}
		else if (mapped_char == '\t')
		{
			for (int i = 0; i < 4; i++)
			{
				framebuffer_write(row, col, ' ', 0xF, 0);
				framebuffer_set_cursor(row, ++col);

				keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = ' ';
				keyboard_state.buffer_index++;
			}
		}
		else if (mapped_char != 0)
		{
			framebuffer_write(row, col, mapped_char, 0xF, 0);
			framebuffer_set_cursor(row, col+1);

			keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = mapped_char;
			keyboard_state.buffer_index++;
		}
	}
	pic_ack(IRQ_KEYBOARD);
}