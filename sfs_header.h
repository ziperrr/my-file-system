
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef SFS_API_HEADER
#define SFS_API_HEADER

#include <time.h>
#include <stdint.h>

#define NB_BLOCK 4096 // 每个块有4096个 byte
#define MAX_DISK 2047 // 磁盘总共有多少个块
#define FILE_LIST_SIZE 100
#define FAT_BLOCK_NUM (MAX_DISK / NB_BLOCK + 1) // FAT所占的块数

typedef struct FileAccessStatus
{               // 文件当前状态
    int opened; // 该文件是否已经打开
} FileAccessStatus;
// 文件类型，有文件、目录、链接
typedef enum FileType
{
    File = 0,
    Directory = 1,
    Link = 2
} FileType;
// // 用户种类
// typedef enum User
// {
//     root=0,

// };
// // 用户权限
// typedef struct Permission
// {
//     int16_t per;
// } Permission;
// 记录文件系统的基本信息
typedef struct FileMessage
{

    char Writer[30];
    // 文件系统作者
    char Version[20];
    // 版本
    int16_t BlockSize;
    // 每个文件块大小
    int16_t MaxDisk;
    // 文件块的数量
    time_t Update;
    // 更新时间
    int SpaceUsed;
    // 已使用空间,单位为块
    int MessageBlock;
    // FileMessage位置
    int MessageBlockNum;
    // FileMessage长度
    int FAT_Block;
    //  FAT存放的Block位置
    int FAT_BlockSize;
    //  FAT块数
    int FD_Block;
    //  FileDescript存放的块位置
    int MaxFileNum;
    // 文件最大存储数量
    int FD_BlockSize;
    //  FileDescript存放块大小
    int DiskBlock;
    // 磁盘起始块
    int DiskBlockNum;
    // 磁盘起始块大小
    int First_Del;

} FileMessage;

#define FILE_MESSAGE_BLOCK_NUM (sizeof(FileMessage) / NB_BLOCK + 1)

typedef struct FileDescriptor
{                      // FCB
    char filename[20]; // 文件名
    int fat_index;     // 在fat 上的index
    int FD_index;      // 在DirectoryDescriptor上的序号
    time_t timestamp;  // 创建时间
    time_t timelast;   //  最后一次修改时间
    FileType type;     // 文件类型（文件，目录，链接）
    int size;          // 文件大小
    int CurrentDirecttoryNext, NextDirectory;
    // 当前目录文件指针与下级目录文件指针
    FileAccessStatus fas; // 文件状态
    //

} FileDescriptor;
// 定义为文件描述单元分配的块数
#define FILE_LIST_BLOCK_NUM (sizeof(FileDescriptor) * FILE_LIST_SIZE / NB_BLOCK + 1)

#define DISK_BLOCK_NUM (MAX_DISK - FILE_LIST_BLOCK_NUM - FAT_BLOCK_NUM - FILE_MESSAGE_BLOCK_NUM)
// 目录结构体 整个文件系统中只有一个
typedef struct DirectoryDescriptor
{ // 描述目录信息

    // 文件列表，最多保存 NB_BLOCK / sizeof(FileDescriptor) 个文件
    FileDescriptor table[FILE_LIST_SIZE];
    int count; // 文件个数
    int first_del;
} DirectoryDescriptor;

// 文件分配表，整个文件系统只有一个
typedef struct FileAllocationTable
{
    int16_t table[MAX_DISK]; // 初始的时候为全-1，代表没有被分配
    int16_t count;           // 已经分配的块的个数
} FileAllocationTable;

#endif
