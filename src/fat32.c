#include "lib-header/disk.h"
#include "lib-header/stdtype.h"
#include "lib-header/fat32.h"
#include "lib-header/stdmem.h"

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '3', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

struct FAT32DriverState driver_state;

uint32_t cluster_to_lba(uint32_t cluster)
{
    return cluster * 4;
}

void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster)
{
    memset(dir_table, 0, sizeof(struct FAT32DirectoryTable));
    memcpy(dir_table->table->name, name, 8);
    memset(dir_table->table->ext, ' ', 3);

    dir_table->table->attribute         = ATTR_SUBDIRECTORY;
    dir_table->table->user_attribute    = UATTR_NOT_EMPTY;

    dir_table->table->undelete          = 0;
    dir_table->table->create_time       = 0;
    dir_table->table->create_date       = 0;
    dir_table->table->access_date       = 0;
    dir_table->table->cluster_high      = (uint16_t)((parent_dir_cluster >> 16) & 0xFFFF);

    dir_table->table->modified_time     = 0;
    dir_table->table->modified_date     = 0;
    dir_table->table->cluster_low       = (uint16_t)(parent_dir_cluster & 0xFFFF);
    dir_table->table->filesize          = 0;
}

bool is_empty_storage(void)
{
    uint8_t boot_sector[BLOCK_SIZE];
    read_blocks(boot_sector, 0, 1);

    if (memcmp(boot_sector, fs_signature, BLOCK_SIZE))
        return TRUE;

    return FALSE;
}

void create_fat32(void)
{
    // Write boot sector / cluster 0
    uint8_t boot_sector[CLUSTER_SIZE];
    memset(boot_sector, 0, CLUSTER_SIZE);
    memcpy(boot_sector, fs_signature, BLOCK_SIZE);
    write_clusters(boot_sector, 0, 1);

    // Write cluster 1
    uint32_t reserved[BLOCK_SIZE];
    memset(reserved, 0, BLOCK_SIZE);
    reserved[0] = CLUSTER_0_VALUE;
    reserved[1] = CLUSTER_1_VALUE;
    reserved[2] = FAT32_FAT_END_OF_FILE;
    write_clusters(reserved, FAT_CLUSTER_NUMBER, 1);

    // Write root
    struct FAT32DirectoryTable root_dir;
    init_directory_table(&root_dir, "root", ROOT_CLUSTER_NUMBER);
    uint8_t root_dir_buffer[CLUSTER_SIZE];
    memcpy(root_dir_buffer, &root_dir, sizeof(root_dir));
    write_clusters(root_dir_buffer, ROOT_CLUSTER_NUMBER, 1);
}

void initialize_filesystem_fat32(void)
{
    if (is_empty_storage())
    {
        create_fat32();
    }
    else
    {
        read_clusters(&driver_state, FAT_CLUSTER_NUMBER, 1);
    }
}

void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count)
{
    write_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * 4);
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count)
{
    read_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * 4);
}


/*
int8_t read_directory(struct FAT32DriverRequest request)
{
    int8_t foundIndex=dirtable_linear_search(request.parent_cluster_number);
    if(foundIndex==-1)
        return 2;
    struct FAT32DirectoryTable dir_parent;
    read_clusters(&dir_parent,request.parent_cluster_number,1);

    struct FAT32DirectoryEntry entry;
    entry = dir_parent.table[foundIndex];

    if(entry.attribute != ATTR_SUBDIRECTORY)
    return 1;

    int32_t entrycluster = entry.cluster_high<<16;
    entrycluster += entry.cluster_low;

    read_clusters(request.buf, entrycluster, 1);
    return 0;
    
    return -1;
}

int8_t read(struct FAT32DriverRequest request)
{
    int8_t foundIndex=dirtable_linear_search(request.parent_cluster_number);
    if(foundIndex==-1)
        return 3;

    struct FAT32DirectoryTable dir_parent;
    read_clusters(&dir_parent,request.parent_cluster_number,1);

    struct FAT32DirectoryEntry entry;
    entry = dir_parent.table[foundIndex];
    if(request.buffer_size<entry.filesize)
        return 2;
    if(entry.attribute==ATTR_SUBDIRECTORY)
        return 1;
    
    int32_t entrycluster = entry.cluster_high<<16;
    entrycluster += entry.cluster_low;

    read_clusters(request.buf,entrycluster, entry.filesize);
    return 0;

    return -1;//gatau dah???
}

int8_t dirtable_linear_search(uint8_t parent_cluster_number)
{
    struct FAT32DirectoryTable dir_parent;
    read_clusters(&dir_parent,parent_cluster_number,1);

    for(int i=0; i<64; i++){
        dir_parent.table[i].name;//TODO: check if same with param(add later)
        dir_parent.table[i].ext;
    }
    //if same return index, else return -1;
    return 0;
}
*/