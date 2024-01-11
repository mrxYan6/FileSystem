#include "fs.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void pirintInodeMap(FileSystem* fs) {
    printf("INODE BIT MAP : \n");
    for (int i = 0; i < 10; ++i) {
        printf("%d ", fs->inode_bitmap[i]);
    }
    printf("\n");
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

// const int size_Byte = 32;
void writeBlockBitMap(FileSystem* fs, ui16 num) {
    int offset_block = num / 8 / fs->super_block.block_size;
    int offset_int = offset_block * fs->super_block.block_size / 4;
    // printf("writeBlockBitMap %d %d %d\n", num, offset_block, offset_int);
    diskWriteBlock(&fs->disk, 1 + fs->super_block.inode_bitmap_block + offset_block, fs->block_bitmap + offset_int);
}

//释放文件系统中指定块号的数据块
int freeBlock(FileSystem* fs, ui16 block_num) {
    assert(block_num >= 0 && block_num < fs->super_block.data_block);
    int int_number = block_num >> 5;
    int bit_number = block_num & 0x1f;
    fs->block_bitmap[int_number] &= ~(1 << bit_number);
    writeBlockBitMap(fs, block_num);
    return 1;
}

//占用文件系统中指定块号的数据块，检查块号的合法性，然后计算出在位图中的位置，将相应位置的位设置为 1
void occupyBlock(FileSystem* fs, ui16 block_num) {
    assert(block_num>=0&&block_num<fs->super_block.data_block);   
    int int_number = block_num >> 5;
    int bit_number = block_num & 0x1f;
    fs->block_bitmap[int_number]|=(1<<bit_number);
    writeBlockBitMap(fs, block_num);
}

//均用于获取文件系统中第一个空闲的数据块号
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

void writeInodeBitMap(FileSystem* fs, ui16 num) {
    int offset_block = num / 8 / fs->super_block.block_size;
    int offset_int = offset_block * fs->super_block.block_size / 4;
    // printf("writeInodeBitMap %d %d %d\n", num, offset_block, offset_int);
    diskWriteBlock(&fs->disk, 1 + offset_block, fs->inode_bitmap + offset_int);
}

//释放文件系统中指定的 inode
void freeInode(FileSystem* fs, ui16 inode_num) {
    assert(inode_num >= 0 && inode_num < fs->super_block.inode_num);   
    int int_number = inode_num >> 5;
    int bit_number = inode_num & 0x1f;
    fs->inode_bitmap[int_number] &= ~(1 << bit_number);
    writeInodeBitMap(fs, inode_num);
}

//占用文件系统中指定的 inode,检查 inode 号的合法性，然后计算出在位图中的位置，将相应位置的位设置为 1
void occupyInode(FileSystem* fs, ui16 inode_num) {
    assert(inode_num >= 0 && inode_num < fs->super_block.inode_num);   
    int int_number = inode_num >> 5;
    int bit_number = inode_num & 0x1f;
    fs->inode_bitmap[int_number] |= (1 << bit_number);
    writeInodeBitMap(fs, inode_num);
}

bool testInode(FileSystem* fs, ui16 inode_num) {
    assert(inode_num >= 0 && inode_num < fs->super_block.inode_num);   
    int int_number = inode_num >> 5;
    int bit_number = inode_num & 0x1f;
    return fs->inode_bitmap[int_number] & (1 << bit_number);
}

//用于获取文件系统中第一个空闲的 inode 号
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


//从文件系统中读取指定 inode 号的 inode 数据，跨块则分别读取两个块的数据，然后合并到 inode 结构体中,如果不跨块，则直接读取相应块的数据
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

//将指定 inode 号的 inode 数据写入到文件系统中，跨块同上
void writeInode(FileSystem* fs, ui16 inode_num, INode* inode) {
    int offset = 1 + fs->super_block.inode_bitmap_block + fs->super_block.block_bitmap_block;
    // printf("wt inode %d\n", inode_num);
    occupyInode(fs, inode_num);
    int begin = inode_num * fs->super_block.inode_size;
    int end = begin + fs->super_block.inode_size - 1;
    if (begin / fs->super_block.block_size != end / fs->super_block.block_size) {
        // 跨块
        // printf("kk %d\n");
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
        // printf("bkk \n");
        // 不跨块
        int block = begin / fs->super_block.block_size;
        int first = begin % fs->super_block.block_size;
        int len = end - begin + 1;
        // printf("bkk %d %d %d suppose sz %d\n", block, first, len, sizeof(INode));
        void* buffer = malloc(fs->super_block.block_size);

        diskReadBlock(&fs->disk, block + offset, buffer);
        memcpy(buffer + first, inode, len);
        diskWriteBlock(&fs->disk, block + offset, buffer);

        free(buffer);
    }
    
}

//从文件系统中读取数据块号为 data_block_num 的数据块到缓冲区 buf 中
void dataReadBlock(FileSystem* fs, int data_block_num, void* buf){
    int offset = 1 + fs->super_block.inode_bitmap_block + fs->super_block.block_bitmap_block + fs->super_block.inode_table_block;
    diskReadBlock(&fs->disk, data_block_num + offset, buf);
}

//将缓冲区 buf 中的数据写入文件系统中数据块号为 data_block_num 的数据块
void dataWriteBlock(FileSystem* fs, int data_block_num, void* buf){
    int offset = 1 + fs->super_block.inode_bitmap_block + fs->super_block.block_bitmap_block + fs->super_block.inode_table_block;
    diskWriteBlock(&fs->disk, data_block_num + offset, buf);
}

//创建一个新的 inode 并返回其编号
ui16 createNewInode(FileSystem* fs, ui16 type) {
    // pirintInodeMap(fs);
    INode res;
    ui16 inode_num = getFirstInode(fs);
    res.inode_number = inode_num;
    res.created_time = res.modified_time = res.access_time = time(NULL);
    res.link_count = 1;
    memset(res.direct_block, 0xff, sizeof(res.direct_block));
    res.direct_block[0] = getFirstBlock(fs);//先分配一个内存
    occupyBlock(fs, res.direct_block[0]);
    res.first_inedxed_block = res.second_indexed_block = 0xffff;
    res.size = 0;
    res.type = type;

    // printf("INODE INFO: \n");
    // printf("id:%d\n",res.inode_number);
    // printf("ty:%o\n", res.type);    
    // printf("sz:%d\n", res.size);    
    // printf("lc:%d\n", res.link_count);
    // printf("fb:%d\n", res.direct_block[0]);

    writeInode(fs, inode_num, &res);
    INode test = readInode(fs, inode_num);
    // printf("INODE INFO - test: \n");
    // printf("id:%d\n", test.inode_number);
    // printf("ty:%o\n", test.type);    
    // printf("sz:%d\n", test.size);    
    // printf("lc:%d\n", test.link_count);
    // printf("fb:%d\n", test.direct_block[0]);
    return res.inode_number;
}

//释放一级索引块及其指向的数据块
int freeFirstIndex(FileSystem* fs, ui16 data_block, int allocated_num) {
    ui16* index = (ui16*)malloc(fs->super_block.block_size);
    dataReadBlock(fs, data_block, index);
    for (int i = 0; i < fs->super_block.block_size / sizeof(ui16) && allocated_num > 0; ++i) {
        allocated_num -= freeBlock(fs, index[i]);//-1
    }
    free(index);
    freeBlock(fs, data_block);
    return allocated_num;
}

//释放二级索引块及其指向的数据块及其指向的一级索引块和数据块
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

//释放一个 inode 占用的所有数据块
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
        // fprintf(stderr, "error: allocated_num > 0\n");
    }
    return !allocated_num;
}

//为一个 inode 分配数据块以满足指定的大小
void fsAllocate(FileSystem*fs, INode* inode, int size) {
    int old_block = inode->size / fs->super_block.block_size;   // 申请前的块数
    int new_size = size + inode->size;
    int end_block = new_size / fs->super_block.block_size;      // 申请完之后的块数
    inode->size = new_size;
    int cur_block = old_block + 1;
    int needed_block = end_block - old_block;
    if (needed_block == 0) {
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
    // printf("fsAllocate ing: %d %d\n", inode->inode_number, size);
    // printf("fsAllocated: %d %d blocks\n", inode->inode_number, cur_block - old_block);
}

//重新分配 inode 占用的数据块
void fsReAllocate(FileSystem* fs, INode* inode, int size) {
    fsFree(fs, inode);
    fsAllocate(fs, inode, size);
}

//从文件系统中读取指定 inode 的数据
int fsRead(FileSystem*fs, INode* inode, int offset, void* res, int size) {
    if (offset + size > inode->size) {
        // fprintf(stderr, "error: index over flow\n");
        return;    
    }
    // printf("fsRead ing: %d %d %d\n",inode->inode_number, offset, size);
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
            int l = cur % block_size;
            int r = (cur_block == end_block) ? end % block_size : block_size - 1;
            // printf("Reading direct block %d %dto%d\n", inode->direct_block[i],l, r);
            memcpy(res + res_ptr, buffer + l, r - l + 1);
            res_ptr += r - l + 1;
            cur_block++;
            cur += r - l + 1;
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
            int l = cur % block_size;
            int r = (cur_block == end_block) ? end % block_size : block_size - 1;
            // printf("Reading direct block %d %dto%d\n", inode->direct_block[i],l, r);
            memcpy(res + res_ptr, buffer + l, r - l + 1);
            res_ptr += r - l + 1;
            cur_block++;
            cur += r - l + 1;
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
                int l = cur % block_size;
                int r = (cur_block == end_block) ? end % block_size : block_size - 1;
                // printf("Reading direct block %d %d to %d\n", inode->direct_block[i],l, r);
                // printf
                memcpy(res + res_ptr, buffer + l, r - l + 1);
                res_ptr += r - l + 1;
                cur_block++;
                cur += r - l + 1;
            }
            dataWriteBlock(fs, second_index[i], first_index);
            free(first_index);
        }

        dataWriteBlock(fs, inode->second_indexed_block, second_index);
        free(second_index);
    }
    // printf("OKKKK\n");
    return cur - begin;
}

//向文件系统中写入指定 inode 的数据
void fsWrite(FileSystem*fs, INode* inode, int offset, void* context, int size) {
    // 参考fsRead
    // printf("fsWrite ing: %d %d %d\n", inode->inode_number, offset, size);
    if (offset + size > inode->size) {
        fsAllocate(fs, inode, offset + size - inode->size);
    }
    // printf("fsWrite ing cur size: %d %d \n", inode->inode_number, inode->size);
    // printf("fsWrite ing: %d %d %d\n", inode->inode_number, offset, size);
    int context_ptr = 0;

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
            int l = cur % block_size;
            int r = (cur_block == end_block) ? end % block_size : block_size - 1;
            // printf("Writing direct block %d %dto%d\n", inode->direct_block[i],l, r);
            memcpy(buffer + l, context + context_ptr, r - l + 1);
            dataWriteBlock(fs, inode->direct_block[i], buffer);
            context_ptr += r - l + 1;
            cur_block++;
            cur += r - l + 1;
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
            int l = cur % block_size;
            int r = (cur_block == end_block) ? end % block_size : block_size - 1;
            // printf("Writing block %d %dto%d\n", inode->direct_block[i],l, r);
            memcpy(buffer + l, context + context_ptr, r - l + 1);
            dataWriteBlock(fs, first_index[i], buffer);
            context_ptr += r - l + 1;
            cur_block++;
            cur += r - l + 1;
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
                int l = cur % block_size;
                int r = (cur_block == end_block) ? end % block_size : block_size - 1;
                // printf("Writing block %d %dto%d\n", inode->direct_block[i],l, r);
                memcpy(buffer + l, context + context_ptr, r - l + 1);
                dataWriteBlock(fs, first_index[j], buffer);
                context_ptr += r - l + 1;
                cur_block++;
                cur += r - l + 1;
            }
            dataWriteBlock(fs, second_index[i], first_index);
            free(first_index);
        }

        dataWriteBlock(fs, inode->second_indexed_block, second_index);
        free(second_index);
    }
    // printf("FUCKKKKKK\n");

    free(buffer);
}

// 从 src 复制 size 字节的数据到 *dst，并更新 *dst 指向下一个位置，处理数据的序列化
void receiveAndShift(void** dst, void* src, int size) {
    memcpy(*dst, src, size);
    *dst += size;
}

// 将 *src 指向的数据复制到 dst，然后更新 *src 指向下一个位置，处理数据的反序列化
void sendAndShift(void* dst, void** src, int size) {
    memcpy(dst, *src, size);
    *src += size;
}

//根据给定的 inode ID 从文件系统中读取目录条目
Dentry readDentry(FileSystem* fs, ui16 inode_id) {
    Dentry res;
    // printf("readDentry ing: %d\n", inode_id);
    INode inode = readInode(fs, inode_id);
    // printf("%o\n",inode.type);
    if (((inode.type >> 9) & 7) != 4) {
        // fprintf(stderr, "error: not a directory\n");
        return res;
    }
    int offset = 0;
    int size = inode.size;
    // printf("sz: %d\n", size);
    void* buffer = malloc(size);
    // printf("!!U(*Y&#*($*()))\n");
    void* ptr = buffer;
    // 从文件系统中读取数据到 buffer
    fsRead(fs, &inode, offset, buffer, size);
    // 从 buffer 中读取数据到 res
    // printf("readDentry buffer: %d\n", size);
    // for(int i=0;i<size;i++){
    //     // printf("%x ", ((char*)buffer)[i]);
    // }
    // printf("\n");
    sendAndShift(&res.inode, &ptr, sizeof(ui16));
    // printf("dentry inode: %d\n", res.inode);
    sendAndShift(&res.father_inode, &ptr, sizeof(ui16));
    // printf("dentry fatherinode: %d\n", res.father_inode);
    sendAndShift(&res.name_length, &ptr, sizeof(ui16));
    // printf("dentry namelength: %d\n", res.name_length);
    res.name = malloc(res.name_length);
    sendAndShift(res.name, &ptr, res.name_length);
    // printf("dentry name: %s\n", res.name);
    sendAndShift(&res.sub_dir_count, &ptr, sizeof(ui16));
    // printf("dentry sub_dir_count: %d\n", res.sub_dir_count);

    res.sub_dir_inode = malloc(res.sub_dir_count * sizeof(ui16));
    res.sub_dir_length = malloc(res.sub_dir_count * sizeof(ui16));
    res.sub_dir = malloc(res.sub_dir_count * sizeof(char*));

    for (int i = 0; i < res.sub_dir_count; ++i) {
        sendAndShift(res.sub_dir_inode + i, &ptr, sizeof(ui16));
        sendAndShift(res.sub_dir_length + i, &ptr, sizeof(ui16));
        // printf("receiving : %d %d\n", res.sub_dir_inode[i], res.sub_dir_length[i]);
        res.sub_dir[i] = malloc(res.sub_dir_length[i]);
        sendAndShift(res.sub_dir[i], &ptr, res.sub_dir_length[i]);
    }
    return res;
}

// 把dentry翻译成流，然后调用fsWrite写入inode_id对应的inode，将给定的 Dentry 结构序列化，并将其保存到对应的 inode 中
void saveDentry(FileSystem* fs, Dentry* dentry) {
    // 计算需要的缓冲区大小
    size_t buffer_size = sizeof(ui16) * 4 + dentry->name_length + 2 * dentry->sub_dir_count * sizeof(ui16);
    for(int i= 0; i<dentry->sub_dir_count; i++){
        buffer_size += dentry->sub_dir_length[i];
    }
    // 分配缓冲区
    void* buffer = malloc(buffer_size);
    void* ptr = buffer;
    // 序列化数据到缓冲区
    receiveAndShift(&ptr, &dentry->inode, sizeof(ui16));
    receiveAndShift(&ptr, &dentry->father_inode, sizeof(ui16));
    receiveAndShift(&ptr, &dentry->name_length, sizeof(ui16));
    receiveAndShift(&ptr, dentry->name, dentry->name_length);
    receiveAndShift(&ptr, &dentry->sub_dir_count, sizeof(ui16));
    for(int i = 0; i < dentry->sub_dir_count; i++){
        // printf("translating : %d %d %s\n", dentry->sub_dir_inode[i], dentry->sub_dir_length[i], dentry->sub_dir[i]);
        receiveAndShift(&ptr, dentry->sub_dir_inode + i , sizeof(ui16));
        receiveAndShift(&ptr, dentry->sub_dir_length + i , sizeof(ui16));
        receiveAndShift(&ptr, dentry->sub_dir[i], dentry->sub_dir_length[i]);
    }
    // printf("saveing dentry: %d %d\n", buffer_size, dentry->inode);
    // for (int i = 0; i < ptr - buffer; ++i) {
    //     // printf("%x ", ((char*)buffer)[i]);
    // }
    printf("\n");
    // 读取对应的inode，将缓冲区写入文件系统
    INode inode = readInode(fs, dentry->inode);
    fsWrite(fs, &inode, 0, buffer, buffer_size);
    writeInode(fs, inode.inode_number, &inode);
    
    free(buffer);
}

//释放 Dentry 结构中动态分配的内存
void freeDentry(Dentry* dir) {
    free(dir->name);
    for (int i = 0; i < dir->sub_dir_count; ++i) {
        free(dir->sub_dir[i]);
    }
    free(dir->sub_dir);
    free(dir->sub_dir_inode);
    free(dir->sub_dir_length);
}

//将一个子目录添加到父目录中
void dentryAddSon(FileSystem* fs, ui16 father_id, ui16 inode_id, char* name, ui16 name_length) {
    Dentry father = readDentry(fs, father_id);
    father.sub_dir_inode = realloc(father.sub_dir_inode, (father.sub_dir_count + 1) * sizeof(ui16));
    father.sub_dir_length = realloc(father.sub_dir_length, (father.sub_dir_count + 1) * sizeof(ui16));
    father.sub_dir = realloc(father.sub_dir, (father.sub_dir_count + 1) * sizeof(char*));
    father.sub_dir_inode[father.sub_dir_count] = inode_id;
    father.sub_dir_length[father.sub_dir_count] = name_length + 1;
    father.sub_dir[father.sub_dir_count] = malloc(name_length + 1);
    memcpy(father.sub_dir[father.sub_dir_count], name, name_length + 1);
    father.sub_dir_count++;
    saveDentry(fs, &father);
    freeDentry(&father);
}

//创建新的 Dentry，为当前目录（.）和父目录（..）初始化信息
ui16 createDentry(FileSystem* fs, ui16 father_inode_id, char* name, ui16 name_length) {
    Dentry res;
    // 创建新的inode
    res.inode = createNewInode(fs, 04777);
    res.father_inode = father_inode_id;
    res.name_length = name_length + 1;
    res.name = malloc(name_length);
    memcpy(res.name, name, name_length + 1);

    // 初始化子目录信息
    res.sub_dir_count = 2;
    res.sub_dir_inode = malloc(2 * sizeof(ui16));
    res.sub_dir_length = malloc(2 * sizeof(ui16));
    res.sub_dir = malloc(2 * sizeof(char*));

    // 初始化当前目录（.）的信息
    res.sub_dir = malloc(2 * sizeof(char*));
    res.sub_dir_inode[0] = res.inode;
    res.sub_dir_length[0] = 2;
    res.sub_dir[0] = malloc(res.sub_dir_length[0]);
    memcpy(res.sub_dir[0], ".", 2);
    res.sub_dir[0][1] = 0;

    // 初始化父目录（..）的信息
    res.sub_dir_inode[1] = father_inode_id;
    res.sub_dir_length[1] = 3;
    res.sub_dir[1] = malloc(res.sub_dir_length[1]);
    memcpy(res.sub_dir[1], "..", 3);
    res.sub_dir[1][2] = 0;

    if (res.inode == 0) {
        // 根目录
        res.father_inode = res.inode;
    } else {
        dentryAddSon(fs, father_inode_id, res.inode, name, name_length + 1);
    }

    // printf("createDentry ing: %d %d %d\n", res.inode, res.father_inode, res.name_length);
    // printf("createDentry ing: %d %d\n", res.sub_dir_count, res.sub_dir_inode[0]);
    // printf("createDentry ing: %d %d\n", res.sub_dir_length[0], res.sub_dir_length[1]);
    // printf("createDentry ing: %s %s\n", res.sub_dir[0], res.sub_dir[1]);

    // 将Dentry保存到文件系统
    saveDentry(fs, &res);
    
    return res.inode;
}

void dentryDeleteSon(FileSystem* fs, ui16 father_id, ui16 inode_id) {
    Dentry father = readDentry(fs, father_id);
    int index = -1;
    for (int i = 0; i < father.sub_dir_count; ++i) {
        if (father.sub_dir_inode[i] == inode_id) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        // fprintf(stderr, "error: no such son\n");
        return;
    }

    int dir_count = father.sub_dir_count;

    ui16* new_inode = malloc((dir_count - 1) * sizeof(ui16));
    ui16* new_sub_dir_length = malloc((dir_count - 1) * sizeof(ui16));
    char** new_sub_dir = malloc((dir_count - 1) * sizeof(char*));
    
    memcpy(new_inode, father.sub_dir_inode, index * sizeof(ui16));
    memcpy(new_inode + index, father.sub_dir_inode + index + 1, (dir_count - index - 1) * sizeof(ui16));
    memcpy(new_sub_dir_length, father.sub_dir_length, index * sizeof(ui16));
    memcpy(new_sub_dir_length + index, father.sub_dir_length + index + 1, (dir_count - index - 1) * sizeof(ui16));
    memcpy(new_sub_dir, father.sub_dir, index * sizeof(char*));
    memcpy(new_sub_dir + index, father.sub_dir + index + 1, (dir_count - index - 1) * sizeof(char*));

    free(father.sub_dir_inode);
    free(father.sub_dir_length);
    free(father.sub_dir[index]);
    free(father.sub_dir);

    father.sub_dir_inode = new_inode;
    father.sub_dir_length = new_sub_dir_length;
    father.sub_dir = new_sub_dir;
    father.sub_dir_count--;
    
    saveDentry(fs, &father);
    freeDentry(&father);
}

void deleteDnetry(FileSystem* fs, ui16 inode_id) {
    Dentry dentry = readDentry(fs, inode_id);
    if (dentry.sub_dir_count > 2) {
        // fprintf(stderr, "error: directory is not empty\n");
        return;
    }
    dentryDeleteSon(fs, dentry.father_inode, inode_id);
    freeDentry(&dentry);
    INode inode = readInode(fs, inode_id);
    fsFree(fs, &inode);
    freeInode(fs, inode_id);
}

void format(FileSystem* fs) {
    diskInit(&fs->disk, BLOCK_NUM, BLOCK_SIZE);
    //初始化boot 1块
    int offset = 0;
    char identifier[] = "ext233233";
    memcpy(fs->disk.base, identifier, sizeof(identifier));
    offset += sizeof(identifier);
    // printf("size : %d\n", offset);
    initSuperBlock(&fs->super_block);
    memcpy(fs->disk.base + offset, &fs->super_block, sizeof(SuperBlock));
    offset = fs->disk.block_size;
    
    // 初始化inode bitmap 放在内存
    fs->inode_bitmap = (int*)malloc(fs->disk.block_size * fs->super_block.inode_bitmap_block);

    // 初始化block bitmap 放在内存
    fs->block_bitmap = (int*)malloc(fs->disk.block_size * fs->super_block.block_bitmap_block);

    fs->root_inode = createDentry(fs, 0, "/", 1);
    fs->current_dir_path = malloc(2);
    strcpy(fs->current_dir_path, "/");
    // printf("first:%d", fs->root_inode);
    fs->current_dir_inode = fs->root_inode;
}

//无斜杠分割的路径
bool cd_(FileSystem* fs, ui16* cur_id , char* path) {
    ui16 cur_dir_id = *cur_id;
    INode cur_inode = readInode(fs, cur_dir_id);
    if (((cur_inode.type >> 9) & 7) != 4) {
        // fprintf(stderr, "error: not a directory\n");
        return false;
    }

    Dentry dentry = readDentry(fs, cur_dir_id);
    for (int i = 0; i < dentry.sub_dir_count; ++i) {
        if (strcmp(path, dentry.sub_dir[i]) == 0) {
            cur_dir_id = dentry.sub_dir_inode[i];
            break;
        }
    }

    freeDentry(&dentry);

    if (*cur_id == cur_dir_id) {
        // fprintf(stderr, "error: %s no such file or directory\n", path);
        return false;
    }

    INode inode = readInode(fs, cur_dir_id);
    if (((inode.type >> 9) & 7) != 4) {
        // fprintf(stderr, "error: %s is not a directory\n", path);
        return false;
    } else {
        *cur_id = cur_dir_id;
        return true;
    }
}

int getFatherId(FileSystem* fs, ui16 cur_id) {
    INode cur_inode = readInode(fs, cur_id);
    Dentry dentry = readDentry(fs, cur_id);
    int res = dentry.father_inode;
    freeDentry(&dentry);
    return res;
}

void pwd_get(FileSystem* fs, ui16 cur_id, char* path, int path_length) {
    if (cur_id == fs->root_inode) {
        return;
    }
    INode cur_inode = readInode(fs, cur_id);
    Dentry dentry = readDentry(fs, cur_id);
    pwd_get(fs, dentry.father_inode, path, path_length + dentry.name_length);
    path = realloc(path, path_length + dentry.name_length + 2);
    strcat(path, "/");
    strcat(path, dentry.name);
    freeDentry(&dentry);
}

void pwd(FileSystem* fs) {
    if (fs->current_dir_inode == fs->root_inode) {
        fs->current_dir_path = realloc(fs->current_dir_path, 2);
        strcpy(fs->current_dir_path, "/");
    } else {
        char* path = malloc(1);
        path[0] = '\0';
        pwd_get(fs, fs->current_dir_inode, path, 1);
        fs->current_dir_path = realloc(fs->current_dir_path, strlen(path) + 1);
        strcpy(fs->current_dir_path, path);
        free(path);
    }
}

void cd(FileSystem* fs, char* path) {
    // 解析路径，获取目录名和父目录名
    if (path[0] == '/') {
        char* tmp_path = malloc(strlen(path) + 1);
        strcpy(tmp_path, path);

        ui16 cur_dir_id = fs->root_inode;
        char* token = strtok(tmp_path, "/");
        while(token != NULL){
            if (!cd_(fs, &cur_dir_id, token)) {
                // fprintf(stderr, "error: %s no dir exists\n", path);
                free(tmp_path);
                return;
            }
            token = strtok(NULL, "/");
        }

        fs->current_dir_inode = cur_dir_id;
        pwd(fs);
        free(tmp_path);
    } else {
        char* tmp_path = malloc(strlen(path) + 1);
        strcpy(tmp_path, path);

        ui16 cur_dir_id = fs->current_dir_inode;
        char* token = strtok(tmp_path, "/");
        while(token != NULL){
            if (!cd_(fs, &cur_dir_id, token)) {
                // fprintf(stderr, "error: %s no dir exists\n", path);
                free(tmp_path);
                return;
            }
            token = strtok(NULL, "/");
        }

        fs->current_dir_inode = cur_dir_id;
        pwd(fs);
        free(tmp_path);
    }
}

//无斜杠分割的路径
int dirGet(FileSystem* fs, ui16 cur_id, char* path) {
    ui16 cur_dir_id = cur_id;
    INode cur_inode = readInode(fs, cur_dir_id);
    if (((cur_inode.type >> 9) & 7) != 4) {
        // fprintf(stderr, "error: %s not a directory\n", path);
        return false;
    }

    Dentry dentry = readDentry(fs, cur_dir_id);
    for (int i = 0; i < dentry.sub_dir_count; ++i) {
        if (strcmp(path, dentry.sub_dir[i]) == 0) {
            cur_dir_id = dentry.sub_dir_inode[i];
            break;
        }
    }

    freeDentry(&dentry);

    if (cur_id == cur_dir_id) {
        return 0;
    } else {
        INode inode = readInode(fs, cur_dir_id);
        return inode.type >> 9;
    }
}

//无斜杠分割的路径
int getInodeFromName(FileSystem* fs, ui16 cur_id, char* path, ui16* res) {
    ui16 cur_dir_id = cur_id;
    INode cur_inode = readInode(fs, cur_dir_id);
    if (((cur_inode.type >> 9) & 7) != 4) {
        // fprintf(stderr, "error: %s not a directory\n", path);
        return false;
    }

    Dentry dentry = readDentry(fs, cur_dir_id);
    for (int i = 0; i < dentry.sub_dir_count; ++i) {
        if (strcmp(path, dentry.sub_dir[i]) == 0) {
            *res = cur_dir_id = dentry.sub_dir_inode[i];
            break;
        }
    }

    freeDentry(&dentry);

    if (cur_id == cur_dir_id) {
        return 0;
    } else {
        INode inode = readInode(fs, cur_dir_id);
        return inode.type >> 9;
    }
}

void mkdir_(FileSystem* fs, char* path, ui16 cur_dir_id) {
    // printf("cur %d\n", cur_dir_id);
    char* last_token = NULL;
    // printf("Fuck\n");
    char* tmp_p = malloc(strlen(path) + 1);
    strcpy(tmp_p, path);
    char* token = strtok(tmp_p, "/");

    while(token != NULL){
        last_token = token;
        token = strtok(NULL, "/");

        if (token == NULL) {
            //创建目录
            if (dirGet(fs, cur_dir_id, last_token)) {
                // fprintf(stderr, "error: %s already exists\n", last_token);
                return;
            } else {
                // printf("mkdir ing %s\n", last_token);
                ui16 son = createDentry(fs, cur_dir_id, last_token, strlen(last_token));
                cur_dir_id = son;
            }
        } else {
            // printf("mkdir ing %s\n", last_token);
            cd_(fs, &cur_dir_id, last_token);
        }
    }
    free(tmp_p);
}

void mkdir(FileSystem* fs, char* path) {
    // 解析路径，获取目录名和父目录名
    if (path[0] == '/') {
        // printf("mkdir root\n");
        mkdir_(fs, path, fs->root_inode);
    } else {
        // printf("mkdir cur\n");
        mkdir_(fs, path, fs->current_dir_inode);
    }
}

void deleteFile(FileSystem* fs, ui16 dir_id, ui16 file_id) {


    INode fileInode = readInode(fs, file_id);
    
    if (((fileInode.type >> 9) & 7) == 4) {
        // fprintf(stderr, "error: %s is a directory\n");
        return;
    }

    dentryDeleteSon(fs, dir_id, file_id);
    if (fileInode.link_count == 1) {
        fsFree(fs, &fileInode);
        freeInode(fs, file_id);
    } else {
        fileInode.link_count--;
        writeInode(fs, file_id, &fileInode);
    }   
}


void ls_(FileSystem *fs, ui16 dir_id){
    printf("reading dentry %d\n", dir_id);
    INode tmp = readInode(fs, dir_id);
    printf("type %o\n", tmp.type);
    printf("f:%d\n", tmp.direct_block[0]);
    printf("sz:%d\n", tmp.size);
    Dentry dentry = readDentry(fs, dir_id);
    printf("%d\n",dentry.sub_dir_count);
    printf("dir\t");
    printf("inode\t");
    printf("size\t");
    printf("link\t");
    printf("type\t");
    printf("created\n");
    printf("modified\n");
    
    for(int i = 0; i < dentry.sub_dir_count; i++){
        // format as ls -l
        printf("%s\t",dentry.sub_dir[i]);
        INode inode = readInode(fs, dentry.sub_dir_inode[i]);
        printf("%d\t",inode.inode_number);
        printf("%d\t",inode.size);
        printf("%d\t",inode.link_count);
        printf("%o\t",inode.type);
        printf("%s\t",ctime(&inode.created_time));
        printf("%s\t",ctime(&inode.modified_time));
        // printf("%s\t",ctime(&inode.access_time));
        printf("\n");
        // printf("%s\n",dentry.sub_dir[i]);
    }
    freeDentry(&dentry);
}

void ls(FileSystem* fs, char* path) {
    ui16 cur_dir_id = fs->root_inode;
    // printf("cur :dir %d\n", cur_dir_id);

    if(path == NULL) {
        ls_(fs,fs->current_dir_inode);
        return;
    }

    // ls /root/abc 怎么处理
    if(path[0] == '/'){
        cur_dir_id = fs->root_inode;    
    } else {
        cur_dir_id = fs->current_dir_inode;
    }
    char *token = strtok(path, "/");
    while(token != NULL){
        if (!dirGet(fs, cur_dir_id, token)) {
            // fprintf(stderr, "error: %s is no exists\n", token);
            return;
        } 
        // printf("ls ing %s\n", token);
        cd_(fs, &cur_dir_id, token);
        token = strtok(NULL, "/");
    }
    
    // printf("ls ing %d\n", cur_dir_id);
    ls_(fs, cur_dir_id);
}


void createFile(FileSystem* fs,ui16 cur_dir_id, char* name, ui16 name_length) {
    for (int i = 0; i < 10; ++i) {
        printf("%d ", fs->inode_bitmap[i]);
    }
    printf("\n");
    if (dirGet(fs, cur_dir_id, name)) {
        // fprintf(stderr, "error: %s already exists\n", name);
        return;
    } else {
        ui16 file_id = createNewInode(fs, 02777);
        // printf("name %s, len%d, suplen: %d \n", name, strlen(name), name_length);
        dentryAddSon(fs, cur_dir_id, file_id, name, name_length);
    }
}

void create(FileSystem* fs, char* path) {
    // pirintInodeMap(fs);
    ui16 cur_dir_id;
    if (path[0] == '/') {
        cur_dir_id = fs->root_inode;
    } else {
        cur_dir_id = fs->current_dir_inode;
    }

    char* token = strtok(path, "/");
    char* last_token = NULL;
    while(token != NULL){
        last_token = token;
        token = strtok(NULL, "/");
        if (token == NULL) {
            createFile(fs, cur_dir_id, last_token, strlen(last_token));
            return false;
        } else {
            if (!cd_(fs, &cur_dir_id, last_token)) {
                // fprintf(stderr, "error: %s no dir exists\n", last_token);
                return false;
            }
        }
    }
}

bool Parser(FileSystem* fs, char* path, ui16* result) {
    ui16 cur_dir_id;
    if (path[0] == '/') {
        cur_dir_id = fs->root_inode;
    } else {
        cur_dir_id = fs->current_dir_inode;
    }

    char* tmp_path = malloc(strlen(path) + 1);
    strcpy(tmp_path, path);
    char* token = strtok(tmp_path, "/");
    char* last_token = NULL;
    while(token != NULL){
        last_token = token;
        token = strtok(NULL, "/");
        if (token == NULL) {
            bool ok = getInodeFromName(fs, cur_dir_id, last_token, result);
            free(tmp_path);
            return ok;
        } else {
            if (!cd_(fs, &cur_dir_id, last_token)) {
                // fprintf(stderr, "error: no such fule or dir\n", last_token);
                free(tmp_path);
                return false;
            }
        }
    }
    free(tmp_path);
}

void rm_(FileSystem* fs, ui16 father_dir_id, ui16 cur_dir_id, int recursive) {
    INode cur_inode = readInode(fs, cur_dir_id);
    if (((cur_inode.type >> 9) & 7) == 4) {
        if (recursive) {
            Dentry dentry = readDentry(fs, cur_dir_id);
            while (dentry.sub_dir_count > 2) {
                rm_(fs, cur_dir_id, dentry.sub_dir_inode[dentry.sub_dir_count - 1], recursive);
                freeDentry(&dentry);
                dentry = readDentry(fs, cur_dir_id);
            }
            deleteDnetry(fs, cur_dir_id);
            freeDentry(&dentry);
            return;
        } else {
            // fprintf(stderr, "cannot rm a directory, please use -r\n");
            return;
        }
    } else {
        // file or soft link
        // fprintf(stderr, "rm file ing %d\n", cur_dir_id);
        deleteFile(fs, father_dir_id, cur_dir_id);
    }
}


bool getParentAndDir(FileSystem* fs, char* path, ui16* parent, ui16* result) {
    ui16 cur_dir_id;
    if (path[0] == '/') {
        cur_dir_id = fs->root_inode;
    } else {
        cur_dir_id = fs->current_dir_inode;
    }

    char* tmp_path = malloc(strlen(path) + 1);
    strcpy(tmp_path, path);
    char* token = strtok(tmp_path, "/");
    char* last_token = NULL;
    while(token != NULL){
        last_token = token;
        token = strtok(NULL, "/");
        if (token == NULL) {
            bool ok = getInodeFromName(fs, cur_dir_id, last_token, result);
            free(tmp_path);
            if (ok) {
                *parent = cur_dir_id;
            }
            return ok;
        } else {
            if (!cd_(fs, &cur_dir_id, last_token)) {
                // fprintf(stderr, "error: no such fule or dir\n", last_token);
                free(tmp_path);
                return false;
            }
        }
    }
    free(tmp_path);
}

void rm(FileSystem* fs, char* path, int recursive){
    // 解析路径，获取inode和父inode
    ui16 father_dir_id, cur_dir_id;
    if (getParentAndDir(fs, path, &father_dir_id, &cur_dir_id)) {
        // printf("removing %d %d\n", father_dir_id, cur_dir_id);
        rm_(fs, father_dir_id, cur_dir_id, recursive);
    } else {
        // fprintf(stderr, "error: no such file or directory\n");
    }
}

// 返回open的哪项
int open_(FileSystem* fs, UserOpenTable* tb, ui16 inode_num) {
    int id = -1;
    for(int i = 0; i < tb->size; i++){
        if(tb->items[i].inode.inode_number == inode_num){
            id = i;
            break;
        }
    }

    if (id != -1){
        // 不用报错
        // printf("The file is already opening\n");
        return id;
    } else {
        INode inode = readInode(fs, inode_num);
        // printf("open inode: %d\n", inode.inode_number);
        UserOpenItem item;
        item.inode = inode;
        item.offset = 0;
        item.modify = false;
        tbl_push_back(tb, item);
        return tb->size - 1;
    }
}

void open(FileSystem* fs,UserOpenTable* tb, char* path){
    ui16 in;
    if(Parser(fs, path, &in)){
        printf("open inode: %d\n", in);
        open_(fs, tb, in);
    } else {
        printf("no such file\n");
    }
}

void closeItem(FileSystem* fs,UserOpenTable* tb, int id) {
    if (id >= 0 && id < tb->size) {
        if (tb->items[id].modify) {
            // 更新修改时间
            tb->items[id].inode.modified_time = time(NULL);
            writeInode(fs, tb->items[id].inode.inode_number, &tb->items[id].inode);
        }
        tbl_remove(tb, id);
    }
}

void close_(FileSystem* fs, UserOpenTable* tb,ui16 inode_num){
    int id = -1;
    for(int i = 0; i < tb->size; i++){
        if(tb->items[i].inode.inode_number == inode_num){
            id = i;
            break;
        }
    }
    // printf("close id: %d\n", id);
    if (testInode(fs, inode_num)) {
        closeItem(fs, tb, id);
    }
}   

//怎么括号不对齐的

void close(FileSystem* fs,UserOpenTable* tb, char* path){
    ui16 in;
    if (Parser(fs, path, &in)) {
        close_(fs, tb, in);
    } else {
        printf("no such file\n");
    }
}

int read(FileSystem* fs,UserOpenTable* tb, char* path, int length, void* content) {
    ui16 in;
    if (Parser(fs, path, &in)) {
        int id = open_(fs, tb, in);

        // printf("Opened %d\n", id);
        // printf("Reading %d %d %d\n", id, tb->items[id].offset, length);

        if (tb->items[id].offset >= tb->items[id].inode.size) {
            printf("EOF\n");
            memset(content, 0, length);
            return 0;
        }
        if (length > tb->items[id].inode.size - tb->items[id].offset) {
            length = tb->items[id].inode.size - tb->items[id].offset;
        }
        int ok = fsRead(fs, &tb->items[id].inode, tb->items[id].offset, content, length);
        // for (int i = 0; i < length; ++i) {
        //     printf("%x ", ((char*)content)[i]);
        // }
        // printf("\n");
        tb->items[id].offset += ok; 
        return ok;
    } else {
        perror("no such file\n");
    }
}

void write_(FileSystem* fs,UserOpenTable* tb, int idx, int length, char* content, int opt){
    printf("WT %d %d %d\n", idx, length, opt);
    if (opt == 0) {
        // 覆盖写
        tb->items[idx].offset = 0;
    } else if (opt == 2) {
        // 追加写
        tb->items[idx].offset = tb->items[idx].inode.size;
    }
    if (length != 0) {
        // printf("WRITING!!!\n");
        // printf("%d %d %d\n", idx, length, opt);
        // printf("%s\n", content);
        fsWrite(fs, &tb->items[idx].inode, tb->items[idx].offset, content, length);
        tb->items[idx].modify = true;
        printf("WRITTED!!!\n");
    }
}

void write(FileSystem* fs, UserOpenTable* tb, char* path, int length, char* content, int opt){
    ui16 in;
    if (Parser(fs, path, &in)) {
        int id = open_(fs, tb, in);
        write_(fs, tb, id, length, content, opt);
        close_(fs, tb, in);
        open_(fs, tb, in);
    } else {
        perror("no such file\n");
    }
}



void exitfs(FileSystem* fs, UserOpenTable* tb, FILE *stream) {
    while (tb->size) {
        closeItem(fs, tb, tb->size - 1);
    }
    if (fs->disk.base != NULL) {
        fwrite(fs->disk.base, 1, fs->disk.block_size * fs->super_block.block_num, stream);
        printf("Saved!! \n");
        fclose(stream);
    } 
}



void loadFs(FileSystem* fs, FILE *stream) {
    if (stream == NULL) {
        // fprintf(stderr, "wrong, can't open the file\n");
        return;
    } else {
        void* buffer;

        buffer = malloc(BLOCK_SIZE);
        size_t read_cnt = fread(buffer, 1, BLOCK_SIZE, stream);
        
        if (memcmp(buffer, "ext233233", 10) != 0) {
            free(buffer);
            return;
        }

        memcpy(&fs->super_block, buffer + 10, sizeof(SuperBlock));
        
        printf("super block: \n");
        printf("inode_bitmap_block: %d\n", fs->super_block.inode_bitmap_block);
        printf("block_bitmap_block: %d\n", fs->super_block.block_bitmap_block);
        printf("inode_table_block: %d\n", fs->super_block.inode_table_block);
        printf("data_block: %d\n", fs->super_block.data_block);
        printf("inode_num: %d\n", fs->super_block.inode_num);
        printf("block_num: %d\n", fs->super_block.block_num);
        printf("inode_size: %d\n", fs->super_block.inode_size);
        printf("block_size: %d\n", fs->super_block.block_size);


        diskInit(&fs->disk, fs->super_block.block_size, fs->super_block.block_num);
        diskWriteBlock(&fs->disk, 0, buffer);

        for (int i = 1; i < fs->super_block.block_num; i++) {
            fread(buffer, 1, fs->disk.block_size, stream);
            diskWriteBlock(&fs->disk, i, buffer);
        }
        free(buffer);

        int offset = 1;
        // 读取inode bitmap
        fs->inode_bitmap = (ui32*)malloc(fs->disk.block_size * fs->super_block.inode_bitmap_block);
        for (int i = 0; i < fs->super_block.inode_bitmap_block; i++) {
            // 一块一块读取 
            // printf("reading inode bitmap %d\n", offset);
            diskReadBlock(&fs->disk, offset, fs->inode_bitmap + i * fs->disk.block_size / 4);
            offset++;
        }

        // 读取block bitmap   
        fs->block_bitmap = (ui32*)malloc(fs->disk.block_size * fs->super_block.block_bitmap_block);
        for (int i = 0; i < fs->super_block.block_bitmap_block; i++) {
            // 一块一块读取
            // printf("reading block bitmap %d\n", offset);
            diskReadBlock(&fs->disk, offset, fs->block_bitmap + i * fs->disk.block_size / 4);
            offset++;
        }

        fs->current_dir_inode = fs->root_inode = 0;
        // printf("root inode: %d\n", fs->root_inode);
        fs->current_dir_path = (char*)malloc(2);
        fs->current_dir_path[0] = '/';
        fs->current_dir_path[1] = '\0';
    }
}
