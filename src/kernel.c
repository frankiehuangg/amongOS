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
#include "lib-header/paging.h"

void kernel_setup(void) {
	enter_protected_mode(&_gdt_gdtr);
	
	pic_remap();
	initialize_idt();

	activate_keyboard_interrupt();
	framebuffer_clear();
	framebuffer_set_cursor(0, 0);

	initialize_filesystem_fat32();
	
	gdt_install_tss();
	set_tss_register();
	
	// Allocate first 4 MiB virtual memory
	allocate_single_user_page_frame((uint8_t*)0);

	// Make ikanaide
	struct ClusterBuffer c;
	memset(&c, 0, CLUSTER_SIZE);

	char message[130] = "When the impostor is sus!\ndun dun dun dun dun dun dun dun dun dun\nbum bum\ndun dun dun dun dun dun dun\ndun dun dun\ndun dun dun dun\n";
	memcpy(&c, message, sizeof(message));

	struct FAT32DriverRequest write_request = {
		.buf 					= &c,
		.name					= "ikanaide",
		.ext					= "txt",
		.parent_cluster_number	= ROOT_CLUSTER_NUMBER,
		.buffer_size			= CLUSTER_SIZE
	};

	write(write_request);

	// Write shell into memory
	struct FAT32DriverRequest request = {
		.buf					= (uint8_t*) 0,
		.name					= "shell",
		.ext					= "\0\0\0",
		.parent_cluster_number	= ROOT_CLUSTER_NUMBER,
		.buffer_size			= 0x100000,
	};

	read(request);

	framebuffer_among_us();

	keyboard_state_activate();
	__asm__("sti");
	while(is_keyboard_blocking());
	framebuffer_clear();
	framebuffer_set_cursor(0, 0);

	// Set TSS $esp pointer and jump into shell
	set_tss_kernel_current_stack();
	kernel_execute_user_program((uint8_t*) 0);
	
	while (TRUE);
}