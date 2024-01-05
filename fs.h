#ifndef FS_H
#define FS_H

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include "disk.h"
#include "config.h"

// 1 block -> 1024B
// | super block | inode bitmap | block bitmap | inode table  | data block |
// | 1 block     | 1 block      | 1 block      | 56 block     | 965 block  |
typedef unsigned int ui32;
typedef unsigned short ui16;


typedef struct User {
    ui16 user_id;                   // 用户id
    ui16 group_id;                  // 用户所属组id
    char user_name[10];             // 用户名
    char password[16];              // 密码
} User;                          

typedef struct UserList {
    User* users;                    // 用户列表
    int size;                       // 用户数
    int capacity;                   // 用户列表容量
} UserList;

void ul_push_back(UserList* ul, User user);

void ul_pop_back(UserList* ul);

void ul_init(UserList* ul);

void ul_destroy(UserList* ul);

void ul_resize(UserList* ul, int new_size);

void ul_clear(UserList* ul);

void ul_remove(UserList* ul, int index);

typedef struct Group {
    ui16 group_id;                  // 组id
    char group_name[10];            // 组名
    ui16 user_count;                // 组内用户数
    ui16 user_id[20];               // 组内用户id
} Group;

typedef struct GroupList {
    Group* groups;                  // 组列表
    int size;                       // 组数
    int capacity;                   // 组列表容量
} GroupList;

void gl_push_back(GroupList* gl, Group group);

void gl_pop_back(GroupList* gl);

void gl_init(GroupList* gl);

void gl_destroy(GroupList* gl);

void gl_resize(GroupList* gl, int new_size);

void gl_clear(GroupList* gl);

void gl_remove(GroupList* gl, int index);

typedef struct SuperBlock {
    ui16 inode_bitmap_block;        // 2B   inode bitmap block number
    ui16 block_bitmap_block;        // 2B   block bitmap block number
    ui16 inode_table_block;         // 2B   inode table block number
    ui16 data_block;                // 2B   data block number
    ui16 inode_num;                 // 2B   inode number
    ui16 block_num;                 // 2B   block number
    ui16 inode_size;                // 2B   inode size
    ui16 block_size;                // 2B   block size
    UserList user_list;             // 所有用户列表
    GroupList group_list;           // 所有组列表

} SuperBlock;

void initSuperBlock(SuperBlock* sb);

typedef struct INode {
    ui16 inode_number;              // 2B   inode的编号
    ui16 size;                      // 2B   文件大小
    ui16 direct_block[10];          // 16B  直接块
    ui16 first_inedxed_block;       // 2B   一级索引块
    ui16 second_indexed_block;      // 2B   二级索引块 
    ui16 type;                      // 2B   文件属性 dir/file/link rwxrwxrwx
    ui16 link_count;                // 2B   链接数
    ui16 user_id;                   // 2B   属于的用户id
    ui16 read_lock;                 // 2B   读锁数
    ui16 write_lock;                // 2B   写锁数
    time_t created_time;            // 8B   创建时间
    time_t modified_time;           // 8B   修改时间
    time_t access_time;             // 8B   访问时间
} INode;                            // 56B


typedef unsigned int* InodeBitmap;
typedef unsigned int* BlockBitmap;

typedef struct Dentry {
    ui16 inode;                     // 2B   inode的编号
    ui16 father_inode;              // 2B   父目录inode编号
    ui16 name_length;               // 2B   文件名长度
    char* name;                     // 8B   文件名
    ui16 sub_dir_count;             // 2B   子目录数
    ui16* sub_dir_inode;            // 8B   子目录inode编号
    ui16* sub_dir_length;           // 8B   子目录名长度
    char** sub_dir;                 // 8B   子目录名
} Dentry;                           // 48B

typedef struct FileSystem {
    Disk disk;                      // 磁盘
    SuperBlock super_block;         // 超级块
    InodeBitmap inode_bitmap;       // inode的位图
    BlockBitmap block_bitmap;       // block的位图
    ui16 root_inode;                // 根目录inode编号
    ui16 current_dir_inode;         // 当前目录inode编号
    char* current_dir_path;         // 当前目录路径
    
    ui16 current_user_id;           // 当前用户id
    UserList user_list;             // 所有用户列表
    GroupList group_list;           // 所有组列表

} FileSystem;

typedef struct UserOpenItem {
    INode inode;                          // 修改之后的文件inode, 用于保存变更, 也可用于判断是否打开

    int offset;                           // 文件内读写偏移量
    bool modify;                          // inode是否被修改
} UserOpenItem;

typedef struct UserOpenTable {
    UserOpenItem* items;                  // 用户打开文件表
    int size;                             // 用户打开文件数
    int capacity;                         // 用户打开文件表容量
} UserOpenTable;

void tbl_push_back(UserOpenTable* tb, UserOpenItem item);

void tbl_pop_back(UserOpenTable* tb);

void tbl_init(UserOpenTable* tb);

void tbl_destroy(UserOpenTable* tb);

void tbl_resize(UserOpenTable* tb, int new_size);

void tbl_clear(UserOpenTable* tb);

void tbl_remove(UserOpenTable* tb, int index);

INode readInode(FileSystem* fs, ui16 inode_num) ;

// file system save to file
void saveFs(FileSystem* fs, FILE *stream);

// file system load from file
void loadFs(FileSystem* fs, FILE *stream);

// api

// my_format
void format(FileSystem* fs);

// my_mkdir
void mkdir(FileSystem* fs, char* path);

// my_rm + my_rmdir
// recursive = 0: rm file or empty dir; recursive = 1: rm dir and all its subdirs
void rm(FileSystem* fs, char* path, int recursive); 

void saveDentry(FileSystem* fs, Dentry* dentry);

// my_ls
void ls(FileSystem* fs, char* path);

// my_cd
void cd(FileSystem* fs, char* path);

// my_create
void create(FileSystem* fs, char* path);

// my_open
void open(FileSystem* fs, UserOpenTable* tb, char* path);

// my_close
void close(FileSystem* fs, UserOpenTable* tb, char* path);

// my_read
int read(FileSystem* fs,UserOpenTable* tb, char* path, int length, void* content);

// my_write
void write(FileSystem* fs, UserOpenTable* tb, char* path, int length, char* content, int opt);

// exit fs
void exitfs(FileSystem* fs, UserOpenTable* tb, FILE* stream);

bool checkPermission(FileSystem* fs, INode* inode, int opt);

#endif