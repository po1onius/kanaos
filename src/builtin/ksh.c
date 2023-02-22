#include <stdio.h>
#include <syscall.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <fs.h>

#define MAX_CMD_LEN 256
#define MAX_ARG_NR 16
#define MAX_PATH_LEN 1024
#define BUFLEN 1024

static int8 cwd[MAX_PATH_LEN];
static int8 cmd[MAX_CMD_LEN];
static int8 *argv[MAX_ARG_NR];
static int8 buf[BUFLEN];

static int8 kanaos_logo[] = {
"    _____   _         ____     _____  \n \
  / ____| | |       / __ \\   / ____| \n \
 | |      | |      | |  | | | (___   \n \
 | |      | |      | |  | |  \\___ \\  \n \
 | |____  | |____  | |__| |  ____) | \n \
  \\_____| |______|  \\____/  |_____/  \n\0"
};


int8 *basename(int8 *name)
{
    int8 *ptr = strrsep(name);
    if (!ptr[1])
    {
        return ptr;
    }
    ptr++;
    return ptr;
}

void print_prompt()
{
    getcwd(cwd, MAX_PATH_LEN);
    int8 *ptr = strrsep(cwd);
    if (ptr != cwd)
    {
        *ptr = 0;
    }
    int8 *base = basename(cwd);
    printf("[root %s]# ", base);
}

void builtin_logo()
{
    clear();
    printf((int8*)kanaos_logo);
}

void builtin_test(int32 argc, int8 *_argv[])
{
    test();
}

void builtin_pwd()
{
    getcwd(cwd, MAX_PATH_LEN);
    printf("%s\n", cwd);
}

void builtin_clear()
{
    clear();
}

void builtin_ls()
{
    int32 fd = open(cwd, O_RDONLY, 0);
    if (fd == EOF)
    {
        return;
    }

    lseek(fd, 0, SEEK_SET);
    struct dentry_t entry;
    while (true)
    {
        int32 len = readdir(fd, &entry, 1);
        if (len == EOF)
        {
            break;
        }
        if (!entry.nr)
        {
            continue;
        }
        if (!strcmp(entry.name, ".") || !strcmp(entry.name, ".."))
        {
            continue;
        }
        printf("%s ", entry.name);
    }
    printf("\n");
    close(fd);
}

void builtin_cd(int32 argc, int8 *_argv[])
{
    chdir(_argv[1]);
}

void builtin_cat(int32 argc, int8 *_argv[])
{
    int32 fd = open(_argv[1], O_RDONLY, 0);
    if (fd == EOF)
    {
        printf("file %s not exists.\n", _argv[1]);
        return;
    }

    while (true)
    {
        int32 len = read(fd, buf, BUFLEN);
        if (len == EOF)
        {
            break;
        }
        write(stdout, buf, len);
    }
    close(fd);
}

void builtin_mkdir(int32 argc, int8 *_argv[])
{
    if (argc < 2)
    {
        return;
    }
    mkdir(_argv[1], 0755);
}

void builtin_rmdir(int32 argc, int8 *_argv[])
{
    if (argc < 2)
    {
        return;
    }
    rmdir(_argv[1]);
}

void builtin_rm(int32 argc, int8 *_argv[])
{
    if (argc < 2)
    {
        return;
    }
    unlink(_argv[1]);
}

static void execute(int32 argc, int8 *_argv[])
{
    int8 *line = _argv[0];
    if (!strcmp(line, "test"))
    {
        return builtin_test(argc, _argv);
    }
    if (!strcmp(line, "logo"))
    {
        return builtin_logo();
    }
    if (!strcmp(line, "pwd"))
    {
        return builtin_pwd();
    }
    if (!strcmp(line, "clear"))
    {
        return builtin_clear();
    }
    if (!strcmp(line, "exit"))
    {
        //int32 code = 0;
        //if (argc == 2)
        //{
        //    code = atoi(_argv[1]);
        //}
        //exit(code);
    }
    if (!strcmp(line, "ls"))
    {
        return builtin_ls();
    }
    if (!strcmp(line, "cd"))
    {
        return builtin_cd(argc, _argv);
    }
    if (!strcmp(line, "cat"))
    {
        return builtin_cat(argc, _argv);
    }
    if (!strcmp(line, "mkdir"))
    {
        return builtin_mkdir(argc, _argv);
    }
    if (!strcmp(line, "rmdir"))
    {
        return builtin_rmdir(argc, _argv);
    }
    if (!strcmp(line, "rm"))
    {
        return builtin_rm(argc, _argv);
    }
    printf("osh: command not found: %s\n", _argv[0]);
}

void readline(int8 *_buf, uint32 count)
{
    assert(_buf != NULL);
    int8 *ptr = _buf;
    uint32 idx = 0;
    int8 ch = 0;
    while (idx < count)
    {
        ptr = _buf + idx;
        int32 ret = read(stdin, ptr, 1);
        if (ret == -1)
        {
            *ptr = 0;
            return;
        }
        switch (*ptr)
        {
            case '\n':
            case '\r':
                *ptr = 0;
                ch = '\n';
                write(stdout, &ch, 1);
                return;
            case '\b':
                if (_buf[0] != '\b')
                {
                    idx--;
                    ch = '\b';
                    write(stdout, &ch, 1);
                }
                break;
            case '\t':
                continue;
            default:
                write(stdout, ptr, 1);
                idx++;
                break;
        }
    }
    _buf[idx] = '\0';
}

static int32 cmd_parse(int8 *_cmd, int8 *_argv[], int8 token)
{
    assert(_cmd != NULL);

    int8 *next = _cmd;
    int32 argc = 0;
    while (*next && argc < MAX_ARG_NR)
    {
        while (*next == token)
        {
            next++;
        }
        if (*next == 0)
        {
            break;
        }
        _argv[argc++] = next;
        while (*next && *next != token)
        {
            next++;
        }
        if (*next)
        {
            *next = 0;
            next++;
        }
    }
    _argv[argc] = NULL;
    return argc;
}

int32 osh_main()
{
    memset(cmd, 0, sizeof(cmd));
    memset(cwd, 0, sizeof(cwd));

    builtin_logo();

    while (true)
    {
        print_prompt();
        readline(cmd, sizeof(cmd));
        if (cmd[0] == 0)
        {
            continue;
        }
        int32 argc = cmd_parse(cmd, argv, ' ');
        if (argc < 0 || argc >= MAX_ARG_NR)
        {
            continue;
        }
        execute(argc, argv);
    }
    return 0;
}
