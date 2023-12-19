#ifndef DIR_H
#define DIR_H

#include <time.h>
#include <stdio.h>
typedef struct iNode {
    unsigned short inodeNumber;         // 2B   inode的编号
    
    unsigned short length;              // 2B   文件长度
    unsigned short directBlock[10];     // 16B  直接块
    unsigned short firstIndirectBlock;  // 2B   一级间接块
    unsigned short secondIndirectBlock; // 2B   二级间接块 

    unsigned short type;                // 2B   文件类型 dir/file/link rwxrwxrwx

    unsigned short linkCount;           // 2B   链接数
        
    time_t createdTime;                 // 8B   创建时间
    time_t modifiedTime;                // 8B   修改时间
    time_t accessTime;                  // 8B   访问时间
}iNode;                                 // 52B


typedef struct dentry {
    unsigned short inode;           // 2B   inode的编号
    unsigned short fatherInode;     // 2B   父目录inode编号
    
    unsigned short nameLength;      // 2B   文件名长度
    char* name;                     // 8B   文件名

    unsigned short subDirCount;     // 2B   子目录数
    unsigned short* subDirLength;   // 8B   子目录名长度
    unsigned short* subDirInode;    // 8B   子目录inode编号
    char** subDir;                  // 8B   子目录名
} dentry;                           // 48B

// int main() {
// //     printf("sizeof(iNode) = %d\n", sizeof(iNode));
// //     printf("sizeof(dentry) = %d\n", sizeof(dentry));

// //     iNode test;
// //     test.inodeNumber = 1;
// //     test.length = 2;
// //     test.directBlock[0] = 3;
// //     test.firstIndirectBlock = 4;
// //     test.secondIndirectBlock = 5;
// //     test.type = 6;
// //     test.linkCount = 7;
// //     test.createdTime = 8;
// //     test.modifiedTime = 9;
// //     test.accessTime = 10;
// //     for (int i = 0; i < 10; i++) {
// //         test.directBlock[i] = i * 20 + 3;
// //     }

// //     FILE* fp = fopen("test", "wb");
// //     fwrite(&test, sizeof(iNode), 1, fp);
// //     fclose(fp);

//     iNode test2;
//     FILE* fp = fopen("test", "rb");
//     fread(&test2, sizeof(iNode), 1, fp);
//     fclose(fp);
//     printf("inodeNumber = %d\n", test2.inodeNumber);
//     printf("length = %d\n", test2.length);
//     printf("directBlock = ");
//     for (int i = 0; i < 10; i++) {
//         printf("%d ", test2.directBlock[i]);
//     }
//     printf("\n");
//     printf("firstIndirectBlock = %d\n", test2.firstIndirectBlock);
//     printf("secondIndirectBlock = %d\n", test2.secondIndirectBlock);
//     printf("type = %d\n", test2.type);
//     printf("linkCount = %d\n", test2.linkCount);
//     printf("createdTime = %d\n", test2.createdTime);
//     printf("modifiedTime = %d\n", test2.modifiedTime);
//     printf("accessTime = %d\n", test2.accessTime);
//     return 0;
// }
#endif