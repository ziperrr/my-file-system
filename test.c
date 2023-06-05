#include "sfs_api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void)
{
    char buf[4096 * 2] = {0};
    char buf2[4096 * 2] = {0};

    char *bigfile = "bigfile.txt";

    for (int i = 0; i < 4096 * 2; i++)
    {
        buf[i] = 'A' + i % 26;
    }
    buf[4096 * 2 - 1] = '\0';
    printf("原始文本 %s len = %d\n", buf, (int)strlen(buf));

    sfs_system_init();
    sfs_ls();

    int fid = sfs_open(bigfile);
    sfs_write(fid, buf, strlen(buf) + 1);

    printf("文件写入完成！\n");
    sfs_ls();
    sfs_read(fid, buf2, 4096 * 2);
    sfs_close(fid);
    printf("读取后的文本 %s len = %d\n", buf, (int)strlen(buf));

    sfs_system_close();
    return 0;
}
