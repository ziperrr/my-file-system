#ifndef SFS_API_UTIL
#define SFS_API_UTIL
#include "sfs_header.h"

void FM_init();
void FAT_init();
// 在FAT中寻找一个新的空的块
int FAT_getFreeNode();
// 在root所在的目录下创建一个新文件夹
void DirectoryDescriptor_init();
// 在CurrentDir目录下寻找名为name的文件
int getIndexOfFileInDirectory(char *name, int CurrentDir);
// 创建一个FileDescriptor对象保存在file指针里面
void FileDescriptor_createFile(char *name, int file, FileType T);
// 获得一个空闲的FBC
int DirectoryDescriptor_getFreespot();
// 将newFile添加到CurrentDir底下
void AddFileInDir(int newFile, int CurrentDir);
// 在DirectoryDescriptor下找到一个空的FileDescriptor
int DirectoryDescriptor_getFreespot();

// 目录栈控制函数
void push(int content);
void pop();

#endif