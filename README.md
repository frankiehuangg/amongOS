# amongos

https://user-images.githubusercontent.com/86491005/235303567-43d7a10f-fb47-4ae4-99f0-f93e244b8a31.mp4

https://user-images.githubusercontent.com/86491005/228898592-f57b4f83-12bb-4f21-9e30-32d327bd104f.mp4

## Group Members

| Name                           |   Nim    |
| ------------------------------ | :------: |
| Irsyad Nurwidianto Basuki      | 13521072 |
| Farizki Kurniawan              | 13521082 |
| Frankie Huang                  | 13521092 |
| Muhammad Zaydan A              | 13521104 |

## Technologies Used
1. [Window Subsytem for Linux](https://docs.microsoft.com/en-us/windows/wsl/install)
2. [Ubuntu 20.04 LTS](https://releases.ubuntu.com/20.04/)
3. [Nasm](https://www.nasm.us/)
4. [Qemu](https://www.qemu.org/docs/master/system/target-i386.html)
5. [genisoimage](https://linux.die.net/man/1/genisoimage)
6. [GNU Make](https://www.gnu.org/software/make/)

## Setup
1. Install all of the requirements

```
sudo apt update
sudo apt install gcc nasm make qemu-system-x86 genisoimage
```

2. Clone this repository
3. To compile the OS, run the following command

```
make clean
make disk
make insert-shell
make run
```

4. qemu should spawn now, click Machine, then untick pause

## Milestone
The features listed below is 100% completed and implemented.

### Milestone 1
1. Setup
- ISO created succesfully
- ISO can be run with GRUB
- `Makefile` created and usable

2. Framebuffer
- `framebuffer_write` implemented to write into screen
- `framebuffer_clear` implemented to clear the whole screen
- `framebuffer_set_cursor` implemented to set the cursor location 

3. GDT and Protected mode
- GDT struct is according to the Intel Manual
- GDT is successfully read and segment register successfully changed
- OS can enter protected mode properly

### Milestone 2
1. Interrupt
- IDT struct is according to the Intel Manual
- PIC remapping implemented
- Interrupt handler / interrupt service routine implemented
- IDT is successfully initialized and loaded

2. Keyboard Device Driver
- `activate_keyboard_interrupt` implemented to enable keyboard interrupt
- Keyboard device interfaces implemented for user program access
- `keyboard_isr` implemented to write characters into screen

3. File System
- Disk driver and disk image implemented on `Makefile`
- FAT32 data structures implemented
- `initialize_filesystem_fat32` implemented to start CRUD operations
- CRUD operations implemented (`read_directory`, `read`, `write`, and `delete`) to modify disk image

4. Bonus
- Unlimited DirectoryEntry to allow a folder having unlimited entries 
- CMOS implemented to add more information on file metadata

### Milestone 3
1. Paging
- Paging Data Structures implemented
- Higher Half Kernel implemented
- Paging activated

2. User Mode
- Inserter implemented
- User GDT & Task State Segment implemented
- Simple Memory Allocator implemented
- Simple User Program implemented
- Launching User Mode implemented

3. Shell
- System Calls implemented according to shell's needs
- Shell implemented
- Built-in Utility Command

4. Bonus
- Relative Pathing
- `clear` command to clear the shell terminal
- added ascii art at startup

Note : Unlimited Directory redacted from Milestone 3 onwards

## References
1. [Intel Manual](https://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-vol-3a-part-1-manual.html.html)
2. [wikiosdev](https://wiki.osdev.org/)
