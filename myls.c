#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <memory.h>
#include <malloc.h>
#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <error.h>
#include <sys/types.h>
#include <linux/kdev_t.h>


#define  OPT_v        0x0001   /* -v 选项:查看本软件版本 */
#define  OPT_h        0x0002   /* -h 选项:查看本软件帮助 */
#define  OPT_l        0x0004   /* -l 选项:长型(long)显示 */
#define  OPT_a        0x0008   /* -a 选项:显示所有文件(all)*/
#define  OPT_R        0x0010   /* -R 选项:显示子目录内容 */
#define  OPT_f        0x0020   /* -f 选项:不排序 */


#define DONE_SUCCESS  0


typedef struct _Node
{
	char path[1024];
	char name[256];
	int length;
	struct stat st;
	struct _Node *pnext;
} nNode;

//由nCol构成的链表用于存储每列字符串在终端的宽度
typedef struct _Col {
	int length;//某列字符串在终端的宽度
	struct _Col *pnext;
} nCol;


int createlslink(char *path,int flag,nNode **head)
{
	DIR * dp;
	struct dirent *entry;
	struct stat statbuf;
	nNode *p;
	char  abspath[1024];
	if((dp=opendir(path))==NULL)
	{
		fprintf(stderr,"cannot open directory:%s\n",path);
		return -1;
	}
	
	if(chdir(path) == -1)
	{
		fprintf(stderr,"cannot cd directory:%s\n",path);
		return -2;
	}

	if(NULL==getcwd(abspath,1024))
	{
		fprintf(stderr,"getcwd error!\n");
		return -3;
	}
	
	while((entry=readdir(dp)) != NULL)
	{
		lstat(entry->d_name,&statbuf);
		if((!(flag & OPT_a)) && (entry->d_name[0]=='.'))   /*没有选项 -a 则不显示.开头的文件(或目录)名*/
			continue;
		else
		{
			p = (nNode *)malloc(sizeof(nNode));
			p->pnext = *head;
			*head = p;
			(*head)->length = strlen(entry->d_name);
			strcpy((*head)->path,abspath);
			strcpy((*head)->name,entry->d_name);
			memcpy(&((*head)->st),&statbuf,sizeof(struct stat));
		}
	}

	if(-1==chdir(".."))
	{
		fprintf(stderr,"cannot cd directory:..\n");
		return -3;
	}

	if((closedir(dp)) ==-1)
	{
		fprintf(stderr,"cannot close dp\n");
		return -4;
	}

	return DONE_SUCCESS;
}


void sortlslink(nNode *head,int flag)
{
	nNode *p,*q,r;
	int  msuccess;
	if(!head)
		return;

	if(flag & OPT_f)
	{
		;
	}
	else
	{
		p = head;
		while(p->pnext) p = p->pnext;

		while(p!=head)
		{
			q=head;
			msuccess = 1;
			while(q->pnext)
			{
				if(strcasecmp(q->name,q->pnext->name)>0)
				{
					memcpy(&r,q,sizeof(nNode));
					strcpy(q->name,q->pnext->name);
					strcpy(q->path,q->pnext->path);
					q->length = q->pnext->length;
					memcpy(&q->st,&(q->pnext->st),sizeof(q->st));

					strcpy(q->pnext->name,r.name);
					strcpy(q->pnext->path,r.path);
					q->pnext->length = r.length;
					memcpy(&(q->pnext->st),&(r.st),sizeof(r.st));
					msuccess = 0;
				}

				if(q->pnext==p)
					break;
				q = q->pnext;
			}

			if(msuccess)
				break;
			p = q;
		}
	}
}


void format_long_print(nNode *head,int flag)
{
	nNode * p = head;
	char buf[12];
	struct passwd *password;
	struct group  *grp;
	struct tm * tm_ptr;

	char  linkfilebuf[1024];
	char pathfilename[1024];
	char filename[256];
	if(!p)
		return;

	while(p)
	{
		if( !strcmp(p->name,".") || !strcmp(p->name,".."))
		{
			printf("%s\n",p->name);
		}
		else
		{
			/*permission*/
			strcpy(pathfilename,p->path);
			strcpy(filename,p->name);

			memset(buf,'-',11);
			buf[11]=0;

			if(S_ISDIR(p->st.st_mode))       buf[0] = 'd';
			else if(S_ISCHR(p->st.st_mode))  buf[0] = 'c';
			else if(S_ISBLK(p->st.st_mode))  buf[0] = 'b';
			else if(S_ISFIFO(p->st.st_mode)) buf[0] = 'p';
			else if(S_ISLNK(p->st.st_mode))  buf[0] = 'l';
			else if(S_ISSOCK(p->st.st_mode)) buf[0] = 's';

			if(S_IRUSR&p->st.st_mode) buf[1] = 'r';
			if(S_IWUSR&p->st.st_mode) buf[2] = 'w';
			if(S_IXUSR&p->st.st_mode) buf[3] = 'x';

			if(S_IRGRP&p->st.st_mode) buf[4] = 'r';
			if(S_IWGRP&p->st.st_mode) buf[5] = 'w';
			if(S_IXGRP&p->st.st_mode) buf[6] = 'x';

			if(S_IROTH&p->st.st_mode) buf[7] = 'r';
			if(S_IWOTH&p->st.st_mode) buf[8] = 'w';
			if(S_IXOTH&p->st.st_mode) buf[9] = 'x';

			printf("%s ",buf);

			/*link number*/
			printf("%4d ",p->st.st_nlink);

			/*owner*/
			password = getpwuid(p->st.st_uid);
			if(password)
			{
				printf("%s ",password->pw_name);
			}
			else
			{
				printf("%d ",p->st.st_uid);
			}

			/* group */
			grp= getgrgid(p->st.st_gid);
			if(grp)
			{
				printf("%s ",grp->gr_name);
			}
			else
			{
				printf("%d ",p->st.st_gid);
			}

			/*file bytes or dev number*/
			if(S_ISBLK(p->st.st_mode) || S_ISCHR(p->st.st_mode))
			{
				printf("%3u,",MAJOR(p->st.st_rdev));
				printf("%3u ",MINOR(p->st.st_rdev));
			}
			else
			{
				printf("%7u ",p->st.st_size);
			}

			/*time*/
			tm_ptr = localtime(&(p->st.st_mtime));
			printf("%02d-%02d ",tm_ptr->tm_mon+1,tm_ptr->tm_mday);
			printf("%02d:%02d ",tm_ptr->tm_hour,tm_ptr->tm_min);

			/*filename */
			if(S_ISDIR(p->st.st_mode))
				printf("\033[;34m%s\033[0m\n",filename);
			else
				printf("%s ",filename);

			if(S_ISLNK(p->st.st_mode))
			{
				strcat(pathfilename,"/");
				strcat(pathfilename,p->name);
				int rslt=readlink(pathfilename,linkfilebuf,1024);
				if (rslt<0)
				{
					printf("readlink error!\n");
					exit(-1);
				}

				linkfilebuf[rslt]='\0';
				printf("-> \033[;32m%s\033[0m",linkfilebuf);
			}
			printf("\n");
		}
		p = p->pnext;
	}
	return;
}


//返回nCol链表包含每列字符串在终端的宽度，rows行数
nCol* getColsList(nNode *head, int rows)
{
	int cols = 0;

	nCol *colsList = (nCol*)malloc(sizeof(nCol));
	nCol *pList = colsList;
	nNode * p = head;
	int i = 0;
	int maxCol = 0;

	while(p) {
		if(p->length > maxCol)
			maxCol = p->length;
		i++;
		//在一列内找到最长的字符串
		if(i == rows) {
			cols++;
			pList->pnext = (nCol*)malloc(sizeof(nCol));
			pList = pList->pnext;
			pList->length = maxCol + 2;//字符串在终端的宽度
			maxCol = 0;
			i = 0;
		}
		p = p->pnext;
	}
	if(maxCol != 0) {
		cols++;
		pList->pnext = (nCol*)malloc(sizeof(nCol));
		pList = pList->pnext;
		pList->length = maxCol + 2;
		maxCol = 0;
		i = 0;
	}
	pList->length -= 2;
	colsList->length = cols;

	return colsList;
}

//返回行数
int getRows(nNode *head)
{
	//获得终端宽度
	struct winsize size;
	ioctl(STDIN_FILENO,TIOCGWINSZ,&size);

	int rows = 0;//行数
	int printCol = size.ws_col + 1;//一行最大宽度
	int maxCol = 0;
	int nodeCount = 1;

	//行数增加，计算某行数对应宽度，小于行最大宽度则满足要求
	while(printCol > size.ws_col && rows < nodeCount) {
		rows++;
		printCol = 0;
		nNode * p = head;
		int i = 0;
		while(p) {
			if(p->length > maxCol)
				maxCol = p->length;
			i++;
			if(i == rows) {
				printCol += maxCol + 2;
				maxCol = 0;
				i = 0;
			}
			p = p->pnext;
			if(rows == 1) {
				nodeCount++;
			}
		}
		if(rows == 1) {
			nodeCount--;
		}
		printCol += maxCol + 1;
		if(nodeCount != 0 && nodeCount / rows * rows == nodeCount) {
			printCol -= 2;
		}
		maxCol = 0;
	}

	return rows;
}

//排版
void pageMaker(nNode *head)
{
	//计算行数
	int rows = getRows(head);
	//计算每列宽度
	nCol *colsList = getColsList(head, rows);

	//列宽度转换为数组
	int *colsArray = (int*)malloc(sizeof(int) * colsList->length);
	nCol *pCols = colsList->pnext;
	int i;
	for(i = 0; i < colsList->length; i++) {
		colsArray[i] = pCols->length;
		pCols = pCols->pnext;
	}

	//按行数分组，即最终一列一列输出
	nNode **pColIndex = (nNode**)malloc(sizeof(nNode*) * colsList->length);
	nNode *pNode = head;
	int j;
	nNode *preNode;
	for(i = 0; i < colsList->length; i++) {
		pColIndex[i] = pNode;
		for(j = 0; j < rows && pNode != NULL; j++) {
			if(j == rows - 1) {
				preNode = pNode;
			}
			pNode = pNode -> pnext;
			if(j == rows - 1) {
				preNode -> pnext = NULL;
			}
		}
	}

	//按输出顺序链接节点，文件名之间加入空格
	nNode *Head = (nNode*)malloc(sizeof(nNode));
	nNode *pHead = Head;
	int endCol = colsList->length - 1;
	while(colsList->length != 0 && pColIndex[0] != NULL) {
		for(i = 0; i < colsList->length; i++) {
			if(pColIndex[i] != NULL) {
				int addLength = colsArray[i] - pColIndex[i] -> length;
				int j;
				for(j = 0; j < addLength; j++) {
					(pColIndex[i] -> name)[pColIndex[i] -> length] = ' ';
					(pColIndex[i] -> name)[(pColIndex[i] -> length) + 1] = '\0';
					(pColIndex[i] -> length)++;
				}
				if(i == endCol) {
					(pColIndex[i] -> name)[pColIndex[i] -> length] = '\n';
					(pColIndex[i] -> name)[(pColIndex[i] -> length) + 1] = '\0';
					(pColIndex[i] -> length)++;
				}

				pHead -> pnext = pColIndex[i];
				pColIndex[i] = pColIndex[i] -> pnext;
				pHead = pHead -> pnext;
			} else if(endCol == colsList->length - 1) {
				endCol = colsList->length - 2;
				(pHead -> name)[pHead -> length] = '\n';
				(pHead -> name)[(pHead -> length) + 1] = '\0';
				(pHead -> length)++;
			}
		}
	}

	head = Head -> pnext;
}


void format_normal_print(nNode *head,int flag)
{
	//排版
	pageMaker(head);

	nNode * p = head;

	while(p)
	{
		if(  ( (strcmp(p->name,".")==0) || (strcmp(p->name,"..")==0) ) && (flag & OPT_a))
		{
			printf("%s",p->name);
		}
		else
		{
			if(S_ISDIR(p->st.st_mode))
				printf("\033[;32m%s\033[0m",p->name);
			else
				printf("%s",p->name);
		}
		p = p->pnext;
	}

	return;
}


void showlslink(nNode *head,int flag)
{
	if(flag & OPT_l)
		format_long_print(head,flag);
	else
		format_normal_print(head,flag);
	return;
}


void lspath(char *path,int flag)
{
	char pathname[1024];
	char filename[256];

	nNode *p = NULL;
	nNode *head = NULL;

	/*创建列文件链表 head*/
	if ((createlslink(path,flag,&head)) != DONE_SUCCESS)
	{
		printf("createlslink error!\n");
		return;
	}

	/* 对链表排序 */
	sortlslink(head,flag);

	/* 按选项显示链表内容  */
	showlslink(head,flag);

	/*如果节点不是子目录，则删除节点，否则当有-R选项时,列子目录内容后删除节点  */
	while(head)
	{
		p = head;
		head = head->pnext;

		if(S_ISDIR(p->st.st_mode) && (flag & OPT_R))
		{
			if( strcmp(p->name,".") && strcmp(p->name,"..") )
			{
				strcpy(pathname,p->path);
				strcat(pathname,"/");
				strcat(pathname,p->name);
				printf("\033[;32m%s:\033[0m\n",pathname);
				lspath(pathname,flag);
			}
		}

		free(p);
	}

	return;
}


//输入参数的排序算法
int cmp (const void *arg1 , const void *arg2)
{
	return -strcmp((*(char **)arg1), (*(char **)arg2));
}


int main(int argc,char *argv[])
{
	int opt;
	char path[1024];
	int listflag = 0;
	struct stat statbuf;
	nNode *phead = NULL;//保存输入的文件名
	nNode *pdhead = NULL;//保存输入的文件夹名

	struct option longopts[] = { {"help",0,NULL,'h'},{"version",0,NULL,'v'},{0,0,0,0}};

	while((opt=getopt_long(argc,argv,"afhlvR",longopts,NULL)) != -1)
	{
		switch(opt)
		{
		case 'a':
			listflag |= OPT_a;
			break;

		case 'f':
			listflag |= OPT_f;
			break;

		case 'h':
			listflag |= OPT_h;
			break;

		case 'l':
			listflag |= OPT_l;
			break;

		case 'v':
			listflag |= OPT_v;
			break;

		case 'R':
			listflag |= OPT_R;
			break;

		case '?':
			printf("Usage:\n %s -h \n or \n  %s --help \nfor help\n",argv[0],argv[0]);
			exit(-1);
		}
	}

	if(listflag & OPT_h )
	{
		printf("Usage:  %s -afhlvR [directory1 [directory2],...]\n",argv[0]);
		return 0;
	}
	else if(listflag & OPT_v)
	{
		printf("myls version 0.1\nCopyright by YCG 2015\n");
		return 0;
	}

	if(optind==argc)
	{
		/* 列当前目录文件 */
		if(NULL==getcwd(path,1024))
			return -1;

		lspath(path,listflag);
	}
	else
	{
		//对输入参数排序
		qsort(argv+optind, argc-optind, sizeof(char*), cmp);

		for(; optind<argc; optind++)
		{
			if(lstat(argv[optind],&statbuf)<0)
			{
				fprintf(stderr,"stat error\n");
				return -1;
			}
			if(S_ISDIR(statbuf.st_mode))
			{
				//文件夹加入pdhead指向的链表
				nNode *p = (nNode*)malloc(sizeof(nNode));
				p->length = strlen(argv[optind]);
				strcpy(p->name,argv[optind]);
				p->pnext = pdhead;
				pdhead = p;
			}
			else
			{
				//文件加入phead指向的链表
				nNode *p = (nNode*)malloc(sizeof(nNode));
				p->length = strlen(argv[optind]);
				strcpy(p->name,argv[optind]);
				memcpy(&(p->st),&statbuf,sizeof(struct stat));
				p->pnext = phead;
				phead = p;
			}
		}
		
		//输出文件
		if(phead!=NULL)
		{
			sortlslink(phead,listflag);
			showlslink(phead,listflag);
			if(argc>2 && pdhead!=NULL)
				printf("\n");
		}
		//输出文件夹
		while(pdhead!=NULL)
		{
			if(argc>2)
				printf("%s:\n", pdhead->name);
			lspath(pdhead->name,listflag);
			pdhead = pdhead->pnext;
			if(argc>2 && pdhead!=NULL)
				printf("\n");
		}
	}

	return 0;
}
