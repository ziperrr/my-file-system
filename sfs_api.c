#include "sfs_api.h"
#include "disk_emu.h"
#include "sfs_header.h"
#include "sfs_util.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>
FileMessage FM;
DirectoryDescriptor root;
FileAllocationTable fat;
int DirStack[FILE_LIST_SIZE];
int DirStackSize = 0;
void sfs_system_info()
{
    printf("\n============= File System Info ================= \n");
    printf("\n========Writer: %s     version:%s==========\n", FM.Writer, FM.Version);
    printf("\n========Disk Size: %d Bytes================\n", FM.BlockSize * FM.MaxDisk);
    printf("\n======Already Use %d Bytes, (%.2f)%% ========\n", NB_BLOCK * fat.count, (fat.count * 100.0f / MAX_DISK));
    printf("\n================File Count %d ==================\n", root.count);
}

void sfs_system_init()
{

    int fresh = access(SAVE_FILE_NAME, 0);
    if (fresh)
    {

        // 创建磁盘文件
        ds_init_fresh(SAVE_FILE_NAME, NB_BLOCK, MAX_DISK);
        // 初始化 FM信息
        FM_init();
        // 初始化 FAT
        FAT_init();
        // 初始化 目录
        DirectoryDescriptor_init();
        // 创建垃圾回收站
        create_File("recycle_bin", Directory, 0);
        ds_write_blocks(FM.MessageBlock, FM.MessageBlockNum, (void *)&FM);
        ds_write_blocks(FM.FAT_Block, FM.FAT_BlockSize, (void *)&fat); //  FAT
        ds_write_blocks(FM.FD_Block, FM.FD_BlockSize, (void *)&root);  //  FileDescriptor
    }
    else
    {
        FM.MessageBlockNum = 1;
        ds_init(SAVE_FILE_NAME, NB_BLOCK, MAX_DISK);
        ds_read_blocks(FM.MessageBlock, FM.MessageBlockNum, (void *)&FM); // FM
        ds_read_blocks(FM.FAT_Block, FM.FAT_BlockSize, (void *)&fat);     //  FAT
        ds_read_blocks(FM.FD_Block, FM.FD_BlockSize, (void *)&root);      //  FileDescriptor
    }
    // 初始化根目录
    for (int i = 0; i < root.count; i++)
    {
        root.table[i].fas.opened = 0;
    }
    sfs_system_info();
    push(0);
}

void sfs_system_close()
{
    // 系统退出之前关闭所有文件
    for (int i = 0; i < FILE_LIST_SIZE; i++)
    {
        root.table[i].fas.opened = 0;
    }

    ds_write_blocks(FM.MessageBlock, FM.MessageBlockNum, (void *)&FM);
    ds_write_blocks(FM.FAT_Block, FM.FAT_BlockSize, (void *)&fat); //  FAT
    ds_write_blocks(FM.FD_Block, FM.FD_BlockSize, (void *)&root);  //  FileDescriptor
    ds_close();
}

void sfs_ls()
{
    int iter = DirStack[DirStackSize - 1];
    while (iter != -1)
    {
        printf("%-20s\t%.1lfK\t", root.table[iter].filename, (double)root.table[iter].size / 1024);
        struct tm *p1 = gmtime(&root.table[iter].timestamp);
        struct tm *p2 = gmtime(&root.table[iter].timelast);
        printf("%d/%d/%d", (1900 + p1->tm_year), 1 + p1->tm_mon, p1->tm_mday);
        printf(" %d:%d:%d\t\t", (8 + p1->tm_hour), p1->tm_min, p1->tm_sec);
        switch (root.table[iter].type)
        {
        case File:
            printf("File");
            break;
        case Directory:
            printf("Directory");
            break;
        case Link:
            printf("Link");
            break;
        }
        printf("\n");
        iter = root.table[iter].CurrentDirecttoryNext;
    }
}
// 返回-1表示文件类型错误
// 返回-2表示文件已经被打开
// 一半返回文件在FD
int sfs_open(char *name)
{
    int CurrentDir = DirStack[DirStackSize - 1];
    int fileID = getIndexOfFileInDirectory(name, CurrentDir);
    // 如果找到了文件，将文件打开并返回
    if (fileID >= 0)
    {
        if (root.table[fileID].type != File)
        {
            printf("%s: Is not a file\n", root.table[fileID].filename);
        }
        return -1;
    }
    else if (fileID == -2)
    {
        printf("The file is open\n");
        return -2;
    }
    // 没有找到文件，则新增一个文件，初始化该文件的FCB，并将该FCB加入到目录中

    fileID = root.count++;
    // 文件创建函数，可以初始化文件
    FileDescriptor_createFile(name, fileID, File);

    // 给当前文件分配一个新的块
    int free_node_index = FAT_getFreeNode();
    root.table[fileID].fat_index = free_node_index;
    root.table[fileID].FD_index = fileID;
    CurrentDir = DirStack[DirStackSize - 1];
    AddFileInDir(fileID, CurrentDir);
    root.table[fileID].NextDirectory = -1;

    fat.table[free_node_index] = free_node_index; // 文件最后一个块的 内容等于它本身的编号

    ds_write_blocks(FM.FD_Block, FM.FD_BlockSize, (void *)&root);
    ds_write_blocks(FM.FAT_Block, FM.FAT_BlockSize, (void *)&fat);

    return fileID;
}

int sfs_close(int fileID)
{
    root.table[fileID].fas.opened = 0;
    return 0;
}

int sfs_write(int fileID, char *buf, int length)
{
    if (root.table[fileID].fas.opened == 0)
        return -1;

    int16_t node_index = root.table[fileID].fat_index;

    // 重新写入文件前，回收该文件所占用的全部空间
    while (node_index != fat.table[node_index])
    {
        fat.table[node_index] = -1;
        fat.count--;
        node_index = fat.table[node_index];
    }
    fat.table[node_index] = -1;
    fat.count--;
    //  写入文件长度
    root.table[fileID].size = length;
    root.table[fileID].timelast = time(NULL);
    // 写入文件
    int block_num = length / NB_BLOCK;
    int res = length % NB_BLOCK;
    // 如果文件的长度不是 块长的整数倍，应该多分配一个块
    if (res > 0)
        block_num += 1;
    if (block_num + fat.count > MAX_DISK)
    {
        printf("Disk is not enough\n");
        return -2;
    }

    // 声明一个块
    void *blockWrite = (void *)malloc(NB_BLOCK);

    if (blockWrite == 0)
        return -1;
    // 全部置为0
    int16_t last_node_index = -1;
    memset(blockWrite, 0, NB_BLOCK);

    while (length > 0)
    {
        if (length > NB_BLOCK)
        {
            memcpy(blockWrite, (void *)buf, NB_BLOCK);

            int16_t free_node_index = FAT_getFreeNode();
            ds_write_blocks(free_node_index, 1, blockWrite); // 写入一个块
            if (last_node_index != -1)
            {
                fat.table[last_node_index] = free_node_index;
            }
            else
            {
                last_node_index = free_node_index;
                root.table[fileID].fat_index = free_node_index;
            }
            length -= NB_BLOCK;
            buf += NB_BLOCK; // 指针后移
        }
        else
        {
            memcpy(blockWrite, (void *)buf, length);
            int16_t free_node_index = FAT_getFreeNode();
            ds_write_blocks(free_node_index, 1, blockWrite); // 写入一个块
            if (last_node_index != -1)
            {
                fat.table[last_node_index] = free_node_index;
            }
            else
            {
                last_node_index = free_node_index;
                root.table[fileID].fat_index = free_node_index;
            }
            buf += length;
            length -= length;
        }
    }
    fat.table[last_node_index] = last_node_index; // 标识文件结束
    free(blockWrite);
    return 0;
}

int sfs_read(int fileID, char *buf, int length)
{
    if (root.table[fileID].fas.opened == 0)
        return -1;

    int8_t read_flag = 1;

    void *blockRead = (void *)malloc(NB_BLOCK);
    // 全部置为0
    int16_t read_node_index = root.table[fileID].fat_index;

    memset(blockRead, 0, NB_BLOCK);

    while (length > 0)
    {
        if (length > NB_BLOCK)
        {
            ds_read_blocks(read_node_index, 1, blockRead);
            memcpy(buf, blockRead, NB_BLOCK);
            length -= NB_BLOCK;
            buf += NB_BLOCK; // 指针后移
        }
        else
        {
            ds_read_blocks(read_node_index, 1, blockRead);
            memcpy(buf, blockRead, length);
            buf += length; // 指针后移
            length -= length;
        }
        int16_t next_read_node_index = fat.table[read_node_index];
        if (next_read_node_index == read_node_index)
            break;
        else
            read_node_index = next_read_node_index;
    }
    free(blockRead);
    return 0;
}

// 创建一个文件，并返回其FileID，返回-1代表文件已经存在
int create_File(char *filename, FileType T, int Dir)
{
    int CurrentDir = Dir;

    if (find_File(filename, CurrentDir) != -1)
        return -1;
    int fileID = DirectoryDescriptor_getFreespot();
    FileDescriptor_createFile(filename, fileID, T);

    int free_node_index = FAT_getFreeNode();
    root.table[fileID].fat_index = free_node_index;
    root.table[fileID].FD_index = fileID;
    AddFileInDir(fileID, CurrentDir);
    fat.table[free_node_index] = free_node_index; // 文件最后一个块的 内容等于它本身的编号
    if (T == Directory)
    {
        int t1 = fileID;
        int t2 = free_node_index;
        int t3 = CurrentDir;

        int fileID1 = DirectoryDescriptor_getFreespot();
        FileDescriptor_createFile(".", fileID1, Directory);

        free_node_index = FAT_getFreeNode();
        root.table[fileID1].fat_index = free_node_index;
        root.table[fileID1].FD_index = fileID1;
        CurrentDir = fileID1;

        fat.table[free_node_index] = free_node_index;

        int fileID2 = DirectoryDescriptor_getFreespot();
        FileDescriptor_createFile("..", fileID2, Directory);
        free_node_index = FAT_getFreeNode();
        root.table[fileID2].fat_index = free_node_index;
        root.table[fileID2].FD_index = fileID2;

        AddFileInDir(fileID2, CurrentDir);
        fat.table[free_node_index] = free_node_index;

        // 文件夹指向.
        root.table[t1].NextDirectory = fileID1;
        //.指向自己
        root.table[fileID1].CurrentDirecttoryNext = fileID2;
        root.table[fileID1].NextDirectory = fileID1;
        //..指向父目录
        root.table[fileID2].CurrentDirecttoryNext = -1;
        root.table[fileID2].NextDirectory = t1;

        CurrentDir = t3;
        free_node_index = t2;
        fileID = t1;
    }

    ds_write_blocks(FM.MessageBlock, FM.MessageBlockNum, (void *)&FM);
    ds_write_blocks(FM.FD_Block, FM.FD_BlockSize, (void *)&root);
    ds_write_blocks(FM.FAT_Block, FM.FAT_BlockSize, (void *)&fat);
    return fileID;
}

int sfs_delete(char *filename, int CurrentDir)
{
    int iter = root.table[CurrentDir].CurrentDirecttoryNext;
    int pre = CurrentDir;
    while (iter != -1)
    {
        if (strcmp(filename, root.table[iter].filename) == 0)
        {

            // 释放DirectoryDescripter
            int fileID = iter;
            root.table[pre].CurrentDirecttoryNext = root.table[iter].CurrentDirecttoryNext;
            root.table[fileID].timelast = time(NULL);
            // 放入垃圾回收站
            int RootDir = DirStack[0];
            int RecyID = find_File("recycle_bin", RootDir);
            if (RecyID == -1)
            {
                RecyID = create_File("recycle_bin", Directory, RootDir);
            }
            AddFileInDir(fileID, root.table[RecyID].NextDirectory);
            break;
        }
        pre = iter;
        iter = root.table[iter].CurrentDirecttoryNext;
    }
}

void sfs_rm(int CurrentDir)
{

    if (root.table[CurrentDir].type == File)
    {
        // fat块清空并回收

        int16_t fat_iter = root.table[CurrentDir].fat_index;
        void *buffer = (void *)malloc(NB_BLOCK);
        memset(buffer, 0, NB_BLOCK);
        while (fat.table[(int)fat_iter] != -1)
        {
            int16_t tmp = fat.table[(int)fat_iter];
            ds_write_blocks(fat_iter, 1, buffer);
            fat.table[(int)fat_iter] = -1;
            fat_iter = tmp;
            fat.count--;
        }

        // FileDescriptor回收
        root.table[CurrentDir].fat_index = 0;
        root.table[CurrentDir].fas.opened = 0;
        root.table[CurrentDir].timelast = 0;
        root.table[CurrentDir].timestamp = 0;
        root.table[CurrentDir].fat_index = -1;
        root.table[CurrentDir].CurrentDirecttoryNext = -1;
        root.table[CurrentDir].NextDirectory = -1;
        root.count--;
        root.first_del = min(root.first_del, CurrentDir);
    }
    else if (root.table[CurrentDir].type == Directory)
    {
        if (strcmp(root.table[CurrentDir].filename, ".") == 0 || strcmp(root.table[CurrentDir].filename, "..") == 0)
        {
            root.table[CurrentDir].type = File;
            sfs_rm(CurrentDir);
            return;
        }

        int iter = root.table[CurrentDir].NextDirectory;
        while (iter != -1)
        {
            int tmp = root.table[iter].CurrentDirecttoryNext;
            sfs_rm(iter);
            iter = tmp;
        }
        root.table[CurrentDir].type = File;
        sfs_rm(CurrentDir);
    }
}
// 找到返回fileID,没找到返回-1
int find_File(char *name, int CurrentDir)
{
    int iter = CurrentDir;
    while (iter != -1)
    {
        if (strcmp(name, root.table[iter].filename) == 0)
        {
            break;
        }
        iter = root.table[iter].CurrentDirecttoryNext;
    }
    return iter;
}

// 返回-1表示没找到目录，返回-2表示文件名过长，-3表示找的是文件
int path_find(char *filepath)
{
    int CurrentDir = DirStack[DirStackSize - 1];
    int len = strlen(filepath);
    char tmp[21] = {0};
    int len_t = 0;

    int state = 0;
    if (len > 0 && filepath[0] == '/')
    {
        CurrentDir = DirStack[0];
    }
    for (int i = 0; i < len + 1; i++)
    {
        if (filepath[i] == '/' || i == len)
        {
            if (len_t == 0)
            {
                CurrentDir = DirStack[0];
                continue;
            }
            tmp[len_t] = 0;
            int fileID = find_File(tmp, CurrentDir);
            if (fileID == -1)
            {
                printf("No such file or directory\n");
                return -1;
            }
            // 判断是文件还是目录，是文件的话检测后面是否有字符串，有的话报错，无的话正常返回该文件的fileID
            if (root.table[fileID].type == File)
            {
                if (i == len)
                {
                    printf("%s is not a directory\n", filepath);
                    return -3;
                }
                else
                {
                    printf("No such file or directory\n");
                    return -1;
                }
            }
            // 如果是文件或者链接则直接进入
            else
            {
                CurrentDir = root.table[fileID].NextDirectory;
            }
            len_t = 0;
            continue;
        }
        if (len <= 20)
        {
            tmp[len_t++] = filepath[i];
        }
        else
        {
            printf("The file name is too long");
            return -2;
        }
    }
    return CurrentDir;
}