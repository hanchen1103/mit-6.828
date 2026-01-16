#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"

static char *get_last_filename(char *path)
{
    int idx = 0;
    int len = strlen(path);

    for (idx = len - 1; idx >= 0 && path[idx] != '/'; idx--)
        ;
    return path + idx + 1;
}

void find_file(char *path, char *target, char **cmd)
{
    int fd;
    struct stat st;
    struct dirent de;
    char buf[512];
    char *p;

    if ((fd = open(path, O_RDONLY)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type)
    {
    case T_DEVICE:
        break;
    case T_FILE:
    {
        if (strcmp(get_last_filename(path), target) == 0)
        {
            if (cmd != 0)
            {
                char *exec_params[MAXARG];
                int i = 0;
                while (cmd[i] != 0 && i < MAXARG - 2)
                {
                    exec_params[i] = cmd[i];
                    i++;
                }
                exec_params[i] = path;
                exec_params[i + 1] = 0;

                int pid = fork();

                if (pid == 0)
                {
                    exec(cmd[0], exec_params);

                    fprintf(2, "exec failed\n");
                    exit(1);
                }
                else
                {
                    wait(0);
                }
            }
            else
                printf("%s\n", path);
        }
        break;
    }
    case T_DIR:
    {
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
        {
            printf("find: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if (de.inum == 0)
                continue;

            if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;

            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;

            find_file(buf, target, cmd);
        }
        break;
    }
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(2, "Usage: find dir and file...\n");
        exit(1);
    }
    char **exec_argv = 0;

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-exec") == 0)
        {
            if (i + 1 == argc)
            {
                fprintf(2, "find: missing argument to -exec\n");
                exit(1);
            }
            exec_argv = &argv[i + 1];
            break;
        }
    }

    find_file(argv[1], argv[2], exec_argv);
    exit(0);
}