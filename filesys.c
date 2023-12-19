#include <stdlib.h>
#include <string.h>
#include "time.h"
#include <stdio.h>

#define BLOCKSIZE       1024        
#define SIZE            1024000     
#define END             65535       
#define FREE            0           
#define MAXOPENFILE     10          
#define MAX_TEXT_SIZE  10000        

typedef struct FCB {
    char filename[8];           
    char exname[3];             
    unsigned char metadata;     
    unsigned short time;        
    unsigned short date;        
    unsigned short first;       
    unsigned long length;       
    char free;                  
} fcb;
typedef struct FAT {
    unsigned short id;   
} fat;
typedef struct USEROPEN {
    char filename[8];           
    char exname[3];             
    unsigned char  metadata;    
    unsigned short time;        
    unsigned short date;        
    unsigned short first;       
    unsigned long  length;      
    char free;                  
    int  dirno;                 
    int  diroff;                
    char dir[80];               
    int  filePtr;               
    char fcbstate;              
    char topenfile;             
} useropen;
typedef struct BootBlock {
    char magic_number[8];       
    char information[200];      
    unsigned short root;        
    unsigned char *startblock;  
} block0;
/***** 全局变量定义 *****/
unsigned char *v_addr0;             
useropen openfilelist[MAXOPENFILE]; 
int currfd;                         
unsigned char *startp;              
char *FILENAME = "FileSys.txt";     
unsigned char buffer[SIZE];         
/***** 函数申明 *****/
int  main();
void startsys();                                     
void my_format();                                    
void my_cd(char *dirname);                           
void my_mkdir(char *dirname);                        
void my_rmdir(char *dirname);                        
void my_ls();                                        
int  my_create(char *filename);                      
void my_rm(char *filename);                          
int  my_open(char *filename);                        
int  my_close(int fd);                               
int  my_write(int fd);                               
int  do_write(int fd,char *text,int len,char wstyle);
int  my_read(int fd);                                
int  do_read(int fd, int len, char *text);           
void my_exitsys();                                   
unsigned short getFreeBLOCK();                       
int  get_Free_Openfile();                            
int  find_father_dir(int fd);                        
void show_help();                                    
void error(char *command);                           
unsigned short int getFreeBLOCK(){
    fat* fat1 = (fat*)(v_addr0 + BLOCKSIZE);
    for(int i=0; i < (int)(SIZE/BLOCKSIZE); i++){
        if(fat1[i].id == FREE){
            return i;
        }
    }
    return END;
}
int  get_Free_Openfile(){
    for(int i=0; i<MAXOPENFILE; i++){
        if(openfilelist[i].topenfile == 0){
            openfilelist[i].topenfile = 1;
            return i;
        }
    }
    return -1;
}
int  find_father_dir(int fd){
    for(int i=0; i<MAXOPENFILE; i++){
        if(openfilelist[i].first == openfilelist[fd].dirno){
            return i;
        }
    }
    return -1;
}
void show_help(){
    printf("命令名\t\t命令参数\t\t命令功能\n\n");
    printf("cd\t\t目录名(路径名)\t\t切换当前目录到指定目录\n");
    printf("mkdir\t\t目录名\t\t\t在当前目录创建新目录\n");
    printf("rmdir\t\t目录名\t\t\t在当前目录删除指定目录\n");
    printf("ls\t\t无\t\t\t显示当前目录下的目录和文件\n");
    printf("create\t\t文件名\t\t\t在当前目录下创建指定文件\n");
    printf("rm\t\t文件名\t\t\t在当前目录下删除指定文件\n");
    printf("open\t\t文件名\t\t\t在当前目录下打开指定文件\n");
    printf("write\t\t无\t\t\t在打开文件状态下，写该文件\n");
    printf("read\t\t无\t\t\t在打开文件状态下，读取该文件\n");
    printf("close\t\t无\t\t\t在打开文件状态下，关闭该文件\n");
    printf("exit\t\t无\t\t\t退出系统\n\n");
}
void error(char *command){
    printf("%s : 缺少参数\n", command);
    printf("输入 'help' 来查看命令提示.\n");
}
void startsys(){
    v_addr0 = (unsigned char *)malloc(SIZE);  
    printf("开始读取文件...");
    FILE *file;
    if((file = fopen(FILENAME, "r")) != NULL){
        fread(buffer, SIZE, 1, file);   
        fclose(file);
        if(memcmp(buffer,"10101010",8) == 0){
            memcpy(v_addr0,buffer,SIZE);
            cout << "myfsys文件读取成功!" <<endl;
        }
        else{
            cout << "myfsys文件系统不存在，现在开始创建文件系统" <<endl;
            my_format();
            memcpy(buffer,v_addr0,SIZE);
        }
    }
    else{
        cout << "myfsys文件系统不存在，现在开始创建文件系统" <<endl;
        my_format();
        memcpy(buffer,v_addr0,SIZE);
    }
    fcb *root;
    root = (fcb *)(v_addr0 + 5 * BLOCKSIZE);
    strcpy(openfilelist[0].filename, root->filename);
    strcpy(openfilelist[0].exname, root->exname);
    openfilelist[0].metadata = root->metadata;
    openfilelist[0].time = root->time;
    openfilelist[0].date = root->date;
    openfilelist[0].first = root->first;
    openfilelist[0].length = root->length;
    openfilelist[0].free = root->free;
    openfilelist[0].dirno = 5;
    openfilelist[0].diroff = 0;
    strcpy(openfilelist[0].dir, "\\root\\");
    openfilelist[0].filePtr = 0;
    openfilelist[0].fcbstate = 0;
    openfilelist[0].topenfile = 1;
    startp = ((block0*)v_addr0)->startblock;
    currfd = 0;
    return ;
}
void my_format(){
    block0 *boot = (block0 *)v_addr0;
    strcpy(boot->magic_number,"10101010");
    strcpy(boot->information,"文件系统,外存分配方式:FAT,磁盘空间管理:结合于FAT的位示图,目录结构:单用户多级目录结构.");
    boot->root = 5;
    boot->startblock = v_addr0 + BLOCKSIZE*5;
    fat* fat1 = (fat*)(v_addr0 + BLOCKSIZE);
    for(int i=0; i<5; i++){  
        fat1[i].id = END;
    }
    for(int i=5; i<1000; i++){
        fat1[i].id = FREE;
    }
    fat* fat2 = (fat*)(v_addr0 + BLOCKSIZE*3);
    memcpy(fat2,fat1,BLOCKSIZE);
    fat1[5].id = fat2[5].id = END;
    fcb *root = (fcb*)(v_addr0 + BLOCKSIZE*5);
    strcpy(root->filename,".");
    strcpy(root->exname,"di");
    root->metadata = 0;
    time_t rawTime = time(NULL);
    struct tm *time = localtime(&rawTime);
    root->time = time->tm_hour * 2048 + time->tm_min*32 + time->tm_sec/2;
    root->date = (time->tm_year-100)*512 + (time->tm_mon+1)*32 + (time->tm_mday);
    root->first = 5;
    root->free = 1;
    root->length = 2 * sizeof(fcb);
    fcb* root2 = root + 1;
    memcpy(root2,root,sizeof(fcb));
    strcpy(root2->filename,"..");
    for(int i=2; i < int(BLOCKSIZE / sizeof(fcb)); i++){
        root2 ++;
        strcpy(root2->filename,"");
        root2->free = 0;
    }
    FILE *fp = fopen(FILENAME, "w");
    fwrite(v_addr0, SIZE, 1, fp);
    fclose(fp);
}
void my_cd(char *dirname){
    if(openfilelist[currfd].metadata == 1){
        cout << "数据文件里不能使用cd, 要是退出文件, 请用close指令" <<endl;
        return ;
    }
    else{
        char *buf = (char *)malloc(MAX_TEXT_SIZE);
        openfilelist[currfd].filePtr = 0;
        do_read(currfd,openfilelist[currfd].length,buf);
        int i = 0;
        fcb* fcbPtr = (fcb*)buf;
        for(; i < int(openfilelist[currfd].length / sizeof(fcb)); i++,fcbPtr++){
            if(strcmp(fcbPtr->filename, dirname) == 0 && fcbPtr->metadata == 0){
                break;
            }
        }
        if(strcmp(fcbPtr->exname, "di") != 0){
            cout << "不允许cd非目录文件!" <<endl;
            return;
        }
        else{
            if(strcmp(fcbPtr->filename,".") == 0){
                return;
            }
            else if(strcmp(fcbPtr->filename, "..") == 0){
                if(currfd == 0){
                    return;
                }
                else{
                    currfd = my_close(currfd);
                    return;
                }
            }
            else{
                int fd = get_Free_Openfile();
                if(fd == -1){
                    return;
                }
                else{
                    openfilelist[fd].metadata = fcbPtr->metadata;
                    openfilelist[fd].filePtr = 0;
                    openfilelist[fd].date = fcbPtr->date;
                    openfilelist[fd].time = fcbPtr->time;
                    strcpy(openfilelist[fd].filename, fcbPtr->filename);
                    strcpy(openfilelist[fd].exname,fcbPtr->exname);
                    openfilelist[fd].first = fcbPtr->first;
                    openfilelist[fd].free = fcbPtr->free;
                    openfilelist[fd].fcbstate = 0;
                    openfilelist[fd].length = fcbPtr->length;
                    strcpy(openfilelist[fd].dir,
                   (char*)(string(openfilelist[currfd].dir) + string(dirname) + string("\\")).c_str());
                    openfilelist[fd].topenfile = 1;
                    openfilelist[fd].dirno = openfilelist[currfd].first;
                    openfilelist[fd].diroff = i;
                    currfd = fd;
                }
            }
        }
    }
}
void my_mkdir(char *dirname){
    char* fname = strtok(dirname,".");
    char* exname = strtok(NULL,".");
    if(exname){
        cout << "不允许输入后缀名!" << endl;
        return ;
    }
    char text[MAX_TEXT_SIZE];
    openfilelist[currfd].filePtr = 0;
    int fileLen = do_read(currfd, openfilelist[currfd].length, text);
    fcb *fcbPtr = (fcb*)text;
    for(int i=0; i < (int)(fileLen/sizeof(fcb)); i++){
        if(strcmp(dirname,fcbPtr[i].filename) == 0 && fcbPtr->metadata == 0){
            cout << "目录名已经存在!"<<endl;
            return;
        }
    }
    int fd = get_Free_Openfile();
    if(fd == -1){
        cout << "打开文件表已全部被占用" << endl;
        return;
    }
    unsigned short int blockNum = getFreeBLOCK();
    if(blockNum == END){
        cout << "盘块已经用完" << endl;
        openfilelist[fd].topenfile = 0;
        return ;
    }
    fat *fat1 = (fat *)(v_addr0 + BLOCKSIZE);
    fat *fat2 = (fat *)(v_addr0 + BLOCKSIZE*3);
    fat1[blockNum].id = END;
    fat2[blockNum].id = END;
    int i = 0;
    for(; i < (int)(fileLen/sizeof(fcb)); i++){
        if(fcbPtr[i].free == 0){
            break;
        }
    }
    openfilelist[currfd].filePtr = i * sizeof(fcb);
    openfilelist[currfd].fcbstate = 1;
    fcb* fcbtmp = new fcb;
    fcbtmp->metadata = 0;
    time_t rawtime = time(NULL);
    struct tm* time = localtime(&rawtime);
    fcbtmp->date = (time->tm_year-100)*512 + (time->tm_mon+1)*32 + (time->tm_mday);
    fcbtmp->time = (time->tm_hour)*2048 + (time->tm_min)*32 + (time->tm_sec) / 2;
    strcpy(fcbtmp->filename , dirname);
    strcpy(fcbtmp->exname, "di");
    fcbtmp->first = blockNum;
    fcbtmp->length = 2 * sizeof(fcb);
    fcbtmp->free = 1;
    do_write(currfd,(char *)fcbtmp,sizeof(fcb),1);
    openfilelist[fd].metadata = 0;
    openfilelist[fd].filePtr = 0;
    openfilelist[fd].date = fcbtmp->date;
    openfilelist[fd].time = fcbtmp->time;
    openfilelist[fd].dirno = openfilelist[currfd].first;
    openfilelist[fd].diroff = i;
    strcpy(openfilelist[fd].exname,"di");
    strcpy(openfilelist[fd].filename,dirname);
    openfilelist[fd].fcbstate = 0;
    openfilelist[fd].first = fcbtmp->first;
    openfilelist[fd].free = fcbtmp->free;
    openfilelist[fd].length = fcbtmp->length;
    openfilelist[fd].topenfile = 1;
    strcpy(openfilelist[fd].dir, (char*)(string(openfilelist[currfd].dir) + string(dirname) + string("\\")).c_str());
    fcbtmp->metadata = 0;
    fcbtmp->date = fcbtmp->date;
    fcbtmp->time = fcbtmp->time;
    strcpy(fcbtmp->filename, ".");
    strcpy(fcbtmp->exname, "di");
    fcbtmp->first = blockNum;
    fcbtmp->length = 2 * sizeof(fcb);
    do_write(fd,(char*)fcbtmp,sizeof(fcb),1);
    fcb *fcbtmp2 = new fcb;
    memcpy(fcbtmp2,fcbtmp,sizeof(fcb));
    strcpy(fcbtmp2->filename,"..");
    fcbtmp2->first = openfilelist[currfd].first;
    fcbtmp2->length = openfilelist[currfd].length;
    fcbtmp2->date = openfilelist[currfd].date;
    fcbtmp2->time = openfilelist[currfd].time;
    do_write(fd,(char*)fcbtmp2,sizeof(fcb),1);
    my_close(fd);
    fcbPtr = (fcb *)text;
    fcbPtr->length =  openfilelist[currfd].length;
    openfilelist[currfd].filePtr = 0;
    do_write(currfd,(char*)fcbPtr,sizeof(fcb),1);
    openfilelist[currfd].fcbstate = 1;
    delete fcbtmp;
    delete fcbtmp2;
}
void my_rmdir(char *dirname){
    char* fname = strtok(dirname, ".");
    char* exname = strtok(NULL, ".");
    if(strcmp(dirname,".") == 0 || strcmp(dirname,"..") == 0){
        cout << "不能删除" << dirname <<"这个特殊目录项" <<endl;
        return ;
    }
    if(exname){
        cout << "删除目录文件不用输入后缀名!" << endl;
        return;
    }
    char buf[MAX_TEXT_SIZE];
    openfilelist[currfd].filePtr = 0;
    do_read(currfd,openfilelist[currfd].length,buf);
    int i;
    fcb* fcbPtr = (fcb*)buf;
    for(i=0; i < int(openfilelist[currfd].length / sizeof(fcb)); i++, fcbPtr++){
        if(strcmp(fcbPtr->filename,fname) == 0  && fcbPtr->metadata == 0){
            break;
        }
    }
    if( i == int(openfilelist[currfd].length / sizeof(fcb))){
        cout << "没有这个目录文件" <<endl;
        return;
    }
    if(fcbPtr->length > 2 * sizeof(fcb)){
        cout << "请先清空这个目录下的所有文件,再删除目录文件" << endl;
        return;
    }
    int blockNum = fcbPtr->first;
    fat* fat1 = (fat*)(v_addr0 + BLOCKSIZE);
    int next = 0;
    while(1){
        next = fat1[blockNum].id;
        fat1[blockNum].id = END;
        if(next != END){
            blockNum = next;
        }
        else{
            break;
        }
    }
    fat1 = (fat*)(v_addr0 + BLOCKSIZE);
    fat* fat2 = (fat*)(v_addr0 + BLOCKSIZE*3);
    memcpy(fat2, fat1, sizeof(fat));
    fcbPtr->date = 0;
    fcbPtr->time = 0;
    fcbPtr->exname[0] = '\0';
    fcbPtr->filename[0] = '\0';
    fcbPtr->first = 0;
    fcbPtr->free = 0;
    fcbPtr->length = 0;
    openfilelist[currfd].filePtr = i * sizeof(fcb);
    do_write(currfd,(char*)fcbPtr,sizeof(fcb),1);
    openfilelist[currfd].length -= sizeof(fcb);
    fcbPtr = (fcb*)buf;
    fcbPtr->length = openfilelist[currfd].length;
    openfilelist[currfd].filePtr = 0;
    do_write(currfd,(char*)fcbPtr,sizeof(fcb),1);
    openfilelist[currfd].fcbstate = 1;
}
void my_ls(){
    if(openfilelist[currfd].metadata == 1){
        cout << "在数据文件里不能使用ls" << endl;
        return;
    }
    char buf[MAX_TEXT_SIZE];
    openfilelist[currfd].filePtr = 0;
    do_read(currfd, openfilelist[currfd].length, buf);
    fcb* fcbPtr = (fcb*)buf;
    printf("name\tsize \ttype\t\tdate\t\ttime\n");
    for(int i=0; i < (int)(openfilelist[currfd].length / sizeof(fcb)); i++){
        if(fcbPtr->free == 1){
            if(fcbPtr->metadata == 0){
                printf("%s\t%dB\t<DIR>\t%d/%d/%d\t%02d:%02d:%02d\n",
                       fcbPtr->filename,fcbPtr->length,
                       (fcbPtr->date>>9)+2000,
                       (fcbPtr->date>>5)&0x000f,
                       (fcbPtr->date)&0x001f,
                       (fcbPtr->time>>11),
                       (fcbPtr->time>>5)&0x003f,
                       (fcbPtr->time)&0x001f * 2);
            }
            else{
                unsigned int length = fcbPtr->length;
                if(length != 0)length -= 2;
                printf("%s.%s\t%dB\t<File>\t%d/%d/%d\t%02d:%02d:%02d\n",
                       fcbPtr->filename,
                       fcbPtr->exname,
                       length,
                       (fcbPtr->date>>9)+2000,
                       (fcbPtr->date>>5)&0x000f,
                       (fcbPtr->date)&0x001f,
                       (fcbPtr->time>>11),
                       (fcbPtr->time>>5)&0x003f,
                       (fcbPtr->time)&0x001f * 2);
            }
        }
        fcbPtr++;
    }
}
int  my_create(char *filename){
    char* fname = strtok(filename,".");
    char* exname = strtok(NULL,".");
    if(strcmp(fname,"") == 0){
        cout << "请输入文件名!" << endl;
        return -1;
    }
    if(!exname){
        cout << "请输入后缀名!" << endl;
        return -1;
    }
    if(openfilelist[currfd].metadata == 1){
        cout << "数据文件下不允许使用create" << endl;
        return -1;
    }
    openfilelist[currfd].filePtr = 0;
    char buf[MAX_TEXT_SIZE];
    do_read(currfd, openfilelist[currfd].length, buf);
    int i;
    fcb* fcbPtr = (fcb*)(buf);
    for(i=0; i < int(openfilelist[currfd].length / sizeof(fcb)); i++,fcbPtr++){
        if(strcmp(fcbPtr->filename,filename)==0 && strcmp(fcbPtr->exname,exname)==0){
            cout << "已有同名文件存在!" << endl;
            return -1;
        }
    }
    fcbPtr = (fcb*)(buf);
    fcb* debug = (fcb*)(buf);
    for(i=0; i < int(openfilelist[currfd].length / sizeof(fcb)); i++,fcbPtr++){
        if(fcbPtr->free == 0)break;
    }
    int blockNum = getFreeBLOCK();
    if(blockNum == -1){
        return -1;
    }
    fat* fat1 = (fat*)(v_addr0 + BLOCKSIZE);
    fat1[blockNum].id = END;
    fat* fat2 = (fat*)(v_addr0 + BLOCKSIZE * 3);
    memcmp(fat2, fat1, BLOCKSIZE*2);
    strcpy(fcbPtr->filename,filename);
    strcpy(fcbPtr->exname,exname);
    time_t rawtime = time(NULL);
    struct tm* time = localtime(&rawtime);
    fcbPtr->date = (time->tm_year-100)*512 + (time->tm_mon+1)*32 + (time->tm_mday);
    fcbPtr->time = (time->tm_hour)*2048 + (time->tm_min)*32 + (time->tm_sec) / 2;
    fcbPtr->first = blockNum;
    fcbPtr->free = 1;
    fcbPtr->length = 0;
    fcbPtr->metadata = 1;
    openfilelist[currfd].filePtr = i * sizeof(fcb);
    do_write(currfd,(char *)fcbPtr,sizeof(fcb),1);
    fcbPtr = (fcb*)buf;
    fcbPtr->length = openfilelist[currfd].length;
    openfilelist[currfd].filePtr = 0;
    do_write(currfd,(char*)fcbPtr,sizeof(fcb),1);
    openfilelist[currfd].fcbstate = 1;
}
void my_rm(char *filename){
    char* fname = strtok(filename, ".");
    char* exname = strtok(NULL, ".");
    if(!exname){
        cout << "请输入后缀名!" << endl;
        return;
    }
    if(strcmp(exname,"di") == 0){
        cout << "不能删除目录项" << endl;
        return ;
    }
    char buf[MAX_TEXT_SIZE];
    openfilelist[currfd].filePtr = 0;
    do_read(currfd,openfilelist[currfd].length,buf);
    int i;
    fcb* fcbPtr = (fcb*)buf;
    for(i=0; i < int(openfilelist[currfd].length / sizeof(fcb)); i++, fcbPtr++){
        if(strcmp(fcbPtr->filename,fname) == 0  && strcmp(fcbPtr->exname,exname) == 0){
            break;
        }
    }
    if( i == int(openfilelist[currfd].length / sizeof(fcb))){
        cout << "没有这个文件" <<endl;
        return;
    }
    int blockNum = fcbPtr->first;
    fat* fat1 = (fat*)(v_addr0 + BLOCKSIZE);
    int next = 0;
    while(1){
        next = fat1[blockNum].id;
        fat1[blockNum].id = FREE;
        if(next != END){
            blockNum = next;
        }
        else{
            break;
        }
    }
    fat1 = (fat*)(v_addr0 + BLOCKSIZE);
    fat* fat2 = (fat*)(v_addr0 + BLOCKSIZE*3);
    memcpy(fat2, fat1, sizeof(fat));
    fcbPtr->date = 0;
    fcbPtr->time = 0;
    fcbPtr->exname[0] = '\0';
    fcbPtr->filename[0] = '\0';
    fcbPtr->first = 0;
    fcbPtr->free = 0;
    fcbPtr->length = 0;
    openfilelist[currfd].filePtr = i * sizeof(fcb);
    do_write(currfd,(char*)fcbPtr,sizeof(fcb),1);
    openfilelist[currfd].length -= sizeof(fcb);
    fcbPtr = (fcb*)buf;
    fcbPtr->length = openfilelist[currfd].length;
    openfilelist[currfd].filePtr = 0;
    do_write(currfd,(char*)fcbPtr,sizeof(fcb),1);
    openfilelist[currfd].fcbstate = 1;
}
int  my_open(char *filename){
    char buf[MAX_TEXT_SIZE];
    openfilelist[currfd].filePtr = 0;
    do_read(currfd, openfilelist[currfd].length, buf);
    char *fname = strtok(filename,".");
    char *exname = strtok(NULL, ".");
    if(!exname){
        cout << "请输入后缀名" << endl;
        return -1 ;
    }
    int i;
    fcb* fcbPtr = (fcb*)buf;
    for(i=0; i < int(openfilelist[currfd].length / sizeof(fcb)); i++, fcbPtr++){
        if(strcmp(fcbPtr->filename,fname) == 0 && strcmp(fcbPtr->exname,exname) == 0 && fcbPtr->metadata == 1){
            break;
        }
    }
    if(i == int(openfilelist[currfd].length / sizeof(fcb))){
        cout << "不存在此文件!" << endl;
        return -1;
    }
    int fd = get_Free_Openfile();
    if(fd == -1){
        cout << "用户打开文件表已经用满" <<endl;
        return -1;
    }
    openfilelist[fd].metadata = 1;
    openfilelist[fd].filePtr = 0;
    openfilelist[fd].date = fcbPtr->date;
    openfilelist[fd].time = fcbPtr->time;
    strcpy(openfilelist[fd].exname, exname);
    strcpy(openfilelist[fd].filename,fname);
    openfilelist[fd].length = fcbPtr->length;
    openfilelist[fd].first = fcbPtr->first;
    strcpy(openfilelist[fd].dir,(string(openfilelist[currfd].dir) + string(filename)).c_str());
    openfilelist[fd].dirno = openfilelist[currfd].first;
    openfilelist[fd].diroff = i;
    openfilelist[fd].free = 1;
    openfilelist[fd].topenfile = 1;
    openfilelist[fd].fcbstate = 0;
    currfd = fd;
    return 1;
}
int  my_close(int fd){
    if(fd > MAXOPENFILE || fd < 0){
        cout << "不存在这个打开文件" << endl;
        return-1;
    }
    else{
        int fatherFd = find_father_dir(fd);
        if(fatherFd == -1){
            cout << "父目录不存在!" <<endl;
            return -1;
        }
        if(openfilelist[fd].fcbstate == 1){
            char buf[MAX_TEXT_SIZE];
            do_read(fatherFd,openfilelist[fatherFd].length, buf);
            fcb* fcbPtr = (fcb *)(buf + sizeof(fcb) * openfilelist[fd].diroff);
            strcpy(fcbPtr->exname, openfilelist[fd].exname);
            strcpy(fcbPtr->filename, openfilelist[fd].filename); 
            fcbPtr->first = openfilelist[fd].first;
            fcbPtr->free = openfilelist[fd].free;
            fcbPtr->length = openfilelist[fd].length;
            openfilelist[fatherFd].filePtr = 0;
            fcbPtr->time = openfilelist[fd].time;
            fcbPtr->date = openfilelist[fd].date;
            fcbPtr->metadata = openfilelist[fd].metadata;
            openfilelist[fatherFd].filePtr = openfilelist[fd].diroff * sizeof(fcb);
            do_write(fatherFd, (char*)fcbPtr, sizeof(fcb), 1);
        }
        memset(&openfilelist[fd], 0, sizeof(USEROPEN));
        currfd = fatherFd;
        return fatherFd;
    }
}
int  my_write(int fd){
    if(fd < 0 || fd >= MAXOPENFILE){
        cout << "文件不存在" <<endl;
        return -1;
    }
    int wstyle;
    while(1){
        cout << "输入: 0=截断写, 1=覆盖写, 2=追加写" <<endl;
        cin >> wstyle;
        if(wstyle > 2 || wstyle < 0){
            cout << "指令错误!" << endl;
        }
        else{
            break;
        }
    }
    char text[MAX_TEXT_SIZE] = "\0";
    char textTmp[MAX_TEXT_SIZE] = "\0";
    char Tmp[MAX_TEXT_SIZE]  = "\0"; 
    char Tmp2[4]             = "\0"; 
    cout << "请输入文件数据, 以END为文件结尾" <<endl;
    getchar();
    while(fgets(Tmp,100,stdin)){
    	for(int i=0;i<strlen(Tmp)-1;i++)
        {
              textTmp[i]   = Tmp[i];
              textTmp[i+1] = '\0';
        }
        if(strlen(Tmp) >= 3)
        {
            Tmp2[0] = Tmp[strlen(Tmp)-4];
            Tmp2[1] = Tmp[strlen(Tmp)-3];
            Tmp2[2] = Tmp[strlen(Tmp)-2];
            Tmp2[3] = '\0';
        }
    	if(strcmp(textTmp,"END")==0 || strcmp(Tmp2,"END")==0)
        {
        	break;
        }
        textTmp[strlen(textTmp)] = '\n';
        strcat(text,textTmp);
    }
    text[strlen(text)] = '\0';
    do_write(fd,text,strlen(text)+1,wstyle);
    openfilelist[fd].fcbstate = 1;
    return 1;
}
int  do_write(int fd, char *text, int len, char wstyle){
    int blockNum = openfilelist[fd].first;
    fat *fatPtr = (fat*)(v_addr0 + BLOCKSIZE) + blockNum ;
    fat* fat1 = (fat*)(v_addr0 + BLOCKSIZE);
    if(wstyle == 0){
        openfilelist[fd].filePtr = 0;
        openfilelist[fd].length = 0;
    }
    else if(wstyle == 1){
        if(openfilelist[fd].metadata == 1 && openfilelist[fd].length != 0){
            openfilelist[fd].filePtr -= 1;
        }
    }
    else if(wstyle == 2){
        if(openfilelist[fd].metadata == 0){
            openfilelist[fd].filePtr = openfilelist[fd].length;
        }
        else if(openfilelist[fd].metadata == 1 && openfilelist[fd].length != 0){
            openfilelist[fd].filePtr = openfilelist[fd].length - 1;
        }
    }
    int off = openfilelist[fd].filePtr;									
    while(off >= BLOCKSIZE){
        blockNum = fatPtr->id;
        if(blockNum == END){
            blockNum = getFreeBLOCK();
            if(blockNum == END){
                cout << "盘块不足"<<endl;
                return -1;
            }
            else{
                fatPtr->id = blockNum;
                fatPtr = (fat*)(v_addr0 + BLOCKSIZE + blockNum);
                fatPtr->id = END;
            }
        }
        fatPtr = (fat*)(v_addr0 + BLOCKSIZE) + blockNum;
        off -= BLOCKSIZE;
    }
    unsigned char *buf = (unsigned char*)malloc(BLOCKSIZE * sizeof(unsigned char));
    if(buf == NULL){
        cout << "申请内存空间失败!";
        return -1;
    }
    fcb *dBlock = (fcb *)(v_addr0 + BLOCKSIZE * blockNum);
    fcb *dFcb = (fcb *)(text);
    unsigned char *blockPtr = (unsigned char *)(v_addr0 + BLOCKSIZE * blockNum);					
    int lenTmp = 0;
    char *textPtr = text;
    fcb *dFcbBuf = (fcb *)(buf);
    while(len > lenTmp){
        memcpy(buf,blockPtr,BLOCKSIZE);
        for (; off < BLOCKSIZE; off++){
            *(buf + off) = *textPtr;
            textPtr ++;
            lenTmp++;
            if(len == lenTmp){
                break;
            }
        }
        memcpy(blockPtr, buf, BLOCKSIZE);
        if(off == BLOCKSIZE && len != lenTmp){
            off = 0;
            blockNum = fatPtr->id;
            if(blockNum == END){
                blockNum = getFreeBLOCK();
                if(blockNum == END){
                    cout << "盘块用完了" <<endl;
                    return -1;
                }
                else{
                    blockPtr = (unsigned char *)(v_addr0 + BLOCKSIZE * blockNum);
                    fatPtr->id = blockNum;
                    fatPtr = (fat *)(v_addr0 + BLOCKSIZE) + blockNum;
                    fatPtr->id = END;
                }
            }
            else{
                blockPtr = (unsigned char *)(v_addr0 + BLOCKSIZE * blockNum);
                fatPtr = (fat *)(v_addr0 + BLOCKSIZE ) + blockNum;
            }
        }
    }
    openfilelist[fd].filePtr += len;
    if(openfilelist[fd].filePtr > openfilelist[fd].length)
        openfilelist[fd].length = openfilelist[fd].filePtr;
    free(buf);
    int i = blockNum;
    while (1){
        if(fat1[i].id != END){
            int next = fat1[i].id;
            fat1[i].id = FREE;
            i = next;
        }
        else{
            break;
        }
    }
    fat1[blockNum].id = END;
    memcpy((fat*)(v_addr0 + BLOCKSIZE * 3), (fat*)(v_addr0 + BLOCKSIZE), 2*BLOCKSIZE);
    return len;
}
int  do_read(int fd, int len, char *text){
    int lenTmp = len;
    unsigned char* buf = (unsigned char*)malloc(1024);
    if(buf == NULL){
        cout << "do_read申请内存空间失败" << endl;
        return -1;
    }
    int off = openfilelist[fd].filePtr;
    int blockNum = openfilelist[fd].first;
    fat* fatPtr = (fat *)(v_addr0+BLOCKSIZE) + blockNum;
    while(off >= BLOCKSIZE){
        off -= BLOCKSIZE;
        blockNum = fatPtr->id;
        if(blockNum == END){
            cout <<"do_read寻找的块不存在" <<endl;
            return -1;
        }
        fatPtr = (fat*)(v_addr0 + BLOCKSIZE) + blockNum;
    }
    unsigned char* blockPtr = v_addr0 + BLOCKSIZE*blockNum;
    memcpy(buf, blockPtr, BLOCKSIZE);
    char *textPtr = text;
    fcb* debug = (fcb*)text;
    while(len > 0 ){
        if( BLOCKSIZE - off > len){
            memcpy(textPtr,buf + off, len);
            textPtr += len;
            off += len;
            openfilelist[fd].filePtr += len;
            len = 0;
        }
        else{
            memcpy(textPtr, buf + off, BLOCKSIZE - off);
            textPtr += BLOCKSIZE - off;
            off = 0;
            len -= BLOCKSIZE - off;
            blockNum = fatPtr->id;
            if(blockNum == END){
                cout << "len长度过长! 超出了文件大小!" << endl;
                break;
            }
            fatPtr = (fat*)(v_addr0 + BLOCKSIZE ) + blockNum;
            blockPtr = v_addr0 + BLOCKSIZE * blockNum;
            memcpy(buf,blockPtr,BLOCKSIZE);
        }
    }
    free(buf);
    return lenTmp - len;
}
int  my_read(int fd, int len){
    if(fd >= MAXOPENFILE || fd < 0){
        cout << "文件不存在" <<endl;
        return -1;
    }
    openfilelist[fd].filePtr = 0;
    char text[MAX_TEXT_SIZE] = "\0";
    if(len > openfilelist[fd].length)
    {
        printf("读取长度已经超过文件大小，默认读到文件末尾.\n");
        len = openfilelist[fd].length;
    }
    do_read(fd,len,text);
    cout << text<<endl ;
    return 1;
}
void my_exitsys(){
    while(currfd){
        my_close(currfd);
    }
    FILE *fp = fopen(FILENAME, "w");
    fwrite(v_addr0, SIZE, 1, fp);
    fclose(fp);
}
int  main(){
    char cmd[15][10] = {"mkdir", "rmdir", "ls", "cd", "create", "rm", "open", "close", "write", "read", "exit", "help"};
    char temp[30],command[30], *sp, *len, yesorno;
    int indexOfCmd, i;
	int length = 0;
    printf("\n\n************************ 文件系统 **************************************************\n");
    printf("************************************************************************************\n\n");
    startsys();
    printf("文件系统已开启.\n\n");
    printf("输入help来显示帮助页面.\n\n");
    while(1){
        printf("%s>", openfilelist[currfd].dir);
        fgets(temp,100,stdin);
        indexOfCmd = -1;
        for(int i=0;i<strlen(temp)-1;i++)
        {
             command[i]   = temp[i];     
             command[i+1] = '\0';
        }
        if (strcmp(command, "")){       
            sp = strtok(command, " ");  
            for (i = 0; i < 15; i++){
                if (strcmp(sp, cmd[i]) == 0){
                    indexOfCmd = i;
                    break;
                }
            }
            switch(indexOfCmd){
                case 0:         
                    sp = strtok(NULL, " ");
                    if (sp != NULL)
                        my_mkdir(sp);
                    else
                        error("mkdir");
                    break;
                case 1:         
                    sp = strtok(NULL, " ");
                    if (sp != NULL)
                        my_rmdir(sp);
                    else
                        error("rmdir");
                    break;
                case 2:         
                    my_ls();
                    break;
                case 3:         
                    sp = strtok(NULL, " ");
                    if (sp != NULL)
                        my_cd(sp);
                    else
                        error("cd");
                    break;
                case 4:         
                    sp = strtok(NULL, " ");
                    if (sp != NULL)
                        my_create(sp);
                    else
                        error("create");
                    break;
                case 5:         
                    sp = strtok(NULL, " ");
                    if (sp != NULL)
                        my_rm(sp);
                    else
                        error("rm");
                    break;
                case 6:         
                    sp = strtok(NULL, " ");
                    if (sp != NULL)
                        my_open(sp);
                    else
                        error("open");
                    break;
                case 7:         
                    if (openfilelist[currfd].metadata == 1)
                        my_close(currfd);
                    else
                        cout << "当前没有的打开的文件" << endl;
                    break;
                case 8:         
                    if (openfilelist[currfd].metadata == 1)
                        my_write(currfd);
                    else
                        cout << "请先打开文件,然后再使用wirte操作" <<endl;
                    break;
                case 9:         
                    sp = strtok(NULL, " ");
                    length = 0;
                    if (sp != NULL)  
                    {
                         for(int i=0;i<strlen(sp);i++)
                              length = length*10+sp[i]-'0';
                    }
                    if (length == 0)
                           error("read");
                    else if (openfilelist[currfd].metadata == 1)
                           my_read(currfd,length);    
                    else
                           cout << "请先打开文件,然后再使用read操作" <<endl;
                    break;
                case 10:        
                    my_exitsys();
                    printf("退出文件系统.\n");
                    return 0;
                    break;
                case 11:        
                    show_help();
                    break;
                default:
                    printf("没有 %s 这个命令\n", sp);
                    break;
            }
        }
        else
            printf("\n");
    }
    return 0;
}
