#ifndef USR_API_H
#define USR_API_H

#include "sfs_api.h"
// 列出当前目录下的所有文件
void my_ls();
// 在指定路径下创建新的文件
void my_touch(char *path);
// 在指定路径下创建文件夹
void my_mkdir(char *path);
// 删除指定路径下的文件夹
void my_rmdir(char *path);
// 进入指定路径下的目录
void my_cd(char *path);
// 查看当前所在路径
void my_pwd();
// 删除文件，path为指定文件/文件夹，op表示删除方式，op==0表示放入回收站的删除，op==1表示强制删除不放入回收站
void my_rm(char *path, int op);
// 查看指定路径下的文件内容
void my_cat(char *path);
// 向指定路径下的文件写内容。以'`'字符作为结束标志
void my_write(char *path);
#endif