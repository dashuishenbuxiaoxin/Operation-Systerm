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
	int control;	
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

struct Openlist //用户打开文件表
{
	int number=-1;
	char file[28];
};

struct User   //用户表
{
	int lever;
	char username[20];
	char password[20];
};

struct Path	//当前路径
{
	int blocknote;
	char path_name[28];
};

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
char tdata[BLOCKSIZE];		//内存数据块
Path path[10];			//当前路径
int currentstep=0;		
struct Openlist openlist[10];		
char ch;
struct User user[4];
int currentlever;
char logname[20];
char logword[20];

/*--------------磁盘操作函数------------------------*/
	
/*--------------生成用户-----------*/
void User_create()
{
	user[0].lever=1;
	strcpy(user[0].username,"root");
	strcpy(user[0].password,"root123");
	user[1].lever=2;
	strcpy(user[1].username,"huser");
	strcpy(user[1].password,"huser123");
	user[2].lever=3;
	strcpy(user[2].username,"muser");
	strcpy(user[2].password,"muser123");
	user[3].lever=4;
	strcpy(user[3].username,"luser");
	strcpy(user[3].password,"luser123");
	fseek(fp,DATASTART,SEEK_SET);
	fwrite(user,sizeof(user),1,fp);
}

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
	tinode.control=currentlever;
	tinode.size=4;
	tinode.father_block=currentblock;
	tinode.self_block=tag_2;
	fseek(fp,INODESTART+INODESIZE*tag_3,SEEK_SET);
	fwrite(&tinode,INODESIZE,1,fp);
	cout<<tag_3<<"创建目录成功，你太棒了！"<<tag_2<<endl;
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
		fseek(fp,DIRSTARTADDR+BLOCKSIZE*currentblock,SEEK_SET);
		fwrite(&tdir,BLOCKSIZE,1,fp);
		currentstep=currentstep-1;
		currentblock=path[currentstep].blocknote;
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
			if(tinode.control<currentlever)
			{
				cout<<tinode.control<<"||"<<path_1;
				cout<<"你的权限不够！"<<endl;
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
			if(tinode.control<currentlever)
			{
				cout<<tinode.control<<" "<<flag_1;
				cout<<"你的权限不够"<<endl;
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

/*---------打开文件------------*/
int open(char *openname)
{
	int open_1;
	for(int i=0;i<FCB_MAX;i++)
	{
		if((tdir.fcb[i].inode_block!=-1)&&(strcmp(tdir.fcb[i].file_name,openname)==0)) //检查是否存在该文件
		{
			open_1=tdir.fcb[i].inode_block;
			fseek(fp,INODESTART+INODESIZE*open_1,SEEK_SET);
			fread(&tinode,INODESIZE,1,fp);
			if(tinode.type==1)
			{
				cout<<openname<<"不是文本文件！"<<endl;
				return 0;
			}
			if(tinode.control<currentlever)
			{
				cout<<tinode.control<<" "<<open_1;
				cout<<"你的权限不够"<<endl;
				return 0;
			}
			for(int i=0;i<10;i++)
				{
					if(openlist[i].number==-1)
					{
						openlist[i].number=open_1;
						strcpy(openlist[i].file,openname);
						return 1;
					}
				}
			cout<<"文件已打开！"<<endl;
			return 0;			
		}
	}
	cout<<"没有该文件！"<<endl;
	return 1;
};

/*---------------关闭打开文件------------*/
int close(char *closename)
{
	for(int i=0;i<10;i++)
	{
		if(strcmp(openlist[i].file,closename)==0)
		{
			openlist[i].number=-1;
			return 1;
		}
	}
	cout<<closename<<"不是一个已打开文件！"<<endl;
	return 0;
};

/*-----------写文件------------*/
int write(char *writename)
{
	int write_1,a,write_2;
	for(a=0;a<10;a++)
	{
		if((strcmp(openlist[a].file,writename)==0)&&(openlist[a].number!=-1))
		{
			write_1=openlist[a].number;
			break;
		}
	}
	if(a==10)
	{
		cout<<writename<<"不是一个已打开文件！"<<endl;
		return 0;
	}
	fseek(fp,INODESTART+INODESIZE*write_1,SEEK_SET);
	fread(&tinode,INODESIZE,1,fp);
	write_2=tinode.self_block;
	a=0;
	memset(tdata,0,BLOCKSIZE);
	cin.get(ch);
	while(ch!='\n')
	{
		tdata[a]=ch;
		a++;
		cin.get(ch);
	}
	fseek(fp,DATASTART+BLOCKSIZE*write_2,SEEK_SET);
	fwrite(tdata,BLOCKSIZE,1,fp);
	return 1;
};

/*------------读文件----------*/
int read(char *readname)
{
	int read_1,read_2,k;
	for(k=0;k<10;k++)
	{
		if((strcmp(openlist[k].file,readname)==0)&&(openlist[k].number!=-1))
		{
			read_1=openlist[k].number;
			break;
		}
	}
	if(k==10)
	{
		cout<<readname<<"不是一个已打开文件！"<<endl;
		return 0;
	}
	fseek(fp,INODESTART+INODESIZE*read_1,SEEK_SET);
	fread(&tinode,INODESIZE,1,fp);
	read_2=tinode.self_block;
	fseek(fp,DATASTART+BLOCKSIZE*read_2,SEEK_SET);
	fread(tdata,BLOCKSIZE,1,fp);
	cout<<tdata<<endl;
	return 1;
};

/*------获取指令------*/
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
	cout<<currentlever;
	for(int i=0;i<=currentstep;i++)
	{
		cout<<"/"<<path[i].path_name;
	}
	cout<<":";
};

/*-----------创建文件----------*/
int mkfile(char *filename)
{
	int tag_1,tag_2,tag_3,i,j,k;
	/*------检查是否有同名文件--------*/
	for(i=0;i<FCB_MAX;i++)
	{
		if((tdir.fcb[i].inode_block!=-1)&&(strcmp(tdir.fcb[i].file_name,filename)==0))
		{
			cout<<"创建文件失败，存在同名文件！"<<endl;
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
	for(j=1;j<DATABLOCK;j++)
	{
		if(tbitmap[j+DIRBLOCK+INODEBLOCK]==0)
		{
			tag_2=j;
			break;
		}
	}
	if(j==DATABLOCK)
	{
		cout<<"磁盘数据块已满！"<<endl;
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
	tinode.control=currentlever;
	tinode.father_block=currentblock;
	tinode.self_block=tag_2;
	fseek(fp,INODESTART+INODESIZE*tag_3,SEEK_SET);
	fwrite(&tinode,INODESIZE,1,fp);
	memset(tdata,0,BLOCKSIZE);
	fseek(fp,DATASTART+BLOCKSIZE*tag_2,SEEK_SET);
	fwrite(tdata,BLOCKSIZE,1,fp);
	cout<<tag_3<<"创建文件成功，你太棒了！"<<tag_2<<endl;
	return 1;

};

/*----------删除文件---------*/
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
				cout<<filename<<"不是文本文件！"<<endl;
				return 0;
			}
			if(tinode.control<currentlever)
			{
				cout<<tinode.control<<" "<<flag_1;
				cout<<"你的权限不够!"<<endl;
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
		cout<<"没有该文件名！"<<endl;
		return 0;
	}
	tbitmap[DIRBLOCK+flag_1]=0;
	tbitmap[DIRBLOCK+INODEBLOCK+flag_2]=0;
	cout<<"删除文件成功!"<<endl;
	return 1;
};

/*-------登录---------*/
int login()
{
	while(1)
	{
		cout<<"输入用户名:"<<endl;
		cin>>logname;
		cout<<"输入登录密码:"<<endl;
		cin>>logword;
		for(int i=0;i<4;i++)
		{
			if((strcmp(logname,user[i].username)==0)&&(strcmp(logword,user[i].password)==0))
			{
				currentlever=user[i].lever;
				return 0;
			}
		}
		cout<<"用户名或密码有误！"<<endl;
	}
};

/*--------登出---------*/
int logout()
{
	fseek(fp,DIRSTARTADDR+BLOCKSIZE*currentblock,SEEK_SET);
	fwrite(&tdir,BLOCKSIZE,1,fp);
	fseek(fp,DIRSTARTADDR,SEEK_SET);
	fread(&tdir,BLOCKSIZE,1,fp);
	currentstep=0;
	currentblock=0;
	for(int i=0;i<10;i++)
	{
		openlist[i].number=-1;
	}
	cout<<"信息已存储，已登出！"<<endl;
	return 1;
}

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
		User_create(); 
		fclose(fp);
		free(BaseAddr);
		cout<<"检测您首次使用该系统，磁盘已创建！"<<endl;
	}
	/*------读位示图、根目录,用户表---------*/
	fp=fopen(FILEPATH,"rb+");
	fseek(fp,0,SEEK_SET);
	fread(tbitmap,4,BLOCKCOUNT,fp);
	fread(&tdir,BLOCKSIZE,1,fp);
	fseek(fp,DATASTART,SEEK_SET);
	fread(user,sizeof(user),1,fp);
	path[0].blocknote=0;
	strcpy(path[0].path_name,"root");
    cout<<"欢迎使用王博组文件管理系统！"<<endl;
    login();
    cout<<"登录成功!"<<endl;
    cin.ignore();
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
    	else if(strcmp(cmd,"open")==0)
    	{
    		open(file_call);
    	}
    	else if(strcmp(cmd,"close")==0)
    	{
    		close(file_call);
    	}
    	else if(strcmp(cmd,"write")==0)
    	{
    		write(file_call);
    	}
    	else if(strcmp(cmd,"read")==0)
    	{
    		read(file_call);
    	}
    	else if(strcmp(cmd,"logout")==0)
    	{
    		logout();
    		login();
    		cin.ignore();
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
