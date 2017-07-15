/*--------------------------------------------------------------*/
/*------------------------ZZZ FILE SYSTEM-----------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/

##文件卷：
        位示图--->目录块----->i节点块----->数据块

##数据结构：

///磁盘：
1.位示图-------整型数组
2.目录项-------FCB结构体
3.目录块-------DIR结构体
4.i节点块------INODE结构体
5.数据块-------字符数组	
6.磁盘---------DISK结构体

///内存：
1.用户打开文件表------openlist结构体
2.用户表--------------User结构体
3.路径表--------------Path结构体

###################以上所有结构体格式见代码#######################

##主要全局变量
FILE *fp                
int currentblock        
char cmd[8]             
char object[28]		
char arg[8]		
int DIRSTART	
int INODESTART	
int DATASTART		
DIR rdir
INODE tinode
INODE cinode
FCB  tfcb
char tdata[blocksize]
Path path[10]
Openlist openlist[10]
User currentuser

###################以上所有变量解释见代码########################

##主要子函数
format()      	//格式化
access()	//判断读写权限，参数0代表读，1代表写
mkdir()		//创建目录
rmdir()		//删除目录
mkfile()	//创建文件
rmfile()	//删除文件
ls()		//显示目录文件 参数 -l 显示文件详细信息
changepath()	//改变路径 参数 .. 返回上一级
open()		//打开文件 参数 0读写打开；1打开读；2打开写
close()		//关闭文件
read()		//读文件
write()		//写文件 参数 -a 尾部添加写 -w 覆盖写
getcmd()	//获取操作
showpath()	//显示路径
login()		//登录
logout()	//登出
copy()		//复制
paste()		//粘贴

###################以上所有函数参数见代码########################

##注释

//该文件系统采用多级用户权限----创建者/同组用户/其他用户，创建
目录或文件时需要输入权限参数。
  系统默认用户有7个，详情见代码User_creat()函数，可通过修改代码
来增删改用户名和密码。

//该文件系统所有结构大小参数见代码前面变量声明，可通过修改代码
改变磁盘大小格式。源代码默认：
文件系统位置："/home/zt/zzz/code/myfile"
目录文件类型---------1
文本文件类型---------2

//该文件系统复制粘贴方式为快捷方式粘贴，非硬链接。

################更多信息请运行代码自行理解######################

                                       from dashuishenbuxiaoxin
			  write for OS Curriculum Design of NEU
					              2017-7-15