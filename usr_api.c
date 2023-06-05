
#include "usr_api.h"
#include "sfs_api.h"
#include "sfs_util.h"
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
extern FileMessage FM;
extern DirectoryDescriptor root;
extern FileAllocationTable fat;
extern int DirStack[FILE_LIST_SIZE];
extern int DirStackSize;

void path_preprocess(char *path, char *dir, char *base)
{
    char path1[100] = {0}, path2[100] = {0};
    strncpy(path1, path, 90);
    strncpy(path2, path, 90);
    strncpy(dir, dirname(path1), 90);
    strncpy(base, basename(path2), 90);
}

void my_ls()
{
    sfs_ls();
}

void my_touch(char *path)
{

    char dir[100] = {0};
    char base[100] = {0};
    path_preprocess(path, dir, base);
    int DirID = path_find(dir);
    int fileID = find_File(base, DirID);
    if (fileID != -1 || DirID < 0)
    {
        printf("cannot create file '%s': File exists\n", path);
        return;
    }
    create_File(base, File, DirID);
}

void my_mkdir(char *path)
{
    char dir[100] = {0};
    char base[100] = {0};
    path_preprocess(path, dir, base);
    int DirID = path_find(dir);
    int fileID = find_File(base, DirID);
    if (fileID != -1 || DirID < 0)
    {
        printf("cannot create directory '%s': File exists\n", path);
        return;
    }
    create_File(base, Directory, DirID);
}

void my_rmdir(char *path)
{
    char dir[100] = {0};
    char base[100] = {0};
    path_preprocess(path, dir, base);
    int DirID = path_find(dir);
    int fileID = find_File(base, DirID);
    if (fileID == -1 || DirID < 0 || root.table[fileID].type == File || root.table[fileID].type == Link)
    {
        printf("failed to remove '%s': No such file or directory\n", path);
        return;
    }
    sfs_delete(base, DirID);
}

void my_cd(char *path)
{
    char dir[100] = {0};
    char base[100] = {0};
    path_preprocess(path, dir, base);
    int DirID = path_find(dir);
    int fileID = find_File(base, DirID);
    if (fileID == -1 || DirID < 0 || root.table[fileID].type == File)
    {
        printf("No such file or directory\n");
        return;
    }
    int CurrentDir = DirStack[DirStackSize - 1];
    int len = strlen(path);
    char tmp[21] = {0};
    int len_t = 0;

    int state = 0;

    for (int i = 0; i < len + 1; i++)
    {
        if (path[i] == '/' || i == len)
        {
            if (len_t == 0)
            {
                CurrentDir = DirStack[0];
                DirStackSize = 1;
                continue;
            }
            tmp[len_t] = 0;
            int fileID = find_File(tmp, CurrentDir);
            if (fileID == -1)
            {
                printf("No such file or directory\n");
                return;
            }
            // 判断是文件还是目录，是文件的话检测后面是否有字符串，有的话报错，无的话正常返回该文件的fileID
            if (root.table[fileID].type == File || root.table[fileID].type == Link)
            {
                if (i == len)
                {
                    printf("%s is not a directory\n", path);
                    return;
                }
                else
                {
                    printf("No such file or directory\n");
                    return;
                }
            }
            // 如果是文件或者链接则直接进入
            else
            {
                if (strcmp(tmp, "..") == 0 && DirStackSize > 1)
                {
                    pop();
                }
                else if (strcmp(tmp, ".") == 0 && DirStackSize > 1)
                {
                }
                else
                {
                    push(root.table[fileID].NextDirectory);
                }
                CurrentDir = root.table[fileID].NextDirectory;
            }
            len_t = 0;
            continue;
        }
        if (len <= 20)
        {
            tmp[len_t++] = path[i];
        }
        else
        {
            printf("The file name is too long\n");
            return;
        }
    }
    return;
}

void my_rm(char *path, int op)
{
    char dir[100] = {0};
    char base[100] = {0};
    path_preprocess(path, dir, base);
    int DirID = path_find(dir);
    int fileID = find_File(base, DirID);
    if (fileID == -1 || DirID < 0)
    {
        printf("cannot remove file '%s': File exists\n", path);
        return;
    }
    // op==0表示放入回收站的删除
    if (op == 0)
    {
        if (root.table[fileID].type == Directory)
        {
            printf("cannot remove '%s': Is a directory\n", path);
            return;
        }
        sfs_delete(base, DirID);
    }
    // op==1代表彻底删除，采用递归删除的方式
    else if (op == 1)
    {
        int iter = root.table[DirID].CurrentDirecttoryNext;
        int pre = DirID;
        while (iter != -1)
        {
            if (iter == fileID)
            {
                // 在链表中删除该文件
                root.table[pre].CurrentDirecttoryNext = root.table[iter].CurrentDirecttoryNext;
                sfs_rm(iter);
                break;
            }
            pre = iter;
            iter = root.table[iter].CurrentDirecttoryNext;
        }
    }
    else
    {
        printf("invalid option\n");
        return;
    }
}

void my_cat(char *path)
{
    char dir[100] = {0};
    char base[100] = {0};
    path_preprocess(path, dir, base);
    int DirID = path_find(dir);
    int fileID = find_File(base, DirID);
    if (fileID == -1 || DirID < 0)
    {
        printf("No such file or directory\n");
        return;
    }
    if (root.table[fileID].type != File)
    {
        printf("%s: Is not a file\n", root.table[fileID].filename);
        return;
    }
    int length = root.table[fileID].size;
    char *buf = (char *)malloc(length);
    root.table[fileID].fas.opened = 1;
    sfs_read(fileID, buf, length);
    for (int i = 0; i < length; i++)
    {
        putchar(buf[i]);
    }
    root.table[fileID].fas.opened = 0;
    free(buf);
}

void my_write(char *path)
{
    char dir[100] = {0};
    char base[100] = {0};
    path_preprocess(path, dir, base);
    int DirID = path_find(dir);
    int fileID = find_File(base, DirID);
    root.table[fileID].fas.opened = 1;
    int length = NB_BLOCK;
    char *buf = (char *)malloc(length);
    if (fileID == -1 || DirID < 0)
    {
        printf("No such file or directory\n");
        return;
    }
    if (root.table[fileID].type != File)
    {
        printf("%s: Is not a file\n", root.table[fileID].filename);
        return;
    }
    if (buf == NULL)
    {
        printf("Space allocation failed\n");
        return;
    }
    int size = 0;
    char ch = 0;
    while ((ch = getchar()) != '`')
    {
        buf[size++] = ch;
        if (size >= length)
        {
            int remain_block = MAX_DISK - fat.count;
            if (remain_block <= 0)
            {
                printf("Disk is not enough\n");
                return;
            }
            buf = (char *)realloc(buf, length + NB_BLOCK);
            length += NB_BLOCK;
        }
    }
    int mes = sfs_write(fileID, buf, size);
    root.table[fileID].fas.opened = 0;
    if (mes < 0)
    {
        return;
    }
}

int input()
{
    printf(">>");
    char command[20];
    fflush(stdin);
    scanf("%s", command);
    if (strcmp(command, "cd") == 0)
    {
        char path[100] = {0};
        scanf("%s", path);
        my_cd(path);
        return 1;
    }
    else if (strcmp(command, "pwd") == 0)
    {
        my_pwd();
        return 1;
    }
    else if (strcmp(command, "mkdir") == 0)
    {
        char path[100] = {0};
        scanf("%s", path);
        my_mkdir(path);
        return 1;
    }
    else if (strcmp(command, "rmdir") == 0)
    {
        char path[100] = {0};
        scanf("%s", path);
        my_rmdir(path);
        return 1;
    }
    else if (strcmp(command, "ls") == 0)
    {
        my_ls();
        return 1;
    }
    else if (strcmp(command, "touch") == 0)
    {
        char path[100] = {0};
        scanf("%s", path);
        my_touch(path);
        return 1;
    }
    else if (strcmp(command, "cat") == 0)
    {
        char path[100] = {0};
        scanf("%s", path);
        my_cat(path);
        return 1;
    }
    else if (strcmp(command, "write") == 0)
    {
        char path[100] = {0};
        scanf("%s", path);
        my_write(path);
        return 1;
    }
    else if (strcmp(command, "rm") == 0)
    {
        // rm有两个参数，第一个为文件路径，第二个为删除方式
        char path[100] = {0};
        scanf("%s", path);
        int op = 0;
        scanf("%d", &op);
        my_rm(path, op);
        return 1;
    }
    else if (strcmp(command, "exit") == 0)
    {
        return 0;
    }
    else
    {
        printf("Input error\n");
        return 1;
    }
    return 1;
}

void my_pwd()
{
    if (DirStackSize == 1)
    {
        printf("/");
        return;
    }
    for (int i = 1; i < DirStackSize; i++)
    {
        printf("/%s", root.table[root.table[root.table[DirStack[i]].CurrentDirecttoryNext].NextDirectory].filename);
    }
    printf("\n");
}
