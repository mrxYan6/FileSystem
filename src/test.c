#include <stdio.h>
#include <time.h>
typedef unsigned short ui16;
typedef struct INode {
    ui16 inode_number;              // 2B   inode的编号
    ui16 size;                      // 2B   文件大小
    ui16 direct_block[10];          // 16B  直接块
    ui16 first_inedxed_block;       // 2B   一级索引块
    ui16 second_indexed_block;      // 2B   二级索引块 
    ui16 type;                      // 2B   文件属性 dir/file/link rwxrwxrwx
    ui16 link_count;                // 2B   链接数
    time_t created_time;            // 8B   创建时间
    time_t modified_time;           // 8B   修改时间
    time_t access_time;             // 8B   访问时间
} INode;                            // 56B

int main() {
    printf("%d\n", sizeof(INode));
}