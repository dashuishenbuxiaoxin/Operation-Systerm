#include<iostream>  
#include<string.h>
#include<malloc.h>
#include<memory.h>
#include<stdio.h>

using namespace std;

#define CATALOG 1        	//目录文件
#define TEXT  2			 //文本文件
#define BLOCKSIZE 512       	//磁盘块大小  
#define DIRBLOCK  64		//目录块数
#define INODEBLOCK 256     	 //文件i节点块数
#define DATABLOCK 256		//数据块数
#define FILEPATH ("/home/zt/zzz/code/myfile") //磁盘保存路径


struct FCB          //目录项结构
{
	char file_name[28];	
	int  inode_block;  //文件i节点
};

struct INODE	//i节点结构
{
	int type;
	int size;	
	int father_block;  //文件父目录块号
	int self_block;	   //文件数据块号

};

/*-------------------常量设置----------------------*/
const int FCB_MAX=BLOCKSIZE/sizeof(FCB);		//子目录最大数
const int INODESIZE=sizeof(INODE); 			//i节点大小	
const int BLOCKCOUNT=DIRBLOCK+INODEBLOCK+DATABLOCK;	//总块号

struct DIR 	//目录结构
{
	struct FCB fcb[FCB_MAX];
	void Init()
	{
		for(int i=0;i<FCB_MAX;i++)
		{
			fcb[i].inode_block=-1;
		}
	}
	
};


struct DISK //磁盘结构
{
	int bitmap[BLOCKCOUNT];  //位示图数组
	DIR dir[DIRBLOCK];	
	INODE inode[INODEBLOCK];
	char data[DATABLOCK][BLOCKSIZE];

	void Init()		//磁盘初始化
	{
		memset(bitmap,0,sizeof(bitmap));
		for(int i=0;i<DIRBLOCK;i++)
		{
			for(int j=0;j<FCB_MAX;j++)
			{
				dir[i].fcb[j].inode_block=-1;
			}
		}
		memset(data,0,sizeof(data));
	}
};

stuct Openlist
{
	int number;
	char file[28];
};

//struct Path	//当前路径
//{
	//int blocknote;
	//char path_name[28];
//};
/*----------------全局变量---------------------------*/
FILE *fp;      		//文件地址
char *BaseAddr;         //虚拟磁盘基地址
struct DISK *OSpoint;	
int currentblock=0;	//当前目录块号
char cmd[8];		//输入命令
char file_call[28];	//输入操作目标
int DISKSIZE=BLOCKCOUNT*4+BLOCKSIZE*(DIRBLOCK+DATABLOCK)+INODEBLOCK*INODESIZE;	//磁盘大小
int DIRSTARTADDR=BLOCKCOUNT*4;			//磁盘目录块开始地址
int INODESTART=DIRSTARTADDR+BLOCKSIZE*DIRBLOCK;	//磁盘i节点块开始地址
int DATASTART=INODESTART+INODESIZE*INODEBLOCK;	//磁盘数据块开始地址
int tbitmap[BLOCKCOUNT];	//内存位示图		
struct DIR tdir;		//内存目录项
struct INODE tinode;		//内存i节点
char tdata[BLOCKSIZE];
Path path[10];			
int currentstep=0;
//struct Openlist openlist[10];		
char ch;

/*--------------磁盘操作函数------------------------*/	


/*------磁盘ge始化---------*/
int format()
{
	memset(tbitmap,0,sizeof(tbitmap));
	tdir.Init();
	fp=fopen(FILEPATH,"rb+");
	fseek(fp,0,SEEK_SET);
	fwrite(tbitmap,4,BLOCKCOUNT,fp);
	for(int i=0;i<DIRBLOCK;i++)
	{
		
		fwrite(&tdir,1,BLOCKSIZE,fp);		
	}
	fclose(fp);
	cout<<"磁盘初始化成功！"<<endl;
	return 1;
};

/*--------创建目录-------*/
int mkdir(char *filename)
{
	int tag_1,tag_2,tag_3,i,j,k;
	/*------检查是否有同名文件--------*/
	for(i=0;i<FCB_MAX;i++)
	{
		if((tdir.fcb[i].inode_block!=-1)&&(strcmp(tdir.fcb[i].file_name,filename)==0))
		{
			cout<<"创建目录失败，存在相同文件！"<<endl;
			return 0;
		}
	}
	/*------检查是否有空闲FCB---------*/
	for(i=0;i<FCB_MAX;i++)
	{
		if(tdir.fcb[i].inode_block==-1)
		{
			tag_1=i;
			break;
		}
	}
	if(i==FCB_MAX)
	{
		cout<<"当前路径目录数已达最大，创建失败！"<<endl;
		return 0;
	}
	for(j=1;j<DIRBLOCK;j++)
	{
		if(tbitmap[j]==0)
		{
			tag_2=j;
			break;
		}
	}
	if(j==DIRBLOCK)
	{
		cout<<"磁盘目录已满！"<<endl;
		return 0;
	}

	for(k=0;k<INODEBLOCK;k++)
	{
		if(tbitmap[k+DIRBLOCK]==0)
		{
			tag_3=k;
			break;
		}
	}
	if(k==INODEBLOCK)
	{
		cout<<"磁盘i节点已满！"<<endl;
		return 0;
	}
	/*----------磁盘分配---------*/
	strcpy(tdir.fcb[tag_1].file_name,filename);
	tdir.fcb[tag_1].inode_block=tag_3;
	tbitmap[tag_2]=1;
	tbitmap[DIRBLOCK+tag_3]=1;
	tinode.type=1;
	tinode.size=4;
	tinode.father_block=currentblock;
	tinode.self_block=tag_2;
	fseek(fp,INODESTART+INODESIZE*tag_3,SEEK_SET);
	fwrite(&tinode,BLOCKSIZE,1,fp);
	cout<<"创建目录成功，你太棒了！"<<endl;
	return 1;
};

/*---------显示当前目录文件----------*/
int ls()
{
	for(int i=0;i<FCB_MAX;i++)
	{
		if(tdir.fcb[i].inode_block!=-1)
		{
			cout<<tdir.fcb[i].file_name<<"   ";
		}
	}
	cout<<endl;
	return 1;
};

/*----------转换路径----------*/
int changepath(char *pathname)
{
	int pathnum,path_1,path_2;
	if(strcmp(pathname,"..")==0)
	{
		if(currentstep==0)
		{
			cout<<"你已经在根目录。"<<endl;
			return 0;
		}
		path[currentstep].blocknote=0;
		currentstep=currentstep-1;
		pathnum=path[currentstep].blocknote;
		currentblock=pathnum;
		fseek(fp,DIRSTARTADDR+BLOCKSIZE*pathnum,SEEK_SET);
		fread(&tdir,BLOCKSIZE,1,fp);
		return 1;
	}
	for(int i=0;i<FCB_MAX;i++)
	{
		if((tdir.fcb[i].inode_block!=-1)&&(strcmp(tdir.fcb[i].file_name,pathname)==0))
		{
			path_1=tdir.fcb[i].inode_block;
			fseek(fp,INODESTART+INODESIZE*path_1,SEEK_SET);
			fread(&tinode,INODESIZE,1,fp);
			if(tinode.type==2)
			{
				cout<<pathname<<"不是目录文件！"<<endl;
				return 0;
			}
			pathnum=tinode.self_block;
			fseek(fp,DIRSTARTADDR+BLOCKSIZE*currentblock,SEEK_SET);
			fwrite(&tdir,BLOCKSIZE,1,fp);
			fseek(fp,DIRSTARTADDR+BLOCKSIZE*pathnum,SEEK_SET);
			fread(&tdir,BLOCKSIZE,1,fp);
			currentstep++;
			path[currentstep].blocknote=pathnum;
			currentblock=pathnum;
			strcpy(path[currentstep].path_name,pathname);
			return 1;
		}
	}
	cout<<"不存在该目录文件！"<<endl;
	return 0;
}
/*------------递归删除目录文件--------*/
int rm_dir(int num)
{
	tbitmap[num]=0;
	int note;
	fseek(fp,DIRSTARTADDR+BLOCKSIZE*num,SEEK_SET);
	fread(&tdir,BLOCKSIZE,1,fp);
	for(int j=0;j<FCB_MAX;j++)
		{
			
			if(tdir.fcb[j].inode_block!=-1)
			{
				note=tdir.fcb[j].inode_block;
				tdir.fcb[j].inode_block!=-1;
				fseek(fp,INODESTART+INODESIZE*note,SEEK_SET);
				fread(&tinode,INODESIZE,1,fp);
				tbitmap[DIRBLOCK+note]=0;
				if(tinode.type==1)
					{
						rm_dir(tinode.self_block);
					}
			}
		}
	return 1;
};

/*------------删除目录----------*/
int rmdir(char *filename)
{
	int i,flag_1,flag_2;
	for(i=0;i<FCB_MAX;i++)
	{
		if((tdir.fcb[i].inode_block!=-1)&&(strcmp(tdir.fcb[i].file_name,filename)==0)) //检查是否存在该目录
		{
			flag_1=tdir.fcb[i].inode_block;
			fseek(fp,INODESTART+INODESIZE*flag_1,SEEK_SET);
			fread(&tinode,INODESIZE,1,fp);
			if(tinode.type==2)
			{
				cout<<filename<<"不是目录文件！"<<endl;
				return 0;
			}
			if(tinode.type==1)
			{
				flag_2=tinode.self_block;
				tdir.fcb[i].inode_block=-1;
			}
			break;
		}
	}
	if(i==FCB_MAX)
	{
		cout<<"没有该目录文件！"<<endl;
		return 0;
	}
	fseek(fp,DIRSTARTADDR+BLOCKSIZE*currentblock,SEEK_SET);
	fwrite(&tdir,BLOCKSIZE,1,fp);
	tbitmap[DIRBLOCK+flag_1]=0;
	rm_dir(flag_2);
	fseek(fp,DIRSTARTADDR+BLOCKSIZE*currentblock,SEEK_SET);
	fread(&tdir,BLOCKSIZE,1,fp);
	cout<<"删除目录成功!"<<endl;
	return 1;
};

/*--------退出系统--------*/
int exit()
{	
	/*-------保存当前到磁盘------*/
	fseek(fp,0,SEEK_SET);
	fwrite(tbitmap,4,BLOCKCOUNT,fp);
	fseek(fp,DIRSTARTADDR+BLOCKSIZE*currentblock,SEEK_SET);
	fwrite(&tdir,BLOCKSIZE,1,fp);
	fclose(fp);
	cout<<"文件已保存，已退出！"<<endl;
	return 1;
}

/*void open(char *openname)
{
	int open_1;
	for(int i=0;i<FCB_MAX;i++)
	{
		if((tdir.fcb[i].inode_block!=-1)&&(strcmp(tdir.fcb[i].file_name,openname)==0)) //检查是否存在该目录
		{
			open_1=tdir.fcb[i].inode_block;
			fseek(fp,INODESTART+INODESIZE*open_1,SEEK_SET);
			fread(&tinode,INODESIZE,1,fp);
			if(tinode.type==1)
			{
				cout<<filename<<"不是wenben文件！"<<endl;
				return 0;
			}
			if(tinode.type==2)
			{
				for()
			}
			break;
		}
	}
};*/
/*------输入指令------*/
void getcmd()
{
	int a=0;
	memset(cmd,0,8);
	memset(file_call,0,28);
	cin.get(ch);
	while(ch==' ')
	{
		cin.get(ch);
	}
	while(ch!='\n'&&ch!=' ')
	{
		cmd[a]=ch;
		a++;
		cin.get(ch);
	};
	a=0;
	while(ch==' ')
	{
		cin.get(ch);
	};
	while(ch!='\n'&&ch!=' ')
	{	
		file_call[a]=ch;
		a++;
		cin.get(ch);
	};
};

/*-------显示当前路径-------*/
void showpath()
{
	for(int i=0;i<=currentstep;i++)
	{
		cout<<"/"<<path[i].path_name;
	}
	cout<<":";
};


int mkfile(char *filename)
{
	int tag_1,tag_2,tag_3,i,j,k;
	/*------检查是否有同名文件--------*/
	for(i=0;i<FCB_MAX;i++)
	{
		if((tdir.fcb[i].inode_block!=-1)&&(strcmp(tdir.fcb[i].file_name,filename)==0))
		{
			cout<<"创建目录失败，存在相同文件！"<<endl;
			return 0;
		}
	}
	/*------检查是否有空闲FCB---------*/
	for(i=0;i<FCB_MAX;i++)
	{
		if(tdir.fcb[i].inode_block==-1)
		{
			tag_1=i;
			break;
		}
	}
	if(i==FCB_MAX)
	{
		cout<<"当前路径目录数已达最大，创建失败！"<<endl;
		return 0;
	}
	for(j=0;j<DATABLOCK;j++)
	{
		if(tbitmap[j+DIRBLOCK+INODEBLOCK]==0)
		{
			tag_2=j;
			break;
		}
	}
	if(j==DATABLOCK)
	{
		cout<<"磁盘shujukuai已满！"<<endl;
		return 0;
	}

	for(k=0;k<INODEBLOCK;k++)
	{
		if(tbitmap[k+DIRBLOCK]==0)
		{
			tag_3=k;
			break;
		}
	}
	if(k==INODEBLOCK)
	{
		cout<<"磁盘i节点已满！"<<endl;
		return 0;
	}
	/*----------磁盘分配---------*/
	strcpy(tdir.fcb[tag_1].file_name,filename);
	tdir.fcb[tag_1].inode_block=tag_3;
	tbitmap[tag_2+DIRBLOCK+INODEBLOCK]=1;
	tbitmap[DIRBLOCK+tag_3]=1;
	tinode.type=2;
	tinode.size=4;
	tinode.father_block=currentblock;
	tinode.self_block=tag_2;
	fseek(fp,INODESTART+INODESIZE*tag_3,SEEK_SET);
	fwrite(&tinode,BLOCKSIZE,1,fp);
	cout<<"创建 wenjian 成功，你太棒了！"<<endl;
	return 1;

};

int rmfile(char *filename)
{
	int i,flag_1,flag_2;
	for(i=0;i<FCB_MAX;i++)
	{
		if((tdir.fcb[i].inode_block!=-1)&&(strcmp(tdir.fcb[i].file_name,filename)==0)) //检查是否存在该目录
		{
			flag_1=tdir.fcb[i].inode_block;
			fseek(fp,INODESTART+INODESIZE*flag_1,SEEK_SET);
			fread(&tinode,INODESIZE,1,fp);
			if(tinode.type==1)
			{
				cout<<filename<<"不是wenben文件！"<<endl;
				return 0;
			}
			if(tinode.type==2)
			{
				flag_2=tinode.self_block;
				tdir.fcb[i].inode_block=-1;
			}
			break;
		}
	}
	if(i==FCB_MAX)
	{
		cout<<"没有该wenben文件！"<<endl;
		return 0;
	}
	tbitmap[DIRBLOCK+flag_1]=0;
	fseek(fp,DATASTART+BLOCKSIZE*flag_2,SEEK_SET);
	memset(tdata,0,BLOCKSIZE);
	fwrite(tdata,BLOCKSIZE,1,fp);
	tbitmap[DIRBLOCK+INODEBLOCK+flag_2]=0;
	cout<<"删除wenjian成功!"<<endl;
	return 1;
};

int main()
{
	/*--------创建磁盘---------*/
	if((fp=fopen(FILEPATH,"r"))==NULL)
	{
		BaseAddr=(char *)malloc(DISKSIZE);
		OSpoint=(struct DISK *)(BaseAddr);
		OSpoint->Init();
		fp=fopen(FILEPATH,"wb+");
		fwrite(BaseAddr,DISKSIZE,1,fp); 
		fclose(fp);
		free(BaseAddr);
		cout<<"检测您首次使用该系统，磁盘已创建！"<<endl;
	}
	/*------读位示图、根目录---------*/
	fp=fopen(FILEPATH,"rb+");
	fseek(fp,0,SEEK_SET);
	fread(tbitmap,4,BLOCKCOUNT,fp);
	fread(&tdir,BLOCKSIZE,1,fp);
	path[0].blocknote=0;
	strcpy(path[0].path_name,"root");
    cout<<"欢迎使用王博组文件管理系统！"<<endl;
    while(1)
    {
    	showpath();
    	getcmd();
    	if(strcmp(cmd,"mkdir")==0)
    	{
    		mkdir(file_call);
    	}
    	else if(strcmp(cmd,"mkfile")==0)
    	{
    		mkfile(file_call);
    	}
    	else if(strcmp(cmd,"rmdir")==0)
    	{
    		rmdir(file_call);
    	}
    	else if(strcmp(cmd,"rmfile")==0)
    	{
    		rmfile(file_call);
    	}
    	else if(strcmp(cmd,"cd")==0)
    	{
    		changepath(file_call);
    	}
    	else if(strcmp(cmd,"ls")==0)
    	{
    		ls();
    	}
    	else if(strcmp(cmd,"exit")==0)
    	{
    		exit();
    		break;
    	}
    	else
    	{
    		cout<<"指令输入有误！"<<endl;
    	}
    }
	cout<<"感谢使用，再见！"<<endl;
	return 0;
}
