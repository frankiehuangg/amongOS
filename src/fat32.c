#include "lib-header/disk.h"
#include "lib-header/stdtype.h"
#include "lib-header/fat32.h"
#include "lib-header/stdmem.h"
#include "lib-header/cmos.h"

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

void set_directory_entry_time_and_date(struct FAT32DirectoryEntry *entry, uint8_t level)
{
    // Level:
    // 0 - Create
    // 1 - Access
    // 2 - Modify
    
    read_rtc();
    uint16_t time = (current_data.hour << 11) | (current_data.minute << 5) | (current_data.second / 2);
    uint16_t date = ((current_data.year - UNIX_START_YEAR) << 9) | (current_data.month << 5) | current_data.day;

    if (level == 0)
    {
        entry->create_time     = time;
        entry->create_date     = date;
        entry->access_date     = date;
        entry->modified_date   = date;
        entry->modified_time   = time;
    }
    else if (level == 2)
    {
        entry->access_date     = date;
    }
}

void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster)
{
    memset(dir_table, 0, sizeof(struct FAT32DirectoryTable));

    memcpy(dir_table->table[0].name, name, 8);
    memset(dir_table->table[0].ext, '\0', 3);

    dir_table->table[0].attribute         = ATTR_SUBDIRECTORY;
    dir_table->table[0].user_attribute    = 0;

    dir_table->table[0].undelete          = 0;
    dir_table->table[0].create_time       = 0;
    dir_table->table[0].create_date       = 0;
    dir_table->table[0].access_date       = 0;
    dir_table->table[0].cluster_high      = (uint16_t)((parent_dir_cluster >> 16) & 0xFFFF);

    dir_table->table[0].modified_time     = 0;
    dir_table->table[0].modified_date     = 0;
    dir_table->table[0].cluster_low       = (uint16_t)(parent_dir_cluster & 0xFFFF);
    dir_table->table[0].filesize          = 0;

    set_directory_entry_time_and_date(&dir_table->table[0], 0);
}

bool is_empty_storage(void)
{
    read_clusters(&driver_state.cluster_buf, BOOT_SECTOR, 1);

    if (memcmp(&driver_state.cluster_buf, fs_signature, BLOCK_SIZE))
        return TRUE;
    return FALSE;
}

void create_fat32(void)
{
    // Clear FAT32DriverState
    memset(&driver_state, 0, sizeof(struct FAT32DriverState));

    // Write boot sector
    memcpy(&driver_state.cluster_buf, fs_signature, BLOCK_SIZE);
    write_clusters(&driver_state.cluster_buf, BOOT_SECTOR, 1);

    // Write cluster 1
    // Update FAT32DriverState
    driver_state.fat_table.cluster_map[0] = CLUSTER_0_VALUE;
    driver_state.fat_table.cluster_map[1] = CLUSTER_1_VALUE;
    driver_state.fat_table.cluster_map[2] = FAT32_FAT_END_OF_FILE;
    write_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);

    // Write root cluster
    init_directory_table(&driver_state.dir_table_buf, "root\0\0\0\0", ROOT_CLUSTER_NUMBER);
    write_clusters(&driver_state.dir_table_buf, ROOT_CLUSTER_NUMBER, 1);
}

void initialize_filesystem_fat32(void)
{
    if (is_empty_storage())
    {
        create_fat32();
    }
    else
    {
        memset(&driver_state, 0, sizeof(struct FAT32DriverState));

        read_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
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

int8_t read_directory(struct FAT32DriverRequest request)
{
    // Index
    uint32_t index = dirtable_linear_search(request, FALSE);
    uint32_t parent_cluster_number = request.parent_cluster_number;

    if (index == 0) // Not found
        return 2;

    // DirectoryEntry
    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
    struct FAT32DirectoryEntry entry = driver_state.dir_table_buf.table[index];

    if (entry.attribute != ATTR_SUBDIRECTORY)       // Not a folder
        return 1;

    // Copy content to request
    struct FAT32DirectoryTable *dir_table = request.buf;
    read_clusters(dir_table, ((entry.cluster_high << 16) | entry.cluster_low), 1);

    // Update access date
    read_clusters(&driver_state.dir_table_buf, parent_cluster_number, 1);
    set_directory_entry_time_and_date(&driver_state.dir_table_buf.table[index], 1);
    write_clusters(&driver_state.dir_table_buf, parent_cluster_number, 1);
    
    return 0;

    return -1;
}

int8_t read(struct FAT32DriverRequest request)
{
    // Index
    uint32_t index = dirtable_linear_search(request, TRUE);

    if (index == 0) // Not found
        return 3;

    // DirectoryEntry
    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
    struct FAT32DirectoryEntry entry = driver_state.dir_table_buf.table[index];

    if (entry.attribute == ATTR_SUBDIRECTORY)       // Not a file
        return 1;

    if (request.buffer_size < entry.filesize)       // Not enough size
        return 2;

    uint32_t cluster = (entry.cluster_high << 16) | entry.cluster_low;

    // Read fat_table
    read_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);

    uint32_t count = 0;
    do {
        read_clusters(request.buf + count * CLUSTER_SIZE, cluster, 1);

        cluster = driver_state.fat_table.cluster_map[cluster];

        count++;
    } while (cluster != FAT32_FAT_END_OF_FILE);

    // Update access date
    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
    set_directory_entry_time_and_date(&driver_state.dir_table_buf.table[index], 1);
    write_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

    return 0;

    return -1;
}

int8_t write(struct FAT32DriverRequest request)
{
    if (!is_cluster_folder(request.parent_cluster_number))      // Check if parent_cluster_number is a folder cluster
        return 2;

    // Find request index
    uint32_t index;
    if (request.buffer_size == 0)
        index = dirtable_linear_search(request, FALSE);
    else
        index = dirtable_linear_search(request, TRUE);

    if (index != 0)     // File / folder exists
        return 1;


    if (request.buffer_size == 0)
        write_directory(request.parent_cluster_number, request.name);
    else
        write_file(request);

    update_directory_entry(request.parent_cluster_number);
    
    return 0;

    return -1;
}

int8_t delete(struct FAT32DriverRequest request)
{
    // Find request index
    uint32_t index;
    if (request.buffer_size == 0)
        index = dirtable_linear_search(request, FALSE);
    else
        index = dirtable_linear_search(request, TRUE);

    if (index == 0)     // File / folder not exists
        return 1;

    read_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

    uint32_t cluster = (driver_state.dir_table_buf.table[index].cluster_high << 16) | driver_state.dir_table_buf.table[index].cluster_low;

    if (request.buffer_size == 0)
    {
        struct FAT32DirectoryEntry entry = driver_state.dir_table_buf.table[index];
        if (entry.user_attribute != UATTR_NOT_EMPTY)
        {
            memset(&driver_state.dir_table_buf.table[index], 0, sizeof(struct FAT32DirectoryEntry));
        
            driver_state.fat_table.cluster_map[cluster] = 0;
        }
        else 
        {
            return 2;
        }
    }
    else 
    {
        memset(&driver_state.dir_table_buf.table[index], 0, sizeof(struct FAT32DirectoryEntry));

        while (driver_state.fat_table.cluster_map[cluster] != FAT32_FAT_END_OF_FILE)
        {
            uint32_t prev_cluster = cluster;
            cluster = driver_state.fat_table.cluster_map[cluster];
            driver_state.fat_table.cluster_map[prev_cluster] = 0;
        }

        driver_state.fat_table.cluster_map[cluster] = 0;
    }

    write_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
    write_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

    return 0;

    return -1;
}

uint32_t dirtable_linear_search(struct FAT32DriverRequest request, uint8_t is_file)
{
    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

    uint32_t size = CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry);
    
    if (is_file)
    {
        for (uint32_t i = 1; i < size; i++)
        {
            if (!memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8) &&
                !memcmp(driver_state.dir_table_buf.table[i].ext, request.ext, 3))
                {
                    return i;
                } 
        }
    }
    else 
    {
        for (uint32_t i = 1; i < size; i++)
        {
            if (!memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8))
            {
                return i;
            }
        }
    }

    return 0;
}

uint32_t get_cluster_number(uint32_t index, uint32_t parent_cluster_number)
{
    read_clusters(&driver_state.dir_table_buf, parent_cluster_number, 1);

    return (driver_state.dir_table_buf.table[index].cluster_high << 16) | driver_state.dir_table_buf.table[index].cluster_low;
}

uint32_t get_cluster_size(uint32_t cluster_number)
{
    read_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);

    uint32_t current_cluster = cluster_number;

    uint32_t count = 1;
    while (driver_state.fat_table.cluster_map[current_cluster] != FAT32_FAT_END_OF_FILE)
    {
        current_cluster = driver_state.fat_table.cluster_map[current_cluster];
        count++;
    }

    return count;
}

uint8_t is_cluster_folder(uint32_t parent_cluster_number)
{
    read_clusters(&driver_state.dir_table_buf, parent_cluster_number, 1);

    // Check 0th index, check if it's a directory
    // Directory -> entry.ext nonexistant, entry.attribute ATTR_SUBDIRECTORY, entry.filesize 0
    struct FAT32DirectoryEntry entry = driver_state.dir_table_buf.table[0];

    if (memcmp(&entry.ext, "\0\0\0", 3))      // Check if ext nonexistant
        return FALSE;
    if (entry.attribute != ATTR_SUBDIRECTORY)                // Check if attribute is not a subdirectory
        return FALSE;
    if (entry.filesize != 0)
        return FALSE;
    
    return TRUE;                                 // Check if size is not 0
}

uint32_t find_empty_dir_table_index(uint32_t parent_cluster_number)
{
    read_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
    read_clusters(&driver_state.dir_table_buf, parent_cluster_number, 1);
    memset(&driver_state.cluster_buf, '\0', CLUSTER_SIZE);

    uint32_t size = CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry);
    uint32_t index = 0;

    while (index < size)
    {
        if (! memcmp(&driver_state.dir_table_buf.table[index],
                    &driver_state.cluster_buf, sizeof(struct FAT32DirectoryEntry)))
            return index;
        index++;
    }

    return 0;
}

void write_directory(uint32_t parent_cluster_number, char* name)
{
    read_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);

    // Find next empty cluster index
    uint32_t cluster_number = 0;
    do {
        cluster_number++;
    } while (driver_state.fat_table.cluster_map[cluster_number] != FAT32_FAT_EMPTY_ENTRY);

    // Find next empty directory table index
    uint32_t empty_index = find_empty_dir_table_index(parent_cluster_number);

    // Create empty folder, add to memory
    init_directory_table(&driver_state.dir_table_buf, name, parent_cluster_number);
    write_clusters(&driver_state.dir_table_buf, cluster_number, 1);

    // Create new entry
    struct FAT32DirectoryEntry entry;
    memset(&entry, '\0', sizeof(struct FAT32DirectoryEntry));

    memcpy(&entry.name, name, 8);
    memset(&entry.ext, '\0', 3);

    entry.attribute         = ATTR_SUBDIRECTORY;
    entry.user_attribute    = 0;

    entry.undelete          = 0;
    entry.create_time       = 0;
    entry.create_date       = 0;
    entry.access_date       = 0;
    entry.cluster_high      = (uint16_t)((cluster_number >> 16) & 0xFFFF);

    entry.modified_time     = 0;
    entry.modified_date     = 0;
    entry.cluster_low       = (uint16_t)(cluster_number & 0xFFFF);
    entry.filesize          = 0;

    // Update create time and date
    set_directory_entry_time_and_date(&entry, 0);

    // Insert entry to table

    read_clusters(&driver_state.dir_table_buf, parent_cluster_number, 1);
    driver_state.dir_table_buf.table[empty_index] = entry;
    write_clusters(&driver_state.dir_table_buf, parent_cluster_number, 1);

    // Write last part of buffer, set cluster map to FAT32_FAT_END_OF_FILE
    driver_state.fat_table.cluster_map[cluster_number] = FAT32_FAT_END_OF_FILE;

    // Update the fat table
    write_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
}

void write_file(struct FAT32DriverRequest request)
{
    read_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);

    // Find next empty cluster index
    uint32_t cluster_number = 0;
    while(driver_state.fat_table.cluster_map[cluster_number] != FAT32_FAT_EMPTY_ENTRY) cluster_number++;

    uint32_t first_cluster = cluster_number;

    // Find next empty directory table index
    uint32_t empty_index = find_empty_dir_table_index(request.parent_cluster_number);

    // Move buf to buffer, begin writing
    uint32_t size = request.buffer_size / CLUSTER_SIZE;
    if (request.buffer_size % CLUSTER_SIZE != 0) size++;

    struct ClusterBuffer buffer[size];
    memcpy(&buffer, request.buf, request.buffer_size);

    uint32_t i = 0;
    while (i < size - 1)
    {
        write_clusters(&buffer[i], cluster_number, 1);

        uint32_t previous_cluster = cluster_number;
        do {
            cluster_number++;
        } while (driver_state.fat_table.cluster_map[cluster_number] != 0);

        driver_state.fat_table.cluster_map[previous_cluster] = cluster_number;

        i++;
    }

    write_clusters(&buffer[i], cluster_number, 1);

    // Create new entry
    struct FAT32DirectoryEntry entry;
    memset(&entry, '\0', sizeof(struct FAT32DirectoryEntry));

    memcpy(&entry.name, request.name, 8);
    memcpy(&entry.ext, request.ext, 3);

    entry.attribute         = 0;
    entry.user_attribute    = 0;

    entry.undelete          = 0;
    entry.create_time       = 0;
    entry.create_date       = 0;
    entry.access_date       = 0;
    entry.cluster_high      = (uint16_t)((first_cluster >> 16) & 0xFFFF);

    entry.modified_date     = 0;
    entry.modified_date     = 0;
    entry.cluster_low       = (uint16_t)(first_cluster & 0xFFFF);
    entry.filesize          = request.buffer_size;

    // Update create time and date
    set_directory_entry_time_and_date(&entry, 0);

    // Insert entry to table
    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
    driver_state.dir_table_buf.table[empty_index] = entry;
    write_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

    // Write last part of buffer, set cluster map to FAT32_FAT_END_OF_FILE
    driver_state.fat_table.cluster_map[cluster_number] = FAT32_FAT_END_OF_FILE;

    // Update the fat_table
    write_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
}

void update_directory_entry(uint32_t parent_cluster_number)
{
    read_clusters(&driver_state.dir_table_buf, parent_cluster_number, 1);

    // Get parent_cluster_number's parent cluster
    uint32_t parent_cluster = (driver_state.dir_table_buf.table[0].cluster_high << 16) | driver_state.dir_table_buf.table[0].cluster_low;

    // Set user_attribute to UATTR_NOT_EMPTY
    read_clusters(&driver_state.dir_table_buf, parent_cluster, 1);
    driver_state.dir_table_buf.table[0].user_attribute = UATTR_NOT_EMPTY;
    write_clusters(&driver_state.dir_table_buf, parent_cluster, 1);
}