#include<iostream>
#include<string>
#include<malloc.h>

using namespace std;

#define CATALOG 1        	//目录文件
#define TEXT  2			//文本文件
#define BLOCKSIZE 512           //磁盘块大小  
#define DIRBLOCK  16		//目录块数
#define BFDTABBLOCK 64          //文件i节点块数
#define DATABLOCK 256		//数据块数



struct FCB          //目录项结构
{
	char file_name[20];	
	int  inode_block;  //文件i节点
};

struct BFD	//i节点结构
{
	int set_time;
	int type;
	int size;	
	int father_block;  //文件父目录块号
	int self_block;	   //文件数据块号

	void BFD_Init() //初始化
	{
		int type=0;
	}
};

const int FCB_MAX=BLOCKSIZE/sizeof(FCB);	//子目录最大数
const int BFD_MAX=BLOCKSIZE/sizeof(BFD);	//磁盘块i节点最大数
const int BLOCKCOUNT=DIRBLOCK+BFDTABBLOCK+DATABLOCK;	//磁盘总块数

struct DIR 	//目录结构
{
	struct FCB fcb[FCB_MAX];
	void DIR_Init()
	{
		for(int i=0;i<FCB_MAX;i++)
		{
			fcb[i].inode_block=-1;
		}
	}
	
};
struct BFDTAB	//i节点结构
{
	struct BFD bfd[BFD_MAX];
};

struct DISK //磁盘结构
{
	int FAT1[BLOCKCOUNT];  //位示图数组
	DIR root;	//主目录
	DIR dir[16];	
	BFDTAB bfd_tab[64];

};

int main()
{
	//int *p;
	//p=(int *)malloc((INODEBLOCK+DATABLOCK+2)*BLOCKSIZE sizeof(char))
	cout<<"test!!"<<endl;
	return 0;
}
