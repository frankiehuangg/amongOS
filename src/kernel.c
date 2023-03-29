#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/keyboard.h"
#include "lib-header/kernel_loader.h"
#include "lib-header/idt.h"
#include "lib-header/interrupt.h"
#include "lib-header/disk.h"
#include "lib-header/fat32.h"

void kernel_setup(void) {
	enter_protected_mode(&_gdt_gdtr);
	
	pic_remap();
	initialize_idt();

	activate_keyboard_interrupt();
	framebuffer_clear();
	framebuffer_set_cursor(0, 0);

	initialize_filesystem_fat32();

    // while (TRUE){
      keyboard_state_activate();
    // }

	struct ClusterBuffer cbuf[5];
	for (uint32_t i = 0; i < 5; i++)
		for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
			cbuf[i].buf[j] = i + 'a';

	struct FAT32DriverRequest request = {
		.buf 					= cbuf,
		.name 					= "ikanaide",
		.ext 					= "uwu",
		.parent_cluster_number 	= ROOT_CLUSTER_NUMBER,
		.buffer_size 			= 0
	};

	write(request);
	memcpy(request.name, "kano1\0\0\0", 8);
	write(request);
	memcpy(request.name, "ikanaide", 8);
	delete(request);

	memcpy(request.name, "daijoubu", 8);
	request.buffer_size = 5 * CLUSTER_SIZE;
	write(request);
	
	struct ClusterBuffer readcbuf;
	read_clusters(&readcbuf, ROOT_CLUSTER_NUMBER + 1, 1);

	for (uint32_t i = 0; i < 5; i++)
		for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
			cbuf[i].buf[j] = i + 'h';

	request.buffer_size = CLUSTER_SIZE;
	read(request);
	request.buffer_size = 5 * CLUSTER_SIZE;
	read(request);

	while(TRUE);
}

