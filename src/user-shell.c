#include "lib-header/stdtype.h"
#include "lib-header/fat32.h"

#define NULL 0

#define BIOS_COLOR_GREEN    0x0A
#define BIOS_COLOR_CYAN     0x0B
#define BIOS_COLOR_RED      0x0C
#define BIOS_COLOR_WHITE    0x0F

#define KEYBOARD_BUFFER_SIZE    256

char *strtok(char *str, const char *delim)
{
    static char *last_token = NULL;
    char *token;
    const char *d;

    if (str != NULL)
        last_token = str;
    else if (last_token == NULL)
        return NULL;

    token = last_token;
    while (*token != '\0')
    {
        d = delim;
        while (*d != '\0')
        {
            if (*token == *d)
            {
                token++;
                break;
            }
            d++;
        }

        if (*d == '\0')
            break;
    }

    if (*token == '\0')
    {
        last_token = NULL;
        return NULL;
    }

    last_token = token;
    while (*last_token != '\0')
    {
        d = delim;
        while (*d != '\0')
        {
            if (*last_token == *d)
            {
                *last_token = '\0';
                last_token++;
                return token;
            }
            d++;
        }
        last_token++;
    }

    return token;
}

uint32_t strlen(const char *str)
{
    const char *s;

    for (s = str; *s; ++s);

    return (s - str);
}

void* memset(void *s, int c, size_t n) {
    uint8_t *buf = (uint8_t*) s;
    for (size_t i = 0; i < n; i++)
        buf[i] = (uint8_t) c;
    return s;
}

void* memcpy(void* restrict dest, const void* restrict src, size_t n) {
    uint8_t *dstbuf       = (uint8_t*) dest;
    const uint8_t *srcbuf = (const uint8_t*) src;
    for (size_t i = 0; i < n; i++)
    {
        if (i < strlen(src))
            dstbuf[i] = srcbuf[i];
        else
            dstbuf[i] = '\0';
    }
    return dstbuf;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const uint8_t *buf1 = (const uint8_t*) s1;
    const uint8_t *buf2 = (const uint8_t*) s2;
    for (size_t i = 0; i < n; i++) {
        if (buf1[i] < buf2[i])
            return -1;
        else if (buf1[i] > buf2[i])
            return 1;
    }

    return 0;
}

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx)
{
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));

    __asm__ volatile("int $0x30");
}

void puts(char* buf, uint8_t color)
{
    syscall(5, (uint32_t) buf, strlen(buf), color);
}

struct ShellState
{
    uint32_t                    current_cluster_number;
    uint32_t                    cluster_number;
    uint32_t                    retcode;
    uint32_t                     path_length;

    char*                       path;
}__attribute__((packed));

struct ShellState shell_state = {
    .current_cluster_number     = ROOT_CLUSTER_NUMBER,
    .path_length                = 1,
    .path                       = "/",
};

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
        shell_state.current_cluster_number  = ROOT_CLUSTER_NUMBER;
        shell_state.path                    = "/";
        return;
    }        

    struct FAT32DirectoryTable dir_table;
    memset(&dir_table, '\0', CLUSTER_SIZE);

    struct FAT32DriverRequest request = {
        .buf                    = &dir_table,
        .name                   = "",
        .ext                    = "\0\0\0",
        .parent_cluster_number  = shell_state.current_cluster_number,
        .buffer_size            = 0
    };
    memcpy(request.name, argv[1], 8);

    syscall(1, (uint32_t) &request, (uint32_t) &shell_state.retcode, 0);

    if (!memcmp(argv[1], "..", strlen(argv[1])) || !memcmp(argv[1], "../", strlen(argv[1])))
    {
        shell_state.current_cluster_number  = (dir_table.table[0].cluster_high << 16) | dir_table.table[0].cluster_low;

        // If not at root, change path string
        if (shell_state.current_cluster_number != ROOT_CLUSTER_NUMBER)
        {
            do {
                shell_state.path[shell_state.path_length - 1] = '\0';
                shell_state.path_length--;
            } while (shell_state.path[shell_state.path_length] != '/');
        }

        return;
    }

    if (shell_state.retcode == 0)
    {
        shell_state.current_cluster_number = (dir_table.table[0].cluster_high << 16) | dir_table.table[0].cluster_low;

        uint32_t i = 0;
        do {
            shell_state.path[shell_state.path_length++] = argv[1][i++];
        } while (i < strlen((argv[1])));

        shell_state.path[shell_state.path_length] = '/';
        shell_state.path_length++;
    }
    else if (shell_state.retcode == 1)      // Not a directory
    {
        puts("cd: ", BIOS_COLOR_WHITE);
        puts(argv[1], BIOS_COLOR_WHITE);
        puts(": Not a directory\n", BIOS_COLOR_WHITE);
    }
    else if (shell_state.retcode == 2)      // Not found
    {
        puts("cd: ", BIOS_COLOR_WHITE);
        puts(argv[1], BIOS_COLOR_WHITE);
        puts(": No such file or directory\n", BIOS_COLOR_WHITE);
    }
    else    // Error
    {
        puts("Unknown error detected\n", BIOS_COLOR_WHITE);
        return;
    }
}

void command_mkdir(int argc, char **argv)
{
    if (argc == 1)
    {
        puts("mkdir: missing operand\n", BIOS_COLOR_WHITE);
        return;
    }

    struct ClusterBuffer cluster;
    memset(&cluster, '\00', CLUSTER_SIZE);

    for (int i = 1; i < argc; i++)
    {
        struct FAT32DriverRequest request = {
            .buf                        = &cluster,
            .name                       = "",
            .ext                        = "\0\0\0",
            .parent_cluster_number      = shell_state.current_cluster_number,
            .buffer_size                = 0
        };

        memcpy(&request.name, argv[i], 8);
       
        syscall(2, (uint32_t)&request, (uint32_t)&shell_state.retcode, 0);

        if (shell_state.retcode == 0)
        {
            continue;
        }
        else if (shell_state.retcode == 1)   // Directory already exists
        {
            puts("mkdir: cannot create directory `", BIOS_COLOR_WHITE);
            puts(argv[i], BIOS_COLOR_WHITE);
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

int main(void)
{
    char buf[KEYBOARD_BUFFER_SIZE];
    char* token;
    char* argv[16];

    while (TRUE)
    {
        puts("IF2230@among", BIOS_COLOR_RED);
        puts("OS", BIOS_COLOR_CYAN);

        syscall(5, (uint32_t) shell_state.path, shell_state.path_length, BIOS_COLOR_GREEN);

        puts("$ ", BIOS_COLOR_GREEN);

        syscall(4, (uint32_t)buf, sizeof(buf), 0);
    
        int argc = 0;

        token = strtok(buf, " \n");
        while (token != NULL)
        {
            argv[argc++] = token;
            token = strtok(NULL, " \n");
        }
        argv[argc] = NULL;

        if (argc > 0)
        {
            int command_length = strlen(argv[0]);
            if (!memcmp(argv[0], "cd", command_length))
                command_cd(argc, argv);
            // else if (!memcmp(argv[0], "ls", command_length))
            //     command_ls(argc, argv);
            else if (!memcmp(argv[0], "mkdir", command_length))
                command_mkdir(argc, argv);
            // else if (!memcmp(argv[0], "cat", command_length))
            //     command_cat(argc, argv);
            // else if (!memcmp(argv[0], "cp", command_length))
            //     command_cp(argc, argv);
            // else if (!memcmp(argv[0], "rm", command_length))
            //     command_rm(argc, argv);
            // else if (!memcmp(argv[0], "mv", command_length))
            //     command_mv(argc, argv);
            // else if (!memcmp(argv[0], "whereis", command_length))
            //     command_whereis(argc, argv);
            else
            {
                puts(argv[0], BIOS_COLOR_WHITE);
                puts(": command not found\n", BIOS_COLOR_WHITE);
            }
        }
    }

    return 0;
}