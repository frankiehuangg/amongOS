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

void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster)
{
    memset(dir_table, 0, sizeof(struct FAT32DirectoryTable));
    memcpy(dir_table->table->name, name, 8);
    memset(dir_table->table->ext, ' ', 3);

    dir_table->table->attribute         = ATTR_SUBDIRECTORY;
    dir_table->table->user_attribute    = UATTR_NOT_EMPTY;

    read_rtc();
    uint16_t create_time = (current_data.hour << 11) | (current_data.minute << 5) | (current_data.second / 2);
    uint16_t create_date = ((current_data.year - UNIX_START_YEAR) << 9) | (current_data.month << 5) | current_data.day;
    // Y Y Y Y Y Y Y M M M M D D D D D 
    // H H H H M M M M M M S S S S S S

    dir_table->table->undelete          = 0;
    dir_table->table->create_time       = create_time;
    dir_table->table->create_date       = create_date;
    dir_table->table->access_date       = create_date;
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
    init_directory_table(&root_dir, "root\0\0\0\0", ROOT_CLUSTER_NUMBER);
    uint8_t root_dir_buffer[CLUSTER_SIZE];
    memcpy(root_dir_buffer, &root_dir, sizeof(root_dir));
    write_clusters(&root_dir, ROOT_CLUSTER_NUMBER, 1);
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



int8_t read_directory(struct FAT32DriverRequest request)
{

    int8_t foundIndex=dirtable_linear_search(request.parent_cluster_number, request);
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
    int8_t foundIndex=dirtable_linear_search(request.parent_cluster_number, request);
    if(foundIndex==-1)
        return 3;

    struct FAT32DirectoryTable dir_parent;
    read_clusters(&dir_parent,request.parent_cluster_number,1);
    struct FAT32FileAllocationTable fat;
    read_clusters(&fat,FAT_CLUSTER_NUMBER,1);

    struct FAT32DirectoryEntry entry;
    entry = dir_parent.table[foundIndex];
    if(request.buffer_size<entry.filesize)
        return 2;
    if(entry.attribute==ATTR_SUBDIRECTORY)
        return 1;
    
    int32_t entrycluster = entry.cluster_high<<16;
    entrycluster += entry.cluster_low;

    int32_t test;
    int32_t cur_cluster=entrycluster;
    int16_t count=0;

    test = fat.cluster_map[cur_cluster];
    while(test!=FAT32_FAT_END_OF_FILE)
    {
        read_clusters(request.buf+count*CLUSTER_SIZE,cur_cluster,1);
        cur_cluster=test;
        test=fat.cluster_map[cur_cluster];
        count++;
    }
    read_clusters(request.buf+count*CLUSTER_SIZE,cur_cluster,1);

    return 0;

    return -1;//gatau dah???
}

int32_t dirtable_linear_search(uint32_t parent_cluster_number, struct FAT32DriverRequest entry)
{
    struct FAT32DirectoryTable dir_parent;
    read_clusters(&dir_parent,parent_cluster_number,1);

    struct FAT32FileAllocationTable fat;
    read_clusters(&fat,FAT_CLUSTER_NUMBER,1);

    int32_t test;
    int32_t cur_cluster=parent_cluster_number;
    int32_t count=0;

    test = fat.cluster_map[cur_cluster];
    while(test!=FAT32_FAT_END_OF_FILE)
    {
        // check table
        for(int i=1; i<64; i++){
            char* cur_name = dir_parent.table[i].name;
            char* cur_ext = dir_parent.table[i].ext;
            if(!memcmp(cur_name, entry.name, 8)){
                if(entry.buffer_size==0)
                    return i + count*64;
                if(!memcmp(cur_ext, entry.ext, 3))
                    return i + count*64;
            }
        }
        cur_cluster=test;
        read_clusters(&dir_parent,cur_cluster,1);
        test=fat.cluster_map[cur_cluster];
        count++;
    }

    // check last table
    for(int i=1; i<64; i++){
        char* cur_name = dir_parent.table[i].name;
        char* cur_ext = dir_parent.table[i].ext;
        if(!memcmp(cur_name, entry.name, 8)){
            if(entry.buffer_size==0)
                return i + count*64;
            if(!memcmp(cur_ext, entry.ext, 3))
                return i + count*64;
        }
    }
    return -1;
}

int8_t write(struct FAT32DriverRequest request)
{
    struct FAT32FileAllocationTable fat;
    read_clusters(&fat,FAT_CLUSTER_NUMBER,1);

    struct FAT32DirectoryTable dir_cur;
    read_clusters(&dir_cur,request.parent_cluster_number,1);
    if(!check_dir_valid(request.parent_cluster_number))
        return 2;
    if(is_file_exists(request.parent_cluster_number,request))
        return 1;
    
    uint32_t size_to_allocate = request.buffer_size/CLUSTER_SIZE;
    if(request.buffer_size % CLUSTER_SIZE!=0) size_to_allocate +=1;
    int16_t count=0;
    int16_t i_before=0;
    int16_t i_start=-1;
    for(int i=3; i<512;i++)
    {
        if(!size_to_allocate&&request.buffer_size!=0) break;
        if(fat.cluster_map[i]!=0)
            continue;
        if(i_start==-1)
            i_start=i;
        
        if(request.buffer_size==0)
        {
            i_before=i;
            struct FAT32DirectoryTable temp;
            init_directory_table(&temp, request.name,request.parent_cluster_number);
            write_clusters(&temp,i,1);
            break;
        }
        else
        {
            write_clusters(request.buf+count*CLUSTER_SIZE,i,1);
            
            size_to_allocate-=1;

            if(count!=0)
                fat.cluster_map[i_before]=i;

            count+=1;
            i_before=i;
        }
        
    }
    fat.cluster_map[i_before]=FAT32_FAT_END_OF_FILE;

    // add to directory
    add_to_dir_table(request.parent_cluster_number, request, i_start);
    write_clusters(&fat,1,1);

    return 0;
    return -1; //ntar
}

bool check_dir_valid(uint32_t parent_cluster_number)
{
    
    uint32_t cur_cluster= parent_cluster_number;
    struct FAT32FileAllocationTable fat;
    read_clusters(&fat,FAT_CLUSTER_NUMBER,1);

    struct FAT32DirectoryTable dir_cur;
    read_clusters(&dir_cur,cur_cluster,1);
    
    return(dir_cur.table->attribute==ATTR_SUBDIRECTORY);
    
    /*
    uint16_t counter=0;
    bool valid=TRUE;

    while(valid)
    {
        if(cur_cluster==ROOT_CLUSTER_NUMBER)
            break;
        if(fat.cluster_map[cur_cluster]!=FAT32_FAT_END_OF_FILE)
        {
            valid=FALSE;
            break;
        } 
        struct FAT32DirectoryTable dir_cur;
        read_clusters(&dir_cur,cur_cluster,1);
        counter++;
        if(dir_cur.table->attribute!=ATTR_SUBDIRECTORY||counter>513){
            valid=FALSE;
            break;
        }
        cur_cluster = dir_cur.table->cluster_high<<16;
        cur_cluster |= dir_cur.table->cluster_low;
    }
    return valid;
    */
}

bool is_file_exists(uint32_t parent_cluster_number, struct FAT32DriverRequest entry)
{
    int8_t test=dirtable_linear_search(parent_cluster_number, entry);
    return test!=-1;
}

void add_to_dir_table(uint32_t parent_cluster_number, struct FAT32DriverRequest entry,int16_t entry_cluster)
{
    struct FAT32DirectoryTable dir_cur;
    read_clusters(&dir_cur,parent_cluster_number,1);
    bool added=FALSE;

    for(int i=1;i<64;i++)
    {
        if(dir_cur.table[i].user_attribute!=UATTR_NOT_EMPTY)
        {
            if(entry.buffer_size!=0)
                dir_cur.table[i].attribute=0;
            else
                dir_cur.table[i].attribute=ATTR_SUBDIRECTORY;

            if(entry.buffer_size!=0)
                memcpy(dir_cur.table[i].ext,entry.ext,3);
            
            dir_cur.table[i].access_date=0;
           

            dir_cur.table[i].cluster_high=0;
            dir_cur.table[i].cluster_low=entry_cluster;
            dir_cur.table[i].create_date=0;
            dir_cur.table[i].create_time=0;
            
            memcpy(dir_cur.table[i].name,entry.name,8);
            dir_cur.table[i].filesize=entry.buffer_size;
            dir_cur.table[i].modified_date=0;
            dir_cur.table[i].modified_time=0;
            dir_cur.table[i].undelete=0;
            dir_cur.table[i].user_attribute=UATTR_NOT_EMPTY;
            added=TRUE;

            write_clusters(&dir_cur,parent_cluster_number,1);
            break;
        }
    }
    if(!added)
    {
        struct FAT32FileAllocationTable fat;
        read_clusters(&fat,FAT_CLUSTER_NUMBER,1);

        for(int i=3; i<512;i++)
        {
            if(fat.cluster_map[i]!=0 || fat.cluster_map[i]==FAT32_FAT_END_OF_FILE)
                continue;
            struct FAT32DirectoryTable temp;
            init_directory_table(&temp, dir_cur.table[i].name,parent_cluster_number);
            write_clusters(&temp,i,1);

            fat.cluster_map[i]=FAT32_FAT_END_OF_FILE;
            fat.cluster_map[parent_cluster_number]=i;
            write_clusters(&fat,1,1);
            add_to_dir_table(i, entry, entry_cluster);
            break;            
        }    
    }
    
}

int8_t delete(struct FAT32DriverRequest request)
{
    struct FAT32FileAllocationTable fat;
    read_clusters(&fat,FAT_CLUSTER_NUMBER,1);

    struct FAT32DirectoryTable dir_cur;
    read_clusters(&dir_cur,request.parent_cluster_number,1);

    //struct ClusterBuffer cbuf;
    //memset(cbuf.buf,0,CLUSTER_SIZE);
    if(!check_dir_valid(request.parent_cluster_number))
        return 1;
    if(!is_file_exists(request.parent_cluster_number,request))
       return 1;

    if(request.buffer_size==0){
        int16_t ind = dirtable_linear_search(request.parent_cluster_number, request);
        int32_t cluster_number=dir_cur.table[ind].cluster_low;
        if(check_dir_has_file(cluster_number))
            return 2;
        memset(&dir_cur.table[ind],0,sizeof(struct FAT32DirectoryEntry));
        //memset(&fat.cluster_map[cluster_number],0,sizeof(int32_t)); 

        // NEW
        int32_t test;
        int32_t cur_cluster=cluster_number;
        test = fat.cluster_map[cur_cluster];
        while(test!=FAT32_FAT_END_OF_FILE)
        {
            memset(&fat.cluster_map[cur_cluster],0,sizeof(int32_t)); 
            cur_cluster=test;
            test=fat.cluster_map[cur_cluster];
        }
        memset(&fat.cluster_map[cur_cluster],0,sizeof(int32_t)); 
        //write_clusters(cbuf.buf,cluster_number,1);
    }
    else{
        int16_t ind = dirtable_linear_search(request.parent_cluster_number, request);
        int32_t cluster_number=dir_cur.table[ind].cluster_low;
        memset(&dir_cur.table[ind],0,sizeof(struct FAT32DirectoryEntry));
        int32_t test;
        int32_t cur_cluster=cluster_number;
        test = fat.cluster_map[cur_cluster];
        while(test!=FAT32_FAT_END_OF_FILE)
        {
            memset(&fat.cluster_map[cur_cluster],0,sizeof(int32_t)); 
            cur_cluster=test;
            test=fat.cluster_map[cur_cluster];
        }
        memset(&fat.cluster_map[cur_cluster],0,sizeof(int32_t)); 
    }
    write_clusters(&fat,FAT_CLUSTER_NUMBER,1);
    write_clusters(&dir_cur,request.parent_cluster_number,1);
    return 0;
    return -1;//????
}


bool check_dir_has_file(uint32_t cluster_number)
{
    struct FAT32DirectoryTable dir_cur;
    read_clusters(&dir_cur,cluster_number,1);

    struct FAT32FileAllocationTable fat;
    read_clusters(&fat,FAT_CLUSTER_NUMBER,1);

    int32_t test;
    int32_t cur_cluster=cluster_number;
    int32_t count=0;

    test = fat.cluster_map[cur_cluster];
    while(test!=FAT32_FAT_END_OF_FILE)
    {
        // check table
        for(int i=1;i<64;i++){
            if(dir_cur.table[i].user_attribute==UATTR_NOT_EMPTY)
                return TRUE;
        }
        cur_cluster=test;
        read_clusters(&dir_cur,cur_cluster,1);
        test=fat.cluster_map[cur_cluster];
        count++;
    }
    for(int i=1;i<64;i++){
        if(dir_cur.table[i].user_attribute==UATTR_NOT_EMPTY)
            return TRUE;
    }

    return FALSE;
}
