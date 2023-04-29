#include "lib-header/interrupt.h"
#include "lib-header/stdtype.h"
#include "lib-header/gdt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to GDT definition in Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
            // TODO : Implement
			// LEVEL 0
			.segment_low 		= 0,
			.base_low 			= 0,
			.base_mid 			= 0,
			.type_bit 			= 0,
			.non_system	 		= 0,
			.descriptor_level 	= 0,
			.present 			= 0,
			.segment_high 		= 0,
			.available 			= 0,
			.l_flag 			= 0,
			.default_flag 		= 0,
			.granularity 		= 0,
			.base_high 			= 0
        },
        {
            // TODO : Implement
			// LEVEL 1
			.segment_low 		= 0xFFFF,
			.base_low 			= 0,
			.base_mid 			= 0,
			.type_bit 			= 0xA,
			.non_system 		= 1,
			.descriptor_level 	= 0,
			.present 			= 1,
			.segment_high 		= 0xF,
			.available 			= 0,
			.l_flag 			= 0,
			.default_flag 		= 1,
			.granularity 		= 1,
			.base_high 			= 0
        },
        {
            // TODO : Implement
			// LEVEL 2
			.segment_low 		= 0xFFFF,
			.base_low 			= 0,
			.base_mid 			= 0,
			.type_bit 			= 0x2,
			.non_system 		= 1,
			.descriptor_level 	= 0,
			.present 			= 1,
			.segment_high 		= 0xF,
			.available 			= 0,
			.l_flag 			= 0,
			.default_flag 		= 1,
			.granularity 		= 1,
			.base_high 			= 0
        },
		{
			.segment_low 		= 0xFFFF,
			.base_low 			= 0,
			.base_mid 			= 0,
			.type_bit 			= 0xA,
			.non_system 		= 1,
			.descriptor_level 	= 3,
			.present 			= 1,
			.segment_high 		= 0xF,
			.available 			= 0,
			.l_flag 			= 0,
			.default_flag 		= 1,
			.granularity 		= 1,
			.base_high 			= 0
        },
        {
			.segment_low 		= 0xFFFF,
			.base_low 			= 0,
			.base_mid 			= 0,
			.type_bit 			= 0x2,
			.non_system 		= 1,
			.descriptor_level 	= 3,
			.present 			= 1,
			.segment_high 		= 0xF,
			.available 			= 0,
			.l_flag 			= 0,
			.default_flag 		= 1,
			.granularity 		= 1,
			.base_high 			= 0
        },
		{
			.segment_low		= sizeof(struct TSSEntry),
			.base_low			= 0,
			.base_mid 			= 0,
			.type_bit 			= 0x9,
			.non_system 		= 0,
			.descriptor_level 	= 0,
			.present 			= 1,
			.segment_high 		= (sizeof(struct TSSEntry) & (0xF << 16)) >> 16,
			.available 			= 0,
			.l_flag 			= 0,
			.default_flag 		= 1,
			.granularity 		= 0,
			.base_high 			= 0
		}
    }
};

/**
 * _gdt_gdtr, predefined system GDTR. 
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    // TODO : Implement, this GDTR will point to global_descriptor_table. 
    //        Use sizeof operator
	sizeof(struct GlobalDescriptorTable)-1,
	&global_descriptor_table
};

void gdt_install_tss(void) 
{
    uint32_t base = (uint32_t) &_interrupt_tss_entry;
    global_descriptor_table.table[5].base_high = (base & (0xFF << 24)) >> 24;
    global_descriptor_table.table[5].base_mid  = (base & (0xFF << 16)) >> 16;
    global_descriptor_table.table[5].base_low  = base & 0xFFFF;
}