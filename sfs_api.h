#ifndef SFS_API_H
#define SFS_API_H
#include "sfs_header.h"
#define SAVE_FILE_NAME "disk.sfs"

// system
void sfs_system_init();
void sfs_system_close();

// directory
void sfs_ls();

// file
int sfs_open(char *name);
int sfs_close(int fileID);

// 返回-1表示文件已经打开，无法写入，返回-2表示磁盘空间不够，返回0正常写入
int sfs_write(int fileID, char *buf, int length);
int sfs_read(int fileID, char *buf, int length);
int sfs_delete(char *filename, int CurrentDir);
void sfs_rm(int CurrentDir);
// 简单直观的文件操作
int create_File(char *name, FileType T, int Dir);
int find_File(char *name, int CurrentDir);
// 目录操作
int path_find(char *path);
#endif
