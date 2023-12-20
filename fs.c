#include "fs.h"
#include <stdio.h>
#include <string.h>

void saveFs(FileSystem* fs, FILE *stream) {
    fwrite(fs->disk.base, fs->disk.block_size * fs->disk.block_size, 1, stream);
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
        
        diskInit(&fs->disk, fs->super_block.block_num, fs->super_block.block_size);
        diskWriteBlock(&fs->disk, 0, buffer);

        for (int i = 1; i < fs->super_block.block_num; i++) {
            fread(buffer, fs->disk.block_size, 1, stream);
            diskWriteBlock(&fs->disk, i, buffer);
        }
        free(buffer);

        int offset = 1;
        // 读取inode bitmap
        fs->inode_bitmap = (ui32*)malloc(fs->disk.block_size * fs->super_block.inode_bitmap_block);
        for (int i = 0; i < fs->super_block.inode_bitmap_block; i++) {
            // 一块一块读取
            diskReadBlock(&fs->disk, offset, fs->inode_bitmap + i);
            offset++;
        }

        // 读取block bitmap   
        fs->block_bitmap = (ui32*)malloc(fs->disk.block_size * fs->super_block.block_bitmap_block);
        for (int i = 0; i < fs->super_block.block_bitmap_block; i++) {
            // 一块一块读取
            diskReadBlock(&fs->disk, offset, fs->block_bitmap + i);
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

const int size_Byte = 32;

//均为数据段的块号
int freeBlock(FileSystem* fs, ui16 block_num) {
    assert(block_num >= 0 && block_num < fs->super_block.data_block);
    int int_number = block_num >> 5;
    int bit_number = block_num & 0x1f;
    fs->block_bitmap[int_number] &= ~(1 << bit_number);
    return 1;
}

//均为数据段的块号
void occupyBlock(FileSystem* fs, ui16 block_num) {
    assert(block_num>=0&&block_num<fs->super_block.data_block);   
    int int_number = block_num >> 5;
    int bit_number = block_num & 0x1f;
    fs->block_bitmap[int_number]|=(1<<bit_number);
}

//均为数据段的块号
ui16 getFirstBlock(FileSystem* fs) {
    ui16 ret;
    int int_number,bit_number;
    for(int_number=0,ret = 0;ret < fs->super_block.data_block;int_number++){
        for(bit_number=0;bit_number<32;bit_number++){
            if(fs->block_bitmap[int_number] & (1 << bit_number)){
               ret++; 
            }
            else return ret;
        }
    }
    return (ui16)-1;
}

void freeInode(FileSystem* fs, ui16 inode_num) {
    assert(inode_num >= 0 && inode_num < fs->super_block.inode_num);   
    int int_number = inode_num >> 5;
    int bit_number = inode_num & 0x1f;
    fs->inode_bitmap[int_number] &= ~(1 << bit_number);
}

void occupyInode(FileSystem* fs, ui16 inode_num) {
    assert(inode_num >= 0 && inode_num < fs->super_block.inode_num);   
    int int_number = inode_num >> 5;
    int bit_number = inode_num & 0x1f;
    fs->inode_bitmap[int_number] |= (1 << bit_number);
}

ui16 getFirstInode(FileSystem* fs) {
    ui16 ret;
    int int_number,bit_number;
    for(int_number=0,ret=0;ret<fs->super_block.inode_num;int_number++){
        for(bit_number=0;bit_number<32;bit_number++){
            if(fs->inode_bitmap[int_number] & (1<<bit_number)){
               ret++; 
            }
            else return ret;
        }
    }
    return (ui16)-1;
}

INode readInode(FileSystem* fs, ui16 inode_num) {
    INode inode;
    int offset = 1 + fs->super_block.inode_bitmap_block + fs->super_block.block_bitmap_block;
    int begin = inode_num * fs->super_block.inode_size;
    int end = begin + fs->super_block.inode_size - 1;
    if (begin / fs->super_block.block_size != end / fs->super_block.block_size) {
        // 跨块
        int begin_block = begin / fs->super_block.block_size;
        int end_block = end / fs->super_block.block_size;
        int begin_first = begin % fs->super_block.block_size;
        int begin_len = fs->super_block.block_size - begin_first;
        int end_len = end % fs->super_block.block_size + 1;
        
        void* buffer = malloc(fs->super_block.block_size);
        
        diskReadBlock(&fs->disk, begin_block + offset, buffer);
        memcpy(&inode, buffer + begin_first, begin_len);
        diskReadBlock(&fs->disk, end_block + offset, buffer);
        memcpy(&inode + begin_len, buffer, end_len);

        free(buffer);
    } else {
        // 不跨块
        int block = begin / fs->super_block.block_size;
        int first = begin % fs->super_block.block_size;
        int len = end - begin + 1;
        void* buffer = malloc(fs->super_block.block_size);

        diskReadBlock(&fs->disk, block + offset, buffer);
        memcpy(&inode, buffer + first, len);

        free(buffer);
    }

    return inode;
}

void writeInode(FileSystem* fs, ui16 inode_num, INode* inode) {
    int offset = 1 + fs->super_block.inode_bitmap_block + fs->super_block.block_bitmap_block;
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

        diskReadBlock(&fs->disk, begin_block + offset, buffer);
        memcpy(buffer + begin_first, inode, begin_len);
        diskWriteBlock(&fs->disk, begin_block + offset, buffer);
        diskReadBlock(&fs->disk, end_block + offset, buffer);
        memcpy(buffer, inode + begin_len, end_len);
        diskWriteBlock(&fs->disk, end_block + offset, buffer);

        free(buffer);
    } else {
        // 不跨块
        int block = begin / fs->super_block.block_size;
        int first = begin % fs->super_block.block_size;
        int len = end - begin + 1;
        void* buffer = malloc(fs->super_block.block_size);

        diskReadBlock(&fs->disk, block + offset, buffer);
        memcpy(buffer + first, inode, len);
        diskWriteBlock(&fs->disk, block + offset, buffer);

        free(buffer);
    }
}

void dataReadBlock(FileSystem* fs, int data_block_num, void* buf){
    int offset = 1 + fs->super_block.inode_bitmap_block + fs->super_block.block_bitmap_block + fs->super_block.inode_table_block;
    diskReadBlock(&fs->disk, data_block_num + offset, buf);
}

void dataWriteBlock(FileSystem* fs, int data_block_num, void* buf){
    int offset = 1 + fs->super_block.inode_bitmap_block + fs->super_block.block_bitmap_block + fs->super_block.inode_table_block;
    diskReadBlock(&fs->disk, data_block_num + offset, buf);
}

ui16 createNewInode(FileSystem* fs, ui16 type) {
    INode res;
    ui16 inode_num = getFirstInode(fs);
    res.inode_number = inode_num;
    res.created_time = res.modified_time = res.access_time = time(NULL);
    res.link_count = 1;
    memset(res.direct_block, 0xff, sizeof(res.direct_block));
    res.first_inedxed_block = res.second_indexed_block = 0xffff;
    res.size = 0;
    res.type = 0;
    writeInode(fs, inode_num, &res);

    return res.inode_number;
}

int freeFirstIndex(FileSystem* fs, ui16 data_block, int allocated_num) {
    ui16* index = (ui16*)malloc(fs->super_block.block_size);
    dataReadBlock(fs, data_block, index);
    for (int i = 0; i < fs->super_block.block_size / sizeof(ui16) && allocated_num > 0; ++i) {
        allocated_num -= freeBlock(fs, index[i]);
    }
    free(index);
    freeBlock(fs, data_block);
    return allocated_num;
}

int freeSecondIndex(FileSystem* fs, ui16 data_block, int allocated_num) {
    ui16* index = (ui16*)malloc(fs->super_block.block_size);
    dataReadBlock(fs, data_block, index);
    for (int i = 0; i < fs->super_block.block_size / sizeof(int) && allocated_num; ++i) {
        allocated_num = freeFirstIndex(fs, index[i], allocated_num);
    }
    free(index);
    freeBlock(fs, data_block);
    return allocated_num;
}

int fsFree(FileSystem* fs, INode* inode) {
    int allocated_num = (inode->size + fs->super_block.block_size - 1) / fs->super_block.block_size;
    for (int i = 0; i < 10 && allocated_num > 0; ++i) {
        allocated_num -= freeBlock(fs, inode->direct_block[i]);
    }    

    if (allocated_num > 0) {
        allocated_num = freeFirstIndex(fs, inode->first_inedxed_block, allocated_num);
    }
    if (allocated_num > 0) {
        allocated_num = freeSecondIndex(fs, inode->second_indexed_block, allocated_num);
    }
    if (allocated_num > 0) {
        fprintf(stderr, "error: allocated_num > 0\n");
    }
    return !allocated_num;
}

void fsAllocate(FileSystem*fs, INode* inode, int size) {
    int old_block = inode->size / fs->super_block.block_size;   // 申请前的块数
    int new_size = size + inode->size;
    int end_block = new_size / fs->super_block.block_size;      // 申请完之后的块数

    int cur_block = old_block + 1;
    int needed_block = end_block - old_block;
    if (needed_block == 0) {
        inode->size = size;
        return;
    }

    // 申请块
    ui16* blocks = (ui16*)malloc(needed_block * sizeof(ui16));
    for (int i = 0; i < needed_block; ++i) {
        blocks[i] = getFirstBlock(fs);
        occupyBlock(fs, blocks[i]);
    }

    int base = 0;

    // 是否在10个块内
    if (old_block < 10) {
        for (int i = cur_block; i < 10 && cur_block <= end_block; ++i) {
            inode->direct_block[i] = blocks[i - cur_block];
            cur_block++;
        }
    }

    base += 10;

    // 是否在一级索引块内
    int index_pre_block = fs->super_block.block_size / sizeof(ui16);
    if (cur_block >= base && cur_block <= end_block) {
        ui16* first_index = (ui16*)malloc(fs->super_block.block_size);
        if (old_block >= base) {
            dataReadBlock(fs, inode->first_inedxed_block, first_index);
        } else {
            // 之前不在一级索引块内,需要创建
            inode->first_inedxed_block = getFirstBlock(fs);
            occupyBlock(fs, inode->first_inedxed_block);
            memset(first_index, 0xff, fs->super_block.block_size);
        }

        for (int i = cur_block - base; i < index_pre_block && cur_block <= end_block; ++i) {
            first_index[i] = blocks[cur_block - old_block];
            cur_block++;
        }

        dataWriteBlock(fs, inode->first_inedxed_block, first_index);
        free(first_index);
    }

    base += index_pre_block;

    // 是否在二级索引块内
    if (cur_block >= base && cur_block <= end_block) {
        ui16* second_index = (ui16*)malloc(fs->super_block.block_size);
        if (old_block >= base) {
            dataReadBlock(fs, inode->second_indexed_block, second_index);
        } else {
            // 之前不在二级索引块内,需要创建
            inode->second_indexed_block = getFirstBlock(fs);
            occupyBlock(fs, inode->second_indexed_block);
            memset(second_index, 0xff, fs->super_block.block_size);
        }

        for (int i = cur_block - base; i < index_pre_block && cur_block <= end_block; ++i) {
            ui16* first_index = (ui16*)malloc(fs->super_block.block_size);
            if (old_block >= base + i * index_pre_block) {
                dataReadBlock(fs, second_index[i], first_index);
            } else {
                // 之前不在一级索引块内,需要创建
                second_index[i] = getFirstBlock(fs);
                occupyBlock(fs, second_index[i]);
                memset(first_index, 0xff, fs->super_block.block_size);
            }

            for (int j = cur_block - base - i * index_pre_block; j < index_pre_block && cur_block <= end_block; ++j) {
                first_index[j] = blocks[cur_block - old_block];
                cur_block++;
            }

            dataWriteBlock(fs, second_index[i], first_index);
            free(first_index);
        }

        dataWriteBlock(fs, inode->second_indexed_block, second_index);
        free(second_index);
    }

    free(blocks);

}

void fsReAllocate(FileSystem* fs, INode* inode, int size) {
    fsFree(fs, inode);
    fsAllocate(fs, inode, size);
}

void fsWrite(FileSystem*fs, INode* inode, int offset, void* buffer, int size) {
    // 参考fsRead
    if (offset + size > inode->size) {
        fprintf(stderr, "error: index over flow\n");
        return;    
    }
    
    int begin = offset;
    int end = offset + size - 1;
    int begin_block = begin / fs->super_block.block_size;
    int end_block = end / fs->super_block.block_size;
    
}

void fsRead(FileSystem*fs, INode* inode, int offset, void* res, int size) {
    if (offset + size > inode->size) {
        fprintf(stderr, "error: index over flow\n");
        return;    
    }

    res = malloc(size);
    int res_ptr = 0;

    const int block_size = fs->super_block.block_size;
    void* buffer = malloc(block_size);

    int begin = offset;
    int end = offset + size - 1;
    int cur = begin;

    int begin_block = begin / block_size;
    int end_block = end / block_size;
    int cur_block = begin_block;

    int block_base = 0;
    
    
    if (cur_block < 10 && cur_block <= end_block) {
        // 在直接块内
        for (int i = cur_block; i < 10 && cur_block <= end_block; ++i) {
            dataReadBlock(fs, inode->direct_block[i], buffer);
            int begin = cur % block_size;
            int end = (cur_block == end_block) ? end % block_size : block_size - 1;
            memcpy(res + res_ptr, buffer + begin, end - begin + 1);
            res_ptr += end - begin + 1;
            cur_block++;
            cur += end - begin + 1;
        }
    }
    
    block_base += 10;

    const int index_pre_block = block_size / sizeof(ui16);

    if (cur_block >= block_base && cur_block <= end_block) {
        // 在一级索引块内
        ui16* first_index = (ui16*)malloc(block_size);
        dataReadBlock(fs, inode->first_inedxed_block, first_index);
        for (int i = cur_block - block_base; i < index_pre_block && cur_block <= end_block; ++i) {
            dataReadBlock(fs, first_index[i], buffer);
            int begin = cur % block_size;
            int end = (cur_block == end_block) ? end % block_size : block_size - 1;
            memcpy(res + res_ptr, buffer + begin, end - begin + 1);
            res_ptr += end - begin + 1;
            cur_block++;
            cur += end - begin + 1;
        }
        free(first_index);
    }

    block_base += index_pre_block;
    
    if (cur_block >= block_base && cur_block <= end_block) {
        ui16* second_index = (ui16*)malloc(fs->super_block.block_size);
        dataReadBlock(fs, inode->second_indexed_block, second_index);

        for (int i = cur_block - block_base; i < index_pre_block && cur_block <= end_block; ++i) {
            ui16* first_index = (ui16*)malloc(fs->super_block.block_size);
            dataReadBlock(fs, second_index[i], first_index);
            for (int j = cur_block - block_base - i * index_pre_block; j < index_pre_block && cur_block <= end_block; ++j) {
                dataReadBlock(fs, first_index[j], buffer);
                int begin = cur % block_size;
                int end = (cur_block == end_block) ? end % block_size : block_size - 1;
                memcpy(res + res_ptr, buffer + begin, end - begin + 1);
                res_ptr += end - begin + 1;
                cur_block++;
                cur += end - begin + 1;
            }
            dataWriteBlock(fs, second_index[i], first_index);
            free(first_index);
        }

        dataWriteBlock(fs, inode->second_indexed_block, second_index);
        free(second_index);
    }

}

// 把dentry翻译成流，然后调用fsWrite写入inode_id对应的inode
void saveDentry(FileSystem* fs, Dentry* dentr, ui16 inode_id) {
    size_t buffer_size = BLOCK_SIZE;
    
    char* beffer = malloc(buffer_size);
    if(beffer == NULL){
        fprintf(stderr,"Error:Can't find a memory to save!");
        return;
    }
    
    char* ptr = buffer;
    memcpy(ptr,&dentry->inode,sizeof(ui16));
    ptr += sizeof(ui16);
    memcpy(ptr,&dentry->father_inode,sizeof(ui16));
    ptr += sizeof(ui16);
    memcpy(ptr,&dentry->name_length,sizeof(ui16));
    ptr += sizeof(ui16);
    memset(ptr,)
    memcpy(ptr,&dentry->sub_dir_count,sizeof(ui16));
    ptr += sizeof(ui16);

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

    fs->current_dir_path = "/";
    
    fs->root_inode = getFirstInode(fs);
    fs->current_dir_inode = fs->root_inode = createNewInode(fs, 04777);
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
    fs->inode_bitmap = (ui32*)malloc(fs->disk.block_size * fs->super_block.inode_bitmap_block);

    // 初始化block bitmap 放在内存
    fs->block_bitmap = (ui32*)malloc(fs->disk.block_size * fs->super_block.block_bitmap_block);

    mkroot(fs, "/");
}

void rm(FileSystem* fs, char* path, int recursive){
    

}