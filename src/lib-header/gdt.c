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
            0x0, //null descriptor
            0x0, 
            0x0, 
            0x0, 
            0b0, 
            0x0, 
            0b0, 
            0x0, 
            0b0, 
            0b0, 
            0b0, 
            0b0, 
            0x0    
        },
        {
            0xFFFF, //seglow
            0x0000, //baselow
            0x00, //basemid
            0xA, //typebit
            0b1, //sflag
            0x0, //dpl
            0b1, //pflag
            0xF, //seghi
            0b0, //avl
            0b0, //lflag
            0b1, //dbflag
            0b1, //gflag
            0x00 //basehi
        },
        {
            0xFFFF, //seglow
            0x0000, //baselow
            0x00, //basemid
            0x2, //typebit
            0b1, //sflag
            0x0, //dpl
            0b1, //pflag
            0xF, //seghi
            0b0, //avl
            0b0, //lflag
            0b1, //dbflag
            0b1, //gflag
            0x00 //basehi
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
