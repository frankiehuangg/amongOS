#include "lib-header/user-commands.h"
#include "lib-header/user-helper.h"

struct ShellState shell_state = {
    .current_cluster_number     = ROOT_CLUSTER_NUMBER,
    .path_length                = 1,
    .path                       = "/",
};

int main(void)
{
    char buf[KEYBOARD_BUFFER_SIZE];
    char* token;
    char* argv[16];

    while (TRUE)
    {
        puts("IF2230@among", BIOS_COLOR_RED);
        puts("OS", BIOS_COLOR_CYAN);

        puts(shell_state.path, BIOS_COLOR_GREEN);

        puts("$ ", BIOS_COLOR_GREEN);

        memset(buf, 0, KEYBOARD_BUFFER_SIZE);
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
            else if (!memcmp(argv[0], "ls", command_length))
                command_ls(argc, argv);
            else if (!memcmp(argv[0], "mkdir", command_length))
                command_mkdir(argc, argv);
            else if (!memcmp(argv[0], "cat", command_length))
                command_cat(argc, argv);
            else if (!memcmp(argv[0], "cp", command_length))
                command_cp(argc, argv);
            else if (!memcmp(argv[0], "rm", command_length))
                command_rm(argc, argv);
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