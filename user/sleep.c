#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(2, "Usage: sleep need time...\n");
        exit(1);
    }

    int time = atoi(argv[1]);
    pause(time);
    exit(0);
}