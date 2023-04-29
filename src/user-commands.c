#include "lib-header/user-commands.h"
#include "lib-header/fat32.h"
#include "lib-header/stdtype.h"
#include "lib-header/user-helper.h"

void command_cd(int argc, char **argv)
{
    // cd only allows "cd a" or "cd"
    if (argc > 2)
    {
        puts("cd: too many arguments\n", BIOS_COLOR_WHITE);
        return;
    }

    if (argc == 1)      // Case "cd", go to root
    {
        memset(shell_state.path, 0, shell_state.path_length);

        shell_state.current_cluster_number  = ROOT_CLUSTER_NUMBER;
        shell_state.path[0]                 = '/';
        shell_state.path_length             = 1;
        return;
    }        

    // Create a copy of argv[i] since strtok changes string
    char path[KEYBOARD_BUFFER_SIZE] = {0};
    memcpy(path, argv[1], strlen(argv[1]));

    // Get final index of path, set is_changing to true to change driver_state.path's value
    uint32_t final_cluster = parse_pathing(path, TRUE, FALSE);

    // Split file name        
    char *full_name;
    char *token = strtok(argv[1], " /");
    while (token != NULL)
    {
        full_name = token;
        token = strtok(NULL, " /");
    }

    char name[8]    = {0};
    char ext[3]     = {0};

    uint32_t i = 0;
    uint32_t index = 0;
    uint32_t is_name = TRUE;
    while (i < strlen(full_name))
    {
        if (full_name[i] == '.')
        {
            index = 0;
            is_name = FALSE;
        }
        else 
        {
            if (is_name)
                name[index] = full_name[i];
            else 
                ext[index] = full_name[i];   

            index++;
        }        
        i++;
    }

    if (final_cluster == 0)     // Not found
    {
        puts("cd: ", BIOS_COLOR_WHITE);

        syscall(5, (uint32_t) name, 8, BIOS_COLOR_WHITE);

        // Check if file has extension
        if (memcmp(ext, "\0\0\0", 3))
        {
            puts(".", BIOS_COLOR_WHITE);
            syscall(5, (uint32_t) ext, 3, BIOS_COLOR_WHITE);
        }

        puts(": No such file or directory\n", BIOS_COLOR_WHITE);
    }
    else
    {
        shell_state.current_cluster_number = final_cluster;
    }
}

void command_ls(int argc, char **argv)
{
    // Can only be either ls or ls dir_name/
    if (argc > 2)
    {
        puts("ls: too many arguments\n", BIOS_COLOR_WHITE);
        return;
    }
    // Create a copy of argv[i] since strtok changes string
    char path[KEYBOARD_BUFFER_SIZE] = {0};
    memcpy(path, argv[1], strlen(argv[1]));

    // Set final cluster; i.e. the cluster number to read from
    uint32_t final_cluster;

    char *full_name = {0};
    char name[8]    = {0};
    char ext[3]     = {0};

    if (argc == 1)
    {
        final_cluster = shell_state.current_cluster_number;
        memcpy(full_name, argv[1], strlen(argv[1]));
    }
    else
    {
        final_cluster = parse_pathing(path, FALSE, FALSE);

        // Split file name        
        char *token = strtok(argv[1], " /");
        while (token != NULL)
        {
            full_name = token;
            token = strtok(NULL, " /");
        }
    }

    uint32_t i = 0;
    uint32_t index = 0;
    uint32_t is_name = TRUE;
    while (i < strlen(full_name))
    {
        if (full_name[i] == '.')
        {
            index = 0;
            is_name = FALSE;
        }
        else 
        {
            if (is_name)
                name[index] = full_name[i];
            else 
                ext[index] = full_name[i];   

            index++;
        }        
        i++;
    }

    if (final_cluster == 0) // Not found
    {
        puts("cd: ", BIOS_COLOR_WHITE);

        syscall(5, (uint32_t) name, 8, BIOS_COLOR_WHITE);

        // Check if file has extension
        if (memcmp(ext, "\0\0\0", 3))
        {
            puts(".", BIOS_COLOR_WHITE);
            syscall(5, (uint32_t) ext, 3, BIOS_COLOR_WHITE);
        }

        puts(": No such file or directory\n", BIOS_COLOR_WHITE);
    }

    // Read cluster
    struct FAT32DirectoryTable dir_table;
    memset(&dir_table, 0, CLUSTER_SIZE);

    syscall(6, (uint32_t) &dir_table, final_cluster, 0);

    // Make an empty entry
    struct FAT32DirectoryEntry entry = {0};

    // Loop through the dir_table, puts if not empty
    i = 1;
    while (i < 0x40)
    {
        if (memcmp(&dir_table.table[i], &entry, sizeof(struct FAT32DirectoryEntry)))
        {
            if (dir_table.table[i].attribute == ATTR_SUBDIRECTORY)  // Check if directory
            {

                syscall(5, (uint32_t) &dir_table.table[i].name, 8, BIOS_COLOR_BLUE);
                puts("\n", BIOS_COLOR_BLUE);
            }
            else 
            {
                syscall(5, (uint32_t) &dir_table.table[i].name, 8, BIOS_COLOR_WHITE);

                // Check if file has extension
                if (memcmp(dir_table.table[i].ext, "\0\0\0", 3))
                {
                    puts(".", BIOS_COLOR_WHITE);
                    syscall(5, (uint32_t) &dir_table.table[i].ext, 3, BIOS_COLOR_WHITE);
                }

                puts("\n", BIOS_COLOR_WHITE);
            }
        }
        i++;
    }
}

void command_mkdir(int argc, char **argv)
{
    // mkdir requires argument
    if (argc == 1)
    {
        puts("mkdir: missing operand\n", BIOS_COLOR_WHITE);
        return;
    }

    // Loop through all args
    for (int i = 1; i < argc; i++)
    {
        // Create a copy of argv[i] since strtok changes string
        char path[KEYBOARD_BUFFER_SIZE] = {0};
        memcpy(path, argv[i], strlen(argv[i]));

        // Create a copyl cluster
        uint32_t final_cluster = parse_pathing(path, FALSE, TRUE);

        // Find file name
        char *name;
        char *token = strtok(argv[i], " /");
        while (token != NULL)
        {
            name = token;
            token = strtok(NULL, " /");
        }

        struct FAT32DriverRequest request = {
            .buf                        = NULL,
            .name                       = "",
            .ext                        = "\0\0\0",
            .parent_cluster_number      = final_cluster,
            .buffer_size                = 0
        };

        memcpy(&request.name, name, strlen(name));
       
        // Call write function, check return code 
        syscall(2, (uint32_t)&request, (uint32_t)&shell_state.retcode, 0);

        if (shell_state.retcode == 0)
        {
            continue;
        }
        else if (shell_state.retcode == 1)   // Directory already exists
        {
            puts("mkdir: cannot create directory `", BIOS_COLOR_WHITE);

            syscall(5, (uint32_t) &request.name, 8, BIOS_COLOR_WHITE);

            // Check if file has extension
            if (memcmp(request.ext, "\0\0\0", 3))
            {
                puts(".", BIOS_COLOR_WHITE);
                syscall(5, (uint32_t) &request.ext, 3, BIOS_COLOR_WHITE);
            }

            puts("`: File exists\n", BIOS_COLOR_WHITE);
        }
        else if (shell_state.retcode == 2)   // current path is not a directory (huh?)
        {
            puts("mkdir: current path is not a directory\n", BIOS_COLOR_WHITE);
        }
        else 
        {
            puts("Unknown error detected\n", BIOS_COLOR_WHITE);
            return;
        }
    }
}

void command_cat(int argc, char **argv)
{
    // cat requires argument
    if (argc == 1)
    {
        puts("cat: missing operand\n", BIOS_COLOR_WHITE);
        return;
    }

    // cat can only have 2 arguments
    if (argc > 2)
    {
        puts("cat: too many arguments\n", BIOS_COLOR_WHITE);
        return;
    }

    // Create a copy of argv[i] since strtok changes string
    char path[KEYBOARD_BUFFER_SIZE] = {0};
    memcpy(path, argv[1], strlen(argv[1]));
    
    // Find file's cluster number
    uint32_t final_cluster = parse_pathing(path, FALSE, TRUE);

    // Split file name        
    char *full_name;
    char *token = strtok(argv[1], " /");
    while (token != NULL)
    {
        full_name = token;
        token = strtok(NULL, " /");
    }

    char name[8]    = {0};
    char ext[3]     = {0};

    uint32_t i = 0;
    uint32_t index = 0;
    uint32_t is_name = TRUE;
    while (i < strlen(full_name))
    {
        if (full_name[i] == '.')
        {
            index = 0;
            is_name = FALSE;
        }
        else 
        {
            if (is_name)
                name[index] = full_name[i];
            else 
                ext[index] = full_name[i];   

            index++;
        }        
        i++;
    }

    uint32_t buffer_size;
    syscall(9, (uint32_t) &buffer_size, final_cluster, 0);

    struct ClusterBuffer cluster = {0};

    struct FAT32DriverRequest request = {
        .buf                    = &cluster,
        .name                   = "",
        .ext                    = "",
        .parent_cluster_number  = final_cluster,
        .buffer_size            = buffer_size * CLUSTER_SIZE
    };

    memcpy(&request.name, name, 8);
    memcpy(&request.ext, ext, 3);

    syscall(0, (uint32_t) &request, (uint32_t) &shell_state.retcode, 0);

    if (shell_state.retcode == 3)
    {
        puts("cat: ", BIOS_COLOR_WHITE);
                syscall(5, (uint32_t) &request.name, 8, BIOS_COLOR_WHITE);

        // Check if file has extension
        if (memcmp(request.ext, "\0\0\0", 3))
        {
            puts(".", BIOS_COLOR_WHITE);
            syscall(5, (uint32_t) &request.ext, 3, BIOS_COLOR_WHITE);
        }

        puts(": No such file or directory\n", BIOS_COLOR_WHITE);
        return;
    }

    if (shell_state.retcode == 2)
    {
        puts("Unknown error occured\n", BIOS_COLOR_WHITE);
    }

    if (shell_state.retcode == 1)
    {
        puts("cat: ", BIOS_COLOR_WHITE);

        syscall(5, (uint32_t) &request.name, 8, BIOS_COLOR_WHITE);

        puts(": Is a directory", BIOS_COLOR_WHITE);
        return;
    }

    if (shell_state.retcode == 0)
    {
        puts((char *) cluster.buf, BIOS_COLOR_WHITE);
    }
}

void command_cp(int argc, char **argv)
{
    if (argc == 1)
    {
        puts("cp: missing file operand\n", BIOS_COLOR_WHITE);
        return;
    }

    if (argc == 2)
    {
        puts("cp: missing destination file operand after '", BIOS_COLOR_WHITE);
        puts(argv[1], BIOS_COLOR_WHITE);
        puts("'\n", BIOS_COLOR_WHITE);
        return;
    }

    if (argc > 3)
    {
        puts("cp: too many arguments\n", BIOS_COLOR_WHITE);
        return;
    }

    // Create a copy of argv[i] since strtok changes string
    char first_path[KEYBOARD_BUFFER_SIZE] = {0};
    memcpy(first_path, argv[1], strlen(argv[1]));
    
    // Find file's cluster number
    uint32_t first_final_cluster = parse_pathing(first_path, FALSE, TRUE);

    // Split file name        
    char *first_full_name;
    char *first_token = strtok(argv[1], " /");
    while (first_token != NULL)
    {
        first_full_name = first_token;
        first_token = strtok(NULL, " /");
    }

    char first_name[8]    = {0};
    char first_ext[3]     = {0};

    uint8_t i = 0;
    uint8_t index = 0;
    uint8_t is_name = TRUE;
    while (i < strlen(first_full_name))
    {
        if (first_full_name[i] == '.')
        {
            index = 0;
            is_name = FALSE;
        }
        else 
        {
            if (is_name)
                first_name[index] = first_full_name[i];
            else 
                first_ext[index] = first_full_name[i];   

            index++;
        }        
        i++;
    }

    // Create another copy of argv[i] since strtok changes string
    char second_path[KEYBOARD_BUFFER_SIZE] = {0};
    memcpy(second_path, argv[2], strlen(argv[2]));
    
    // Find file's cluster number
    uint32_t second_final_cluster = parse_pathing(second_path, FALSE, TRUE);

    char second_name[8];
    char second_ext[3];

    // Split file name        
    char *second_full_name;
    char *second_token = strtok(argv[2], " /");
    while (second_token != NULL)
    {
        second_full_name = second_token;
        second_token = strtok(NULL, " /");
    }

    i = 0;
    index = 0;
    is_name = TRUE;
    while (i < strlen(second_full_name))
    {
        if (second_full_name[i] == '.')
        {
            index = 0;
            is_name = FALSE;
        }
        else 
        {
            if (is_name)
                second_name[index] = second_full_name[i];
            else 
                second_ext[index] = second_full_name[i];   

            index++;
        }        
        i++;
    }

    uint32_t buffer_size;
    syscall(9, (uint32_t) &buffer_size, first_final_cluster, 0);

    struct ClusterBuffer cluster;
    memset(&cluster, '\0', CLUSTER_SIZE);

    struct FAT32DriverRequest request = {
        .buf                    = &cluster,
        .name                   = "",
        .ext                    = "",
        .parent_cluster_number  = first_final_cluster,
        .buffer_size            = buffer_size * CLUSTER_SIZE
    };

    memcpy(&request.name, first_name, 8);
    memcpy(&request.ext, first_ext, 3);

    syscall(0, (uint32_t) &request, (uint32_t) &shell_state.retcode, 0);
   
   if (shell_state.retcode == 3)
    {
        puts("cp: ", BIOS_COLOR_WHITE);

        // Check if file has extension
        if (memcmp(request.ext, "\0\0\0", 3))
        {
            puts(".", BIOS_COLOR_WHITE);
            syscall(5, (uint32_t) &request.ext, 3, BIOS_COLOR_WHITE);
        }

        puts(": No such file or directory\n", BIOS_COLOR_WHITE);
        return;
    }

    if (shell_state.retcode == 2)
    {
        puts("Unknown error occured\n", BIOS_COLOR_WHITE);
    }

    if (shell_state.retcode == 1)
    {
        puts("cp: ", BIOS_COLOR_WHITE);


        puts(argv[1], BIOS_COLOR_WHITE);
        puts(": Is a directory", BIOS_COLOR_WHITE);
        return;
    }

    if (shell_state.retcode == 0)
    {
        memset(&request.name, '\0', 8);
        memcpy(&request.name, second_name, strlen(second_name));

        memset(&request.ext, '\0', 3);
        memcpy(&request.ext, second_ext, strlen(second_ext));

        request.buf = &cluster;
        request.parent_cluster_number = second_final_cluster;
        request.buffer_size = buffer_size * CLUSTER_SIZE;

        syscall(2, (uint32_t) &request, (uint32_t) &shell_state.retcode, 0);
    }
}

void command_rm(int argc, char **argv)
{
    // rm requires argument
    if (argc == 1)
    {
        puts("rm: missing operand\n", BIOS_COLOR_WHITE);
        return;
    }

    // rm can only have 2 arguments
    if (argc > 2)
    {
        puts("rm: too many arguments\n", BIOS_COLOR_WHITE);
        return;
    }

    // Create a copy of argv[i] since strtok changes string
    char path[KEYBOARD_BUFFER_SIZE] = {0};
    memcpy(path, argv[1], strlen(argv[1]));
    
    // Find file's cluster number
    uint32_t final_cluster = parse_pathing(path, FALSE, TRUE);

    // Split file name        
    char *full_name;
    char *token = strtok(argv[1], " /");
    while (token != NULL)
    {
        full_name = token;
        token = strtok(NULL, " /");
    }

    char name[8]    = {0};
    char ext[3]     = {0};

    uint32_t i = 0;
    uint32_t index = 0;
    uint32_t is_name = TRUE;
    while (i < strlen(full_name))
    {
        if (full_name[i] == '.')
        {
            index = 0;
            is_name = FALSE;
        }
        else 
        {
            if (is_name)
                name[index] = full_name[i];
            else 
                ext[index] = full_name[i];   

            index++;
        }        
        i++;
    }

    uint32_t buffer_size;
    syscall(9, (uint32_t) &buffer_size, final_cluster, 0);

    struct ClusterBuffer cluster = {0};

    struct FAT32DriverRequest request = {
        .buf                    = &cluster,
        .name                   = "",
        .ext                    = "",
        .parent_cluster_number  = final_cluster,
        .buffer_size            = buffer_size * CLUSTER_SIZE
    };

    memcpy(&request.name, name, 8);
    memcpy(&request.ext, ext, 3);

    syscall(3, (uint32_t) &request, (uint32_t) &shell_state.retcode, 0);

    if (shell_state.retcode == 2)
    {
        puts("rm: cannot remove '", BIOS_COLOR_WHITE);

        syscall(5, (uint32_t) &request.name, 8, BIOS_COLOR_WHITE);

        // Check if file has extension
        if (memcmp(request.ext, "\0\0\0", 3))
        {
            puts(".", BIOS_COLOR_WHITE);
            syscall(5, (uint32_t) &request.ext, 3, BIOS_COLOR_WHITE);
        }

        puts("': directory not empty\n", BIOS_COLOR_WHITE);
        return;
    }
    if (shell_state.retcode == 1)
    {
        puts("rm: cannot remove '", BIOS_COLOR_WHITE);

        syscall(5, (uint32_t) &request.name, 8, BIOS_COLOR_WHITE);

        // Check if file has extension
        if (memcmp(request.ext, "\0\0\0", 3))
        {
            puts(".", BIOS_COLOR_WHITE);
            syscall(5, (uint32_t) &request.ext, 3, BIOS_COLOR_WHITE);
        }

        puts("': No such file or directory\n", BIOS_COLOR_WHITE);
        return;
    }
}

void command_mv(int argc, char **argv)
{
    // mv requires argument
    if (argc == 1)
    {
        puts("mv: missing operand\n", BIOS_COLOR_WHITE);
        return;
    }

    if (argc == 2)
    {
        puts("mv: missing destination file operand after '", BIOS_COLOR_WHITE);
        puts(argv[1], BIOS_COLOR_WHITE);
        puts("'\n", BIOS_COLOR_WHITE);
        return; 
    }

    // mv can only have 3 arguments
    if (argc > 3)
    {
        puts("mv: too many arguments\n", BIOS_COLOR_WHITE);
        return;
    }

    // Create a copy of argv[i] since strtok changes string
    char first_path[KEYBOARD_BUFFER_SIZE] = {0};
    memcpy(first_path, argv[1], strlen(argv[1]));
    
    // Find file's cluster number
    uint32_t first_final_cluster = parse_pathing(first_path, FALSE, TRUE);

    if (first_final_cluster == 0)
    {
        puts("mv: cannot stat '", BIOS_COLOR_WHITE);
        puts(argv[1], BIOS_COLOR_WHITE);
        puts("': No such file or directory\n", BIOS_COLOR_WHITE);
        return;
    }

    // Split file name        
    char *first_full_name;
    char *first_token = strtok(argv[1], " /");
    while (first_token != NULL)
    {
        first_full_name = first_token;
        first_token = strtok(NULL, " /");
    }

    // Split file name
    char *second_full_name;
    char *second_token = strtok(argv[2], " /");
    while (second_token != NULL)
    {
        second_full_name = second_token;
        second_token = strtok(NULL, " /");
    }



    char first_name[8]    = {0};
    char first_ext[3]     = {0};

    uint32_t i = 0;
    uint32_t index = 0;
    uint32_t is_name = TRUE;
    while (i < strlen(first_full_name))
    {
        if (first_full_name[i] == '.')
        {
            index = 0;
            is_name = FALSE;
        }
        else 
        {
            if (is_name)
                first_name[index] = first_full_name[i];
            else 
                first_ext[index] = first_full_name[i];   

            index++;
        }        
        i++;
    }

    // Get cluster size
    uint32_t buffer_size;
    syscall(9, (uint32_t) &buffer_size, first_final_cluster, 0);

    struct ClusterBuffer cluster = {0};

    struct FAT32DriverRequest request = {
        .buf                    = &cluster,
        .name                   = "",
        .ext                    = "",
        .parent_cluster_number  = first_final_cluster,
        .buffer_size            = buffer_size * CLUSTER_SIZE
    };

    memcpy(&request.name, first_name, 8);
    memcpy(&request.ext, first_ext, 3);

    syscall(0, (uint32_t) &request, (uint32_t) &shell_state.retcode, 0);

    if (shell_state.retcode == 3)       // Not found, try to search dir
    {
        request.buffer_size = 0;

        syscall(1, (uint32_t) &request, (uint32_t) &shell_state.retcode, 0);

        if (shell_state.retcode == 2)   // file and dir not found
        {
            puts("mv: ", BIOS_COLOR_WHITE);
            puts(first_name, BIOS_COLOR_WHITE);
            puts(": No such file or directory\n", BIOS_COLOR_WHITE);
            return;
        }

        if (shell_state.retcode == 1)   // should be impossible
        {            
            puts("mv: ", BIOS_COLOR_WHITE);
            puts(first_name, BIOS_COLOR_WHITE);
            puts(": No such file or directory\n", BIOS_COLOR_WHITE);
            return;
        }
    }

    if (shell_state.retcode == 2)       // Not enough size, should be impossible
    {
        puts("Unknown error occured\n", BIOS_COLOR_WHITE);
        return;
    }

    if (shell_state.retcode == 1)       // Not a file, try to read dir
    {
        request.buffer_size = 0;

        syscall(1, (uint32_t) &request, (uint32_t) &shell_state.retcode, 0);

        if (shell_state.retcode == 2)   // file and dir not found
        {
            puts("mv: ", BIOS_COLOR_WHITE);
            puts(first_name, BIOS_COLOR_WHITE);
            puts(": No such file or directory\n", BIOS_COLOR_WHITE);
            return;
        }

        if (shell_state.retcode == 1)   // should be impossible
        {
            puts("Unknown error occured\n", BIOS_COLOR_WHITE);
        }

        // puts("mv: ", BIOS_COLOR_WHITE);
        // puts(argv[1], BIOS_COLOR_WHITE);
        // puts(": Is a directory", BIOS_COLOR_WHITE);
        // return;
    }

    // if (shell_state.retcode == 0)
    // {
    //     puts((char *) cluster.buf, BIOS_COLOR_WHITE);
    // }

    // Delete first request
    syscall(3, (uint32_t) &request, shell_state.retcode, 0);

    // Create a copy of argv[2] since strtok changes string
    char second_path[KEYBOARD_BUFFER_SIZE] = {0};
    memcpy(second_path, argv[2], strlen(argv[2]));

    // Find file's cluster number
    uint32_t second_final_cluster = parse_pathing(second_path, FALSE, FALSE);

    uint32_t check_cluster = second_final_cluster;
    if (second_final_cluster == 0)
    {
        second_final_cluster = shell_state.current_cluster_number;
        // puts("mv: cannot stat '", BIOS_COLOR_WHITE);
        // puts(argv[2], BIOS_COLOR_WHITE);
        // puts("': No such file or directory\n", BIOS_COLOR_WHITE);
    }

    char second_name[8]     = {0};
    char second_ext[3]      = {0};

    i = 0;
    index = 0;
    is_name = TRUE;
    while (i < strlen(second_full_name))
    {
        if (second_full_name[i] == '.')
        {
            index = 0;
            is_name = FALSE;
        }
        else 
        {
            if (is_name)
                second_name[index] = second_full_name[i];
            else 
                second_ext[index] = second_full_name[i];   

            index++;
        }        
        i++;
    }

    // Get cluster size

    memcpy(&request.name, second_name, 8);
    memcpy(&request.ext, second_ext, 3);
    request.parent_cluster_number = second_final_cluster;

    syscall(3, (uint32_t) &request, (uint32_t) &shell_state.retcode, 0);    // Try to delete, don't care if it exist

    if (check_cluster == 0)  // Not found, we rename
    {
        memcpy(&request.name, second_name, 8);
        memcpy(&request.ext, second_ext, 3);
    }
    else    // Move 
    { 
        memcpy(&request.name, first_name, 8);
        memcpy(&request.ext, first_ext, 3);
    }

    syscall(2, (uint32_t) &request, (uint32_t) &shell_state.retcode, 0);
}

void search_recursive(uint32_t cluster_number, char *name, char *ext, char *path)
{
    struct FAT32DirectoryTable dir_table = {0};

    syscall(6, (uint32_t) &dir_table, cluster_number, 1);
    
    uint32_t i = 1;
    while (i < 0x40)
    {
        if (dir_table.table[i].attribute == ATTR_SUBDIRECTORY)
        {
            char new_path[KEYBOARD_BUFFER_SIZE] = {0};
            memcpy(new_path, path, strlen(path));

            if (!memcmp(dir_table.table[i].name, name, 8) &&
                !memcmp(dir_table.table[i].ext, ext, 3))
            {
                puts(path, BIOS_COLOR_BLUE);
                
                syscall(5, (uint32_t) &dir_table.table[i].name, 8, BIOS_COLOR_BLUE);

                if (new_path[strlen(new_path) - 1] != '/')
                    puts("/", BIOS_COLOR_BLUE);

                puts("\n", BIOS_COLOR_BLUE);
            }

            if (new_path[strlen(new_path) - 1] != '/' && strlen(new_path) != 0)
                new_path[strlen(new_path)] = '/';

            uint32_t j = 0;
            do {
                new_path[strlen(new_path)] = dir_table.table[i].name[j];
                j++;
            } while(j < 8 || dir_table.table[i].name[j] != '\0');

            new_path[strlen(new_path)] = '/';

            uint32_t cluster = (dir_table.table[i].cluster_high << 16) | dir_table.table[i].cluster_low;
            
            search_recursive(cluster, name, ext, new_path);
        }
        else 
        {
            if (!memcmp(dir_table.table[i].name, name, 8) &&
                !memcmp(dir_table.table[i].ext, ext, 3))
            {
                puts(path, BIOS_COLOR_BLUE);
                
                syscall(5, (uint32_t) &dir_table.table[i].name, 8, BIOS_COLOR_WHITE);

                // Check if file has extension
                if (memcmp(dir_table.table[i].ext, "\0\0\0", 3))
                {
                    puts(".", BIOS_COLOR_WHITE);
                    syscall(5, (uint32_t) &dir_table.table[i].ext, 3, BIOS_COLOR_WHITE);
                }

                puts("\n", BIOS_COLOR_WHITE);
            }
        }

        i++;
    }
}

void command_whereis(int argc, char **argv)
{
    if (argc == 1)
    {
        puts("whereis: missing operand\n", BIOS_COLOR_WHITE);
        return;
    }

    if (argc > 2)
    {
        puts("whereis: too many arguments\n", BIOS_COLOR_WHITE);
        return;
    }

    char *full_name;
    char *token = strtok(argv[1], " /");
    while (token != NULL)
    {
        full_name = token;
        token = strtok(NULL, " /");
    }

    char name[8]    = {0};
    char ext[3]     = {0};

    uint32_t i = 0;
    uint32_t index = 0;
    uint32_t is_name = TRUE;
    while (i < strlen(full_name))
    {
        if (full_name[i] == '.')
        {
            index = 0;
            is_name = FALSE;
        }
        else 
        {
            if (is_name)
                name[index] = full_name[i];
            else 
                ext[index] = full_name[i];   

            index++;
        }        
        i++;
    }

    char path[KEYBOARD_BUFFER_SIZE] = {0};

    search_recursive(shell_state.current_cluster_number, name, ext, path);
}