#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(2, "Usage: sixfive need file ...\n");
        exit(1);
    }
    char *sep = " -\r\t\n./,";

    char c;
    char buf[512];

    for (int i = 1; i < argc; i++)
    {
        int n, idx = 0;
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0)
        {
            fprintf(2, "sixfive: cannot open file %s\n", argv[1]);
            exit(1);
        }
        while ((n = read(fd, &c, 1)) > 0)
        {
            char *find = strchr(sep, c);
            if (find == 0)
            {
                if (idx < sizeof(buf) - 1)
                {
                    buf[idx++] = c;
                }
            }
            else
            {
                if (idx > 0)
                {
                    buf[idx] = '\0';
                    int x = atoi(buf);
                    if (x != 0)
                    {
                        if (x % 5 == 0 || x % 6 == 0)
                        {
                            printf("%d\n", x);
                        }
                    }
                    idx = 0;
                }
            }
        }

        if (idx != 0)
        {
            buf[idx] = '\0';
            int x = atoi(buf);
            if (x != 0)
            {
                if (x % 5 == 0 || x % 6 == 0)
                {
                    printf("%d\n", x);
                }
            }
        }
        close(fd);
    }

    exit(0);
}