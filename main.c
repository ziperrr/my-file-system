#include "sfs_api.h"
#include "usr_api.h"
#include "sfs_header.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
extern int DirStack[FILE_LIST_SIZE];
extern int DirStackSize;
int main()
{
    sfs_system_init();
    // 六月三日调试fat
    while (input())
        ;

    sfs_system_close();

    return 0;
}
