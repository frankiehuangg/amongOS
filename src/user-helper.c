#include "lib-header/user-helper.h"
#include "lib-header/fat32.h"
#include "lib-header/stdtype.h"
#include "lib-header/user-commands.h"

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

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx)
{
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));

    __asm__ volatile("int $0x30");
}

uint32_t parse_pathing(char *path, uint8_t is_changing, uint8_t ignore_last)
{
    uint32_t current_cluster;

    // Relative or absolute
    if (path[0] == '/') // Absolute pathing
        current_cluster = ROOT_CLUSTER_NUMBER;
    else                // Relative pathing
        current_cluster = shell_state.current_cluster_number;

    // Seperate paths into array of chars divided by '/'
    char *token;
    char *paths[8] = {0};
    int len = 0;

    token = strtok(path, "/");
    while (token != NULL)
    {
        paths[len++] = token;
        token = strtok(NULL, "/");
    }
    paths[len] = NULL;

    uint32_t limit;
    if (ignore_last)
        limit = len - 1;
    else
        limit = len;

    uint32_t i = 0;
    while (i < limit)
    {  
        if (!memcmp(paths[i], "..", strlen(paths[i]))) 
        {
            uint32_t previous_cluster = current_cluster;

            syscall(8, (uint32_t) &current_cluster, 0, current_cluster);

            if (is_changing && previous_cluster != ROOT_CLUSTER_NUMBER)
            {
                do {
                    shell_state.path[shell_state.path_length - 1] = '\0';
                    shell_state.path_length--;
                } while(shell_state.path[shell_state.path_length - 1] != '/');
            }
        }
        else 
        {
            char name[8]    = {0};
            char ext[3]     = {0};

            uint8_t j = 0;
            uint8_t index = 0;
            uint8_t is_name = TRUE;
            while (j < strlen(paths[i]))
            {
                if (paths[i][j] == '.')
                {
                    index = 0;
                    is_name = FALSE;
                }
                else 
                {
                    if (is_name)
                        name[index] = paths[i][j];
                    else
                        ext[index] = paths[i][j];

                    index++;
                }
                j++;
            }

            struct FAT32DriverRequest request = {0};

            memcpy(&request.name, name, strlen(name));
            memcpy(&request.ext, ext, strlen(ext));
            request.parent_cluster_number = current_cluster;

            syscall(7, (uint32_t) &request, (uint32_t) &current_cluster, 0);

            if (current_cluster == 0)       // Check if not exists
                return 0;

            syscall(8, (uint32_t) &current_cluster, current_cluster, request.parent_cluster_number);

            if (is_changing)
            {
                uint32_t j = 0;
                do {
                    shell_state.path[shell_state.path_length++] = paths[i][j++];
                } while (j < strlen(paths[i]));

                shell_state.path[shell_state.path_length++] = '/';
            }
        }
        i++;
    }

    return current_cluster;
}

void puts(char* buf, uint8_t color)
{
    syscall(5, (uint32_t) buf, strlen(buf), color);
}