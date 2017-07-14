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
	int type;		//文件类型
	int size;		//文件大小
	int ilever;		//文件等级
	char maker[12];		//文件创建者
	char control[8];	//文件控制信息
	int self_block;	   	//文件数据块号
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
		bitmap[0]=bitmap[DIRBLOCK]=bitmap[DIRBLOCK+INODEBLOCK]=1;
		for(int i=0;i<DIRBLOCK;i++)
		{
			for(int j=0;j<FCB_MAX;j++)
			{
				dir[i].fcb[j].inode_block=-1;
			}
		}
		inode[0].type=1;
		inode[0].size=4;
		inode[0].ilever=0;
		strcpy(inode[0].control,"111111");
		strcpy(inode[0].maker,"root");
		inode[0].self_block=0;
		memset(data,0,sizeof(data));
	}
};

struct Openlist //用户打开文件表
{
	int number=-1;	//文件i节点号
	int style;	//文件打开方式
	char file[28];	//文件名
};	

struct User   //用户表
{
	int lever;		//用户组号
	char username[12];	//用户名
	char password[12];	//用户密码
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
int currentstep=0;	
char cmd[8];		//操作类型
char object[28];	//操作目标
char arg[8];		//操作参数
char ch;
int DISKSIZE=BLOCKCOUNT*4+BLOCKSIZE*(DIRBLOCK+DATABLOCK)+INODEBLOCK*INODESIZE;	//磁盘大小
int DIRSTARTADDR=BLOCKCOUNT*4;			//磁盘目录块开始地址
int INODESTART=DIRSTARTADDR+BLOCKSIZE*DIRBLOCK;	//磁盘i节点块开始地址
int DATASTART=INODESTART+INODESIZE*INODEBLOCK;	//磁盘数据块开始地址
int tbitmap[BLOCKCOUNT];	//内存位示图		
struct DIR tdir;		//内存目录块
struct INODE cinode;		//内存当前i节点
struct INODE tinode;		//内存临时i节点
struct FCB tfcb;		//内存复制文件块
char tdata[BLOCKSIZE];		//内存数据块
Path path[10];			//当前路径	
struct Openlist openlist[10];	//用户文件打开表	
struct User user[7];		
struct User currentuser;

	
/*--------------磁盘操作函数------------------------*/
	
/*--------------生成自定义用户-----------*/
void User_create()
{
	user[0].lever=0;
	strcpy(user[0].username,"root");
	strcpy(user[0].password,"root");
	user[1].lever=1;
	strcpy(user[1].username,"user11");
	strcpy(user[1].password,"user11");
	user[2].lever=1;
	strcpy(user[2].username,"user12");
	strcpy(user[2].password,"user12");
	user[3].lever=2;
	strcpy(user[3].username,"user21");
	strcpy(user[3].password,"user21");
	user[4].lever=2;
	strcpy(user[4].username,"user22");
	strcpy(user[4].password,"user22");
	user[5].lever=3;
	strcpy(user[5].username,"user31");
	strcpy(user[5].password,"user31");
	user[6].lever=3;
	strcpy(user[6].username,"user32");
	strcpy(user[6].password,"user32");
	fseek(fp,DATASTART,SEEK_SET);
	fwrite(user,sizeof(user),1,fp);
}

/*------磁盘格始化---------*/
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

/*--------判断读/写权限---------*/
int access(struct INODE ainode,int a)
{
	if(strcmp(currentuser.username,ainode.maker)==0)
		return 1;
	else if(currentuser.lever==ainode.ilever)
	{
		if(ainode.control[a+2]=='1')
			return 1;
		else
			return 0;
	}
	if(ainode.control[a+4]=='1')
		return 1;
	else
		return 0;
};

/*--------创建目录-------*/
int mkdir(char *mk_dname,char *marg)
{
	
	int tag_1,tag_2,tag_3,i,j,k;
	/*---------测试权限--------*/
	if(access(cinode,1)==0)
	{
		cout<<"创建失败，请确认是否有足够权限！"<<endl;
		return 0;
	}
	for(i=0;i<6;i++)
	{
		if(marg[i]!='0'&&marg[i]!='1')
		{
			cout<<"创建目录参数有误，请确认后重新创建！"<<endl;
			return 0;
		}
	}
	/*------检查是否有同名文件--------*/
	for(i=0;i<FCB_MAX;i++)
	{
		if((tdir.fcb[i].inode_block!=-1)&&(strcmp(tdir.fcb[i].file_name,mk_dname)==0))
		{
			cout<<"创建目录失败，存在同名文件！"<<endl;
			return 0;
		}
	}
	/*------检查是否有空闲块--------*/
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
	for(j=0;j<DIRBLOCK;j++)
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
	strcpy(tdir.fcb[tag_1].file_name,mk_dname);
	tdir.fcb[tag_1].inode_block=tag_3;
	tbitmap[tag_2]=1;
	tbitmap[DIRBLOCK+tag_3]=1;
	tinode.type=1;
	strcpy(tinode.control,marg);
	tinode.size=0;
	tinode.ilever=currentuser.lever;
	strcpy(tinode.maker,currentuser.username);
	tinode.self_block=tag_2;
	fseek(fp,INODESTART+INODESIZE*tag_3,SEEK_SET);
	fwrite(&tinode,INODESIZE,1,fp);
	cout<<"创建目录成功！"<<endl;
	return 1;
};

/*---------显示当前目录文件----------*/
int ls(char *ls_arg)
{ 	
	/*---------显示文件详细信息---------*/
	if(strcmp(ls_arg,"-l")==0)
	{
		for(int i=0;i<FCB_MAX;i++)
		{
			if(tdir.fcb[i].inode_block!=-1)
				{
					fseek(fp,INODESTART+INODESIZE*tdir.fcb[i].inode_block,SEEK_SET);
					fread(&tinode,INODESIZE,1,fp);		
					cout<<tdir.fcb[i].file_name<<"   "<<tinode.type<<" "<<tinode.ilever<<" "
					<<tinode.control<<" "<<tinode.maker<<" "<<tinode.size<<"b  "<<tinode.self_block<<endl;
				}
		}
		return 1;
	}
	/*--------显示文件名称--------*/
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
	int path_1;
	/*---------返回上一级目录--------*/
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
		path_1=path[currentstep].blocknote;
		fseek(fp,INODESTART+INODESIZE*path_1,SEEK_SET);
		fread(&cinode,INODESIZE,1,fp);
		currentblock=cinode.self_block;
		fseek(fp,DIRSTARTADDR+BLOCKSIZE*currentblock,SEEK_SET);
		fread(&tdir,BLOCKSIZE,1,fp);
		return 1;
	}
	/*-------进入子目录-------*/
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
			if(access(tinode,0)==0)
			{
				cout<<"用户权限不足，无法访问该文件夹！"<<endl;
				return 0;
			}
			fseek(fp,DIRSTARTADDR+BLOCKSIZE*currentblock,SEEK_SET);
			fwrite(&tdir,BLOCKSIZE,1,fp);
			currentblock=tinode.self_block;
			fseek(fp,DIRSTARTADDR+BLOCKSIZE*currentblock,SEEK_SET);
			fread(&tdir,BLOCKSIZE,1,fp);
			fseek(fp,INODESTART+INODESIZE*path_1,SEEK_SET);
			fread(&cinode,INODESIZE,1,fp);
			currentstep++;
			path[currentstep].blocknote=path_1;
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
	int note1,note2;
	fseek(fp,DIRSTARTADDR+BLOCKSIZE*num,SEEK_SET);
	fread(&tdir,BLOCKSIZE,1,fp);
	for(int j=0;j<FCB_MAX;j++)
		{			
			if(tdir.fcb[j].inode_block!=-1)
			{
				note1=tdir.fcb[j].inode_block;
				tdir.fcb[j].inode_block=-1;
				fseek(fp,INODESTART+INODESIZE*note1,SEEK_SET);
				fread(&tinode,INODESIZE,1,fp);
				tbitmap[DIRBLOCK+note1]=0;
				note2=tinode.self_block;
				if(tinode.type==1)
					{
						rm_dir(tinode.self_block);
					}
				else
				{
					tbitmap[DIRBLOCK+INODEBLOCK+note2]=0;
				}
			}
		}
	return 1;
};

/*------------删除目录----------*/
int rmdir(char *rm_dname)
{
	int i,flag_1,flag_2;
	for(i=0;i<FCB_MAX;i++)
	{
		if((tdir.fcb[i].inode_block!=-1)&&(strcmp(tdir.fcb[i].file_name,rm_dname)==0)) //检查是否存在该目录
		{
			flag_1=tdir.fcb[i].inode_block;
			fseek(fp,INODESTART+INODESIZE*flag_1,SEEK_SET);
			fread(&tinode,INODESIZE,1,fp);
			if(tinode.type==2)
			{
				cout<<rm_dname<<"不是目录文件！"<<endl;
				return 0;
			}
			if(strcmp(tinode.maker,currentuser.username)!=0)
			{
				cout<<"用户权限不足，无法删除该文件夹！"<<endl;
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
	int open_1,i,select;
	for(i=0;i<FCB_MAX;i++)
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
			cout<<"选择打开方式(0代表可读可写，1代表只可读；2代表只可写)："<<endl;
			cin>>select;
			cin.ignore();
			switch(select)
			{
				case 0:if(access(tinode,0)==0||access(tinode,1)==0)
						{
							cout<<"用户权限不足，打开文件失败！"<<endl;
							return 0;
						}
						break;
				case 1:if(access(tinode,0)==0)
						{
							cout<<"用户权限不足，打开文件失败!"<<endl;
							return 0;
						}
						break;
				case 2:if(access(tinode,1)==0)
						{
							cout<<"用户权限不足，打开文件失败！"<<endl;
							return 0;
						}
						break;
				default: cout<<"输入参数错误！"<<endl;
							return 0;
			}
			for(i=0;i<10;i++)
				{
					if(openlist[i].number==-1)
					{
						openlist[i].number=open_1;
						strcpy(openlist[i].file,openname);
						openlist[i].style=select;
						cout<<"文件已打开！"<<endl;
						return 1;
					}
				}
			cout<<"文件打开表已达上限，打开文件失败！"<<endl;
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
		if((strcmp(openlist[i].file,closename)==0)&&(openlist[i].number!=-1))
		{
			openlist[i].number=-1;
			return 1;
		}
	}
	cout<<closename<<"不是一个已打开文件！"<<endl;
	return 0;
};

/*-----------写文件------------*/
int write(char *writename,char *write_arg)
{
	int a,write_1,write_2,write_3;
	for(a=0;a<10;a++)
	{
		if((strcmp(openlist[a].file,writename)==0)&&(openlist[a].number!=-1))
		{
			if(openlist[a].style==0||openlist[a].style==2)
			{
				write_1=openlist[a].number;
				break;
			}
			else
			{
				cout<<writename<<"是不可写文件！"<<endl;
				return 0;
			}
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
	if(strcmp(write_arg,"-a")==0)
	{
		a=tinode.size;
		fseek(fp,DATASTART+BLOCKSIZE*write_2,SEEK_SET);
		fread(tdata,BLOCKSIZE,1,fp);
	}
	else if(strcmp(write_arg,"-w")==0)
	{
		a=0;
		memset(tdata,0,BLOCKSIZE);
	}
	else
	{
		cout<<"参数错误！"<<endl;
		return 0;
	}
	cin.get(ch);
	while(ch!='\n')
	{
		tdata[a]=ch;
		a++;
		cin.get(ch);
	}
	fseek(fp,DATASTART+BLOCKSIZE*write_2,SEEK_SET);
	fwrite(tdata,BLOCKSIZE,1,fp);
	tinode.size=a;
	fseek(fp,INODESTART+INODESIZE*write_1,SEEK_SET);
	fwrite(&tinode,INODESIZE,1,fp);
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
			if(openlist[k].style==0||openlist[k].style==1)
			{
				read_1=openlist[k].number;
				break;
			}
			else
			{
				cout<<readname<<"是不可读文件！"<<endl;
				return 0;
			}
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
	memset(object,0,28);
	memset(arg,0,6);
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
		object[a]=ch;
		a++;
		cin.get(ch);
	};
	a=0;
	while(ch==' ')
	{
		cin.get(ch);
	};
	while(ch!='\n')
	{	
		arg[a]=ch;
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

/*-----------创建文件----------*/
int mkfile(char *mk_fname,char *brg)
{
	int tag_1,tag_2,tag_3,i,j,k;
	if(access(cinode,1)==0)
	{
		cout<<"用户权限不足，创建失败！"<<endl;
		return 0;
	}
	for(i=0;i<6;i++)
	{
		if(brg[i]!='0'&&brg[i]!='1')
		{
			cout<<"创建文件参数错误，请确认后重试！"<<endl;
			return 0;
		}
	}
	/*------检查是否有同名文件--------*/
	for(i=0;i<FCB_MAX;i++)
	{
		if((tdir.fcb[i].inode_block!=-1)&&(strcmp(tdir.fcb[i].file_name,mk_fname)==0))
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
	strcpy(tdir.fcb[tag_1].file_name,mk_fname);
	tdir.fcb[tag_1].inode_block=tag_3;
	tbitmap[tag_2+DIRBLOCK+INODEBLOCK]=1;
	tbitmap[DIRBLOCK+tag_3]=1;
	tinode.type=2;
	tinode.size=0;
	tinode.ilever=currentuser.lever;
	strcpy(tinode.control,brg);
	strcpy(tinode.maker,currentuser.username);
	tinode.self_block=tag_2;
	fseek(fp,INODESTART+INODESIZE*tag_3,SEEK_SET);
	fwrite(&tinode,INODESIZE,1,fp);
	memset(tdata,0,BLOCKSIZE);
	fseek(fp,DATASTART+BLOCKSIZE*tag_2,SEEK_SET);
	fwrite(tdata,BLOCKSIZE,1,fp);
	cout<<"创建文件成功"<<endl;
	return 1;

};

/*----------删除文件---------*/
int rmfile(char *rm_fname)
{
	int i,flag_1,flag_2;
	for(i=0;i<FCB_MAX;i++)
	{
		if((tdir.fcb[i].inode_block!=-1)&&(strcmp(tdir.fcb[i].file_name,rm_fname)==0)) //检查是否存在该目录
		{
			flag_1=tdir.fcb[i].inode_block;
			fseek(fp,INODESTART+INODESIZE*flag_1,SEEK_SET);
			fread(&tinode,INODESIZE,1,fp);
			if(tinode.type==1)
			{
				cout<<rm_fname<<"不是文本文件！"<<endl;
				return 0;
			}
			if(strcmp(tinode.maker,currentuser.username)!=0)
			{
				cout<<"用户权限不足！"<<endl;
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
		cout<<"没有该文件！"<<endl;
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
		cin>>currentuser.username;
		cout<<"输入登录密码:"<<endl;
		cin>>currentuser.password;
		for(int i=0;i<7;i++)
		{
			if((strcmp(currentuser.username,user[i].username)==0)
				&&(strcmp(currentuser.password,user[i].password)==0))
			{
				currentuser.lever=user[i].lever;
				return 0;
			}
		}
		cout<<"用户名或密码有误，请重试！"<<endl;
	}
};

/*--------登出---------*/
int logout()
{
	fseek(fp,DIRSTARTADDR+BLOCKSIZE*currentblock,SEEK_SET);
	fwrite(&tdir,BLOCKSIZE,1,fp);
	fseek(fp,DIRSTARTADDR,SEEK_SET);
	fread(&tdir,BLOCKSIZE,1,fp);
	fseek(fp,INODESTART,SEEK_SET);
	fread(&cinode,INODESIZE,1,fp);
	currentstep=0;
	currentblock=0;
	for(int i=0;i<10;i++)
	{
		openlist[i].number=-1;
	}
	cout<<"信息已存储，已登出！"<<endl;
	return 1;
}

/*---------帮助---------*/
void help()
{
	cout<<endl;
	cout<<"mkdir  <file> <arg>---------------创建目录"<<endl;
	cout<<"rmdir  <file>---------------------删除目录"<<endl;
	cout<<"mkfile <file> <arg>---------------创建文件"<<endl;
	cout<<"rmfile <file>---------------------删除文件"<<endl;
	cout<<"cd     <file>---------------------进入子目录"<<endl;
	cout<<"cd  ..   -------------------------返回上级目录"<<endl;
	cout<<"open   <file> <arg>---------------打开文件(0/1/2)"<<endl;
	cout<<"close  <file>---------------------关闭文件"<<endl;
	cout<<"read    <file>--------------------读文件"<<endl;
	cout<<"write  <file> <arg>---------------写文件(-a/-w)"<<endl;
	cout<<"ls     [-l]------------------ ----显示当前目录文件"<<endl;
	cout<<"copy   <file>---------------------复制文件"<<endl;
	cout<<"paste       ----------------------粘贴文件"<<endl;
	cout<<endl;
};

/*--------复制文件--------*/
int copy(char *copyname)
{
	for(int i=0;i<FCB_MAX;i++)
	{
		if((tdir.fcb[i].inode_block!=-1)&&(strcmp(tdir.fcb[i].file_name,copyname)==0))
		{
			tfcb.inode_block=tdir.fcb[i].inode_block;
			strcpy(tfcb.file_name,tdir.fcb[i].file_name);
			return 1; 
		}
		
	}
	cout<<"没有该文件"<<endl;
	return 0;
};

/*---------粘贴文件---------*/
int paste()
{
	int i;
	if(tfcb.inode_block==-1)
	{
		cout<<"没有已复制文件！"<<endl;
		return 1;
	}
	for(i=0;i<FCB_MAX;i++)
	{
		if((tdir.fcb[i].inode_block!=-1)&&(strcmp(tdir.fcb[i].file_name,tfcb.file_name)==0))
		{
			cout<<"粘贴失败，存在同名文件！"<<endl;
			return 0;
		}
	}
	for(i=0;i<FCB_MAX;i++)
		{
			if(tdir.fcb[i].inode_block==-1)
			{
				tdir.fcb[i].inode_block=tfcb.inode_block;
				strcpy(tdir.fcb[i].file_name,tfcb.file_name);
				return 1;
			}
		}
	cout<<"当前目录已满！"<<endl;
	return 0;
}

/*--------初始化---------*/
void initialize()
{
	fp=fopen(FILEPATH,"rb+");
	fseek(fp,0,SEEK_SET);
	fread(tbitmap,4,BLOCKCOUNT,fp);
	fread(&tdir,BLOCKSIZE,1,fp);
	fseek(fp,INODESTART,SEEK_SET);
	fread(&cinode,INODESIZE,1,fp);
	fseek(fp,DATASTART,SEEK_SET);
	fread(user,sizeof(user),1,fp);
	path[0].blocknote=0;
	strcpy(path[0].path_name,"root");
	tfcb.inode_block=-1;
}


int main()
{
	system("clear");
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
	initialize();
    cout<<"欢迎使用周涛文件管理系统！"<<endl;
    login();
    cout<<"登录成功!"<<endl;
    cin.ignore();
    while(1)
    {
    	showpath();
    	getcmd();
    	if(strcmp(cmd,"mkdir")==0)         
    	{
    		mkdir(object,arg);
    	}
    	else if(strcmp(cmd,"mkfile")==0)
    	{
    		mkfile(object,arg);
    	}
    	else if(strcmp(cmd,"rmdir")==0)
    	{
    		rmdir(object);
    	}
    	else if(strcmp(cmd,"rmfile")==0)
    	{
    		rmfile(object);
    	}
    	else if(strcmp(cmd,"cd")==0)
    	{
    		changepath(object);
    	}
    	else if(strcmp(cmd,"ls")==0)
    	{
    		ls(object);
    	}
    	else if(strcmp(cmd,"open")==0)
    	{
    		open(object);
    	}
    	else if(strcmp(cmd,"close")==0)
    	{
    		close(object);
    	}
    	else if(strcmp(cmd,"write")==0)
    	{
    		write(object,arg);
    	}
    	else if(strcmp(cmd,"read")==0)
    	{
    		read(object);
    	}
		else if(strcmp(cmd,"copy")==0)
    	{
    		copy(object);
    	}
    	else if(strcmp(cmd,"paste")==0)
    	{
    		paste();
    	}
    	else if(strcmp(cmd,"logout")==0)
    	{
    		logout();
    		system("clear");
    		login();
    		cin.ignore();
    	}
    	else if(strcmp(cmd,"help")==0)
    	{
    		help();
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
