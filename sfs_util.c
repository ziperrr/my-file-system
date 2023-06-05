#include "sfs_util.h"
#include "disk_emu.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int fatID = 0;
extern FileMessage FM;
extern DirectoryDescriptor root;
extern FileAllocationTable fat;
extern int DirStack[FILE_LIST_SIZE];
extern int DirStackSize;

void FM_init()
{
	char *WriterName = "ziperr";
	strncpy((FM.Writer), WriterName, strlen(WriterName));
	char *Version = "1.0";
	strncpy((FM.Version), Version, strlen(Version));
	FM.BlockSize = NB_BLOCK;
	FM.MaxDisk = MAX_DISK;
	// 更新时间
	time(&(FM.Update));
	// 已用空间
	FM.SpaceUsed = 0;
	// FileMessage位置
	FM.MessageBlock = 0;
	// 为FileMessage分配的空间大小
	FM.MessageBlockNum = FILE_MESSAGE_BLOCK_NUM;
	// FAT的位置
	FM.FAT_Block = FILE_MESSAGE_BLOCK_NUM;
	// FAT所占块大小
	FM.FAT_BlockSize = FAT_BLOCK_NUM;
	// FileDescript的位置
	FM.FD_Block = FILE_MESSAGE_BLOCK_NUM + FAT_BLOCK_NUM;
	// FileDescript所能存放的文件数量
	FM.MaxFileNum = FILE_LIST_SIZE;
	// FileDescript所占块数量
	FM.FD_BlockSize = FILE_LIST_BLOCK_NUM;
	// Disk起始块
	FM.DiskBlock = FILE_MESSAGE_BLOCK_NUM + FAT_BLOCK_NUM + FILE_LIST_BLOCK_NUM;
	// Disk所占块数
	FM.DiskBlockNum = DISK_BLOCK_NUM;
}

// 初始化FAT，将FAT中的所有节点设置为自由节点
void FAT_init()
{
	// 除了第0个和第1个块，其它全部块初始化均为-1代表没有被分配
	for (int i = 0; i < FM.DiskBlock; i++)
	{
		fat.table[i] = i;
		fat.count++;
	}
	for (int i = FM.DiskBlock; i < FM.DiskBlockNum + FM.DiskBlock; i++)
	{
		fat.table[i] = -1;
	}
	// 将磁盘中的第一个和第二个块分别设置为.和..作为根目录
	fat.table[FM.DiskBlock] = FM.DiskBlock;
	fat.table[FM.DiskBlock + 1] = FM.DiskBlock + 1;
	fat.count += 2;
}

// 获取一个自由块，代表
int FAT_getFreeNode()
{

	int i;
	for (i = FM.DiskBlock; i < FM.DiskBlock + FM.DiskBlockNum; i++)
	{
		if (fat.table[i] == -1)
		{
			fat.count++; // 被占用块 + 1
			return i;	 // 返回第i个块
		}
	}

	fprintf(stderr, "Error: Cannot get free block in FAT.\n");
	// exit(0);
	return -1;
}

// 初始化目录表
void DirectoryDescriptor_init()
{
	// 将根目录的计数初始化为2
	// 创建根目录时会默认创建两个文件夹，分别为.和..
	root.count = 2;
	root.first_del = 2;

	FileDescriptor_createFile(".", 0, Directory);
	FileDescriptor_createFile("..", 1, Directory);

	root.table[0].CurrentDirecttoryNext = 1;
	root.table[0].NextDirectory = 0;
	root.table[0].FD_index = 0;
	root.table[0].fat_index = FM.DiskBlock;

	root.table[1].CurrentDirecttoryNext = -1;
	root.table[1].NextDirectory = 0;
	root.table[1].FD_index = 1;
	root.table[1].fat_index = FM.DiskBlock + 1;
}
// 返回2代表回收站出错
int DirectoryDescriptor_getFreespot()
{
	// 文件系统已满
	if (root.count >= FILE_LIST_SIZE - 1)
	{
		int Recy = 2;
		if (strcmp(root.table[Recy].filename, "recycle_bin") != 0)
		{
			printf("recycle_bin find error\n");
			return -2;
		}
		int freeID = root.table[root.table[Recy].NextDirectory].CurrentDirecttoryNext;
		freeID = root.table[freeID].CurrentDirecttoryNext;
		if (freeID == -1)
		{
			printf("Disk is already full");
			return -1;
		}
		return freeID;
	}
	int ans = root.first_del++;
	root.count++;
	// 在删除文件描述符的时候会把time重置为0，之前也会初始化为0
	while (root.table[root.first_del].timelast != 0)
	{
		root.first_del++;
	}
	return ans;
}

// 打开文件，成功会非0的整数代表文件ID
// 当前系统中不存在该文件，返回 -1
// 当前系统中存在该文件，并且已经打开，返回 -2
int getIndexOfFileInDirectory(char *name, int CurrentDir)
{

	// Looks for the file and gets its index.
	// for (i = 0; i < rootDir->count; i++)
	// {
	// 	if (!strcmp(rootDir->table[i].filename, name))
	// 	{
	// 		fileID = i;
	// 		break;
	// 	}
	// }

	int iter = CurrentDir;
	while (iter != -1)
	{
		if (strcmp(root.table[iter].filename, name) == 0)
		{

			break;
		}
		iter = root.table[iter].CurrentDirecttoryNext;
	}

	// Checks if the file was correctly closed.
	// if (fileID != -1)
	// {
	// 	if (rootDir->table[fileID].fas.opened == 1)
	// 	{
	// 		// fprintf(stderr, "Warning: '%s' is already open.\n", name);
	// 		return -2;
	// 	}
	// }
	if (iter == -1)
		return -1;
	if (root.table[iter].fas.opened == 1)
		return -2;
	return iter;
}

void AddFileInDir(int newFile, int CurrentDir)
{
	int iter = root.table[CurrentDir].CurrentDirecttoryNext;
	int pre = CurrentDir;
	while (iter != -1)
	{
		if (strcmp(root.table[newFile].filename, root.table[iter].filename) < 0)
		{
			root.table[pre].CurrentDirecttoryNext = newFile;
			root.table[newFile].CurrentDirecttoryNext = iter;
			break;
		}
		iter = root.table[iter].CurrentDirecttoryNext;
		pre = root.table[pre].CurrentDirecttoryNext;
	}
	if (iter == -1)
	{
		root.table[pre].CurrentDirecttoryNext = newFile;
		root.table[newFile].CurrentDirecttoryNext = iter;
	}
}

void FileDescriptor_createFile(char *file_name, int file, FileType T)
{
	strcpy(root.table[file].filename, file_name);
	root.table[file].fas.opened = 1;

	root.table[file].size = 0;
	root.table[file].timestamp = time(NULL);
	root.table[file].timelast = time(NULL);
	root.table[file].type = T;
	root.table[file].CurrentDirecttoryNext = -1;
	root.table[file].NextDirectory = -1;
}

void push(int content)
{
	DirStack[DirStackSize++] = content;
}
void pop()
{
	if (DirStackSize <= 0)
	{
		printf("目录栈错误");
		return;
	}
	DirStackSize--;
}
