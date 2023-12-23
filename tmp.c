#include <stdio.h>
#include "fs.h"

int main() {
    // 创建并初始化文件系统
    printf("%d\n", sizeof(INode));
    FileSystem fs;
    UserOpenTable openTable;
    tbl_init(&openTable); // 初始化打开文件表
    
    // 格式化文件系统
    format(&fs);
    INode inode;
    inode = readInode(&fs, 0);
    printf("inodenum: %d\n", inode.inode_number);
    printf("inodetype: %o\n", inode.type);
    printf("size: %d\n", inode.size);
    printf("File system formatted.\n");
    printf("%d %d\n", fs.current_dir_inode, fs.current_dir_inode);
    ls(&fs, "/dir1");
    ls(&fs, "/");
    printf("fucking ls\n");

    mkdir(&fs, "./fuck1");
    ls(&fs, "/");
    mkdir(&fs, "fuck2");
    ls(&fs, "/");

    printf("cd !!!!!\n");
    cd(&fs, "fuck1");
    ls(&fs, NULL);
    cd(&fs, "/fuck2");
    ls(&fs, NULL);


    create(&fs, "cjd");
    create(&fs, "cjd2");
    ls(&fs, NULL);

    // 打开文件进行写操作
    open(&fs, &openTable, "cjd");
    char content[] = "Hello, World!askjefhgiouqwehfiuowqbfuykgavweuyifgiyuawe";
    write(&fs, &openTable, "cjd", sizeof(content), content, 0);
    printf("wirtted\n");

    char buf[1024];
    read(&fs, &openTable, "cjd", 50, buf);
    buf[50] = '\0';
    printf("Readded : %s\n", buf);

    // 关闭文件
    close(&fs, &openTable, "cjd");
    // exitfs(&fs, "filesystem");

    // // 清理打开文件表
    // tbl_destroy(&openTable);

    return 0;
}
