#include "fs.h"
#include <stdio.h>
#include <string.h>

void saveFs(FileSystem* fs, FILE *stream) {
    fwrite(fs->disk.base, fs->disk.block_size * fs->disk.block_num, 1, stream);
}

void loadFs(FileSystem* fs, FILE *stream) {
    if (stream == NULL) {
        fprintf(stderr, "wrong, can't open the file\n");
        return;
    } else {
        char* buffer;

        buffer = malloc(BLOCK_SIZE);
        size_t read_cnt = fread(buffer, BLOCK_SIZE, 1, stream);
        if (memcmp(buffer, "ext233233", strlen("ext233233")) != 0) {
            fprintf(stderr, "Not a file system\n");
            free(buffer);
            return;
        }
        // 读取super block
        memcpy(&fs->super_block, buffer + strlen("ext233233"), sizeof(SuperBlock));
        
        fs->disk.block_size = fs->super_block.block_size;
        fs->disk.block_num = fs->super_block.block_num;
        fs->disk.base = malloc(fs->disk.block_size * fs->disk.block_num);
        memcpy(fs->disk.base, buffer, fs->disk.block_size);

        int offset = 1;
        // 读取inode bitmap
        fs->inode_bitmap = (int*)malloc(fs->disk.block_size * fs->super_block.inode_bitmap_block);
        for (int i = 0; i < fs->super_block.inode_bitmap_block; i++) {
            // 一块一块读取
            fread(buffer, fs->disk.block_size, 1, stream);
            memcpy(fs->inode_bitmap + i * fs->disk.block_size, buffer, fs->disk.block_size);
            memcpy(fs->disk.base + offset * fs->disk.block_size, buffer, fs->disk.block_size);
            offset++;
        }

        // 读取block bitmap   
        fs->block_bitmap = (int*)malloc(fs->disk.block_size * fs->super_block.block_bitmap_block);
        for (int i = 0; i < fs->super_block.block_bitmap_block; i++) {
            // 一块一块读取
            fread(buffer, fs->disk.block_size, 1, stream);
            memcpy(fs->block_bitmap + i * fs->disk.block_size, buffer, fs->disk.block_size);
            memcpy(fs->disk.base + offset * fs->disk.block_size, buffer, fs->disk.block_size);
            offset++;
        }

        free(buffer);
    }
}

void initSuperBlock(SuperBlock* sb) {
    sb->inode_bitmap_block = INODE_BITMAP_BLOCK;
    sb->block_bitmap_block = BLOCK_BITMAP_BLOCK;
    sb->inode_table_block = INODE_TABLE_BLOCK;
    sb->data_block = DATA_BLOCK;
    sb->inode_num = INODE_NUM;
    sb->block_num = BLOCK_NUM;
    sb->inode_size = INODE_SIZE;
    sb->block_size = BLOCK_SIZE;
}

const int size_Byte = sizeof(int) * 8;

void freeBlock(FileSystem* fs, unsigned short block_num) {
    assert(block_num >= 0 && block_num < fs->super_block.block_num);   
    int int_number = block_num / size_Byte;
    int bit_number = block_num % size_Byte;
    fs->block_bitmap[int_number] &= ~(1 << bit_number);
}

void occupyBlock(FileSystem* fs, unsigned short block_num) {
    assert(block_num>=0&&block_num<fs->super_block.block_num);   
    int int_number=block_num/size_Byte;
    int bit_number=block_num%size_Byte;
    fs->block_bitmap[int_number]|=(1<<bit_number);
}

unsigned short getFirstBlock(FileSystem* fs) {
    unsigned short ret;
    int int_number,bit_number;
    for(int_number=0,ret=0;ret<fs->super_block.block_num;int_number++){
        for(bit_number=0;bit_number<size_Byte;bit_number++){
            if(fs->block_bitmap[int_number] & (1<<bit_number)){
               ret++; 
            }
            else return ret;
        }
    }
    return (unsigned short)-1;
}

void freeInode(FileSystem* fs, unsigned short inode_num) {
    assert(inode_num >= 0 && inode_num < fs->super_block.inode_num);   
    int int_number = inode_num / size_Byte;
    int bit_number = inode_num % size_Byte;
    fs->inode_bitmap[int_number] &= ~(1 << bit_number);
}

void occupyInode(FileSystem* fs, unsigned short inode_num) {
    assert(inode_num >= 0 && inode_num < fs->super_block.inode_num);   
    int int_number = inode_num / size_Byte;
    int bit_number = inode_num % size_Byte;
    fs->inode_bitmap[int_number] |= (1 << bit_number);
}

unsigned short getFirstInode(FileSystem* fs) {
    unsigned short ret;
    int int_number,bit_number;
    for(int_number=0,ret=0;ret<fs->super_block.inode_num;int_number++){
        for(bit_number=0;bit_number<size_Byte;bit_number++){
            if(fs->inode_bitmap[int_number] & (1<<bit_number)){
               ret++; 
            }
            else return ret;
        }
    }
    return (unsigned short)-1;
}

void saveInode(FileSystem* fs, unsigned short inode_num, INode* inode) {
    occupyInode(fs, inode_num);
    int begin = inode_num * fs->super_block.inode_size;
    int end = begin + fs->super_block.inode_size - 1;
    if (begin / fs->super_block.block_size != end / fs->super_block.block_size) {
        // 跨块
        int begin_block = begin / fs->super_block.block_size;
        int end_block = end / fs->super_block.block_size;
        int begin_first = begin % fs->super_block.block_size;
        int end_first = 0;
        int begin_len = fs->super_block.block_size - begin_first - 1;
        int end_len = end + 1;
        
        void* buffer = malloc(fs->super_block.block_size);

        diskReadBlock(&fs->disk, begin_block, buffer);
        memcpy(buffer + begin_first, inode, begin_len);
        diskWriteBlock(&fs->disk, begin_block, buffer);
        diskReadBlock(&fs->disk, end_block, buffer);
        memcpy(buffer, inode + begin_len, end_len);

        free(buffer);
    } else {
        // 不跨块
        int block = begin / fs->super_block.block_size;
        int first = begin % fs->super_block.block_size;
        int len = end - begin + 1;
        void* buffer = malloc(fs->super_block.block_size);

        diskReadBlock(&fs->disk, block, buffer);
        memcpy(buffer + first, inode, len);
        diskWriteBlock(&fs->disk, block, buffer);

        free(buffer);
    }
}



void mkroot(FileSystem* fs, char* path) {
    Dentry root;
    root.inode = 0;
    root.name = "/";
    root.name_length = 1;
    root.father_inode = 0;
    root.sub_dir_count = 0;
    root.sub_dir = NULL;
    root.sub_dir_inode = NULL;
    root.sub_dir_length = NULL;
    
    fs->root_inode = getFirstInode(fs);
    INode cur_root_inode;
    cur_root_inode.inode_number = fs->root_inode;
    // cur_root_inode.type = 
    cur_root_inode.link_count = 1;
    cur_root_inode.created_time = cur_root_inode.modified_time = cur_root_inode.access_time = time(NULL);
    saveInode(fs, fs->root_inode, &cur_root_inode);    
}

void format(FileSystem* fs) {
    // 如果fs有内容，清空fs
    if (fs->disk.base!=NULL) {
        free(fs->disk.base);
    }

    // 初始化disk
    diskInit(&fs->disk, BLOCK_NUM, BLOCK_SIZE);

    //初始化boot 1块
    int offset = 0;
    char identifier[] = "ext233233";
    memcpy(fs->disk.base, identifier, strlen(identifier));
    offset += strlen(identifier);
    initSuperBlock(&fs->super_block);
    memcpy(fs->disk.base + offset, &fs->super_block, sizeof(SuperBlock));
    offset = fs->disk.block_size;
    
    // 初始化inode bitmap 放在内存
    fs->inode_bitmap = (int*)malloc(fs->disk.block_size * fs->super_block.inode_bitmap_block);

    // 初始化block bitmap 放在内存
    fs->block_bitmap = (int*)malloc(fs->disk.block_size * fs->super_block.block_bitmap_block);

    mkroot(fs, "/");
}