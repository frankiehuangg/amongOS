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
            .segment_low = 0x0,
            .base_low = 0x0, 
            .base_mid = 0x0, 
            .type_bit = 0x0, 
            .non_system = 0b0, 
            .dpl = 0x0, 
            .p_flag = 0b0, 
            .seg_high = 0x0, 
            .avl_bit = 0b0, 
            .l_flag = 0b0, 
            .db_flag = 0b0, 
            .g_flag = 0b0, 
            .base_high = 0x0    
        },
        {
            .segment_low = 0xFFFF,
            .base_low = 0x0000, 
            .base_mid = 0x00, 
            .type_bit = 0xA, 
            .non_system = 0b1, 
            .dpl = 0x0, 
            .p_flag = 0b1, 
            .seg_high = 0xF, 
            .avl_bit = 0b0, 
            .l_flag = 0b0, 
            .db_flag = 0b1, 
            .g_flag = 0b1, 
            .base_high = 0x00   
        },
        {
            .segment_low = 0xFFFF,
            .base_low = 0x0000, 
            .base_mid = 0x00, 
            .type_bit = 0x2, 
            .non_system = 0b1, 
            .dpl = 0x0, 
            .p_flag = 0b1, 
            .seg_high = 0xF, 
            .avl_bit = 0b0, 
            .l_flag = 0b0, 
            .db_flag = 0b1, 
            .g_flag = 0b1, 
            .base_high = 0x00  
        }
    }
};

/**
 * _gdt_gdtr, predefined system GDTR. 
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    sizeof(struct GlobalDescriptorTable)-1,
    &global_descriptor_table
};
