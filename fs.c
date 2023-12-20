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

//释放文件系统中指定块号的数据块
int freeBlock(FileSystem* fs, ui16 block_num) {
    assert(block_num >= 0 && block_num < fs->super_block.data_block);
    int int_number = block_num >> 5;
    int bit_number = block_num & 0x1f;
    fs->block_bitmap[int_number] &= ~(1 << bit_number);
    return 1;
}

//占用文件系统中指定块号的数据块，检查块号的合法性，然后计算出在位图中的位置，将相应位置的位设置为 1
void occupyBlock(FileSystem* fs, ui16 block_num) {
    assert(block_num>=0&&block_num<fs->super_block.data_block);   
    int int_number = block_num >> 5;
    int bit_number = block_num & 0x1f;
    fs->block_bitmap[int_number]|=(1<<bit_number);
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

//释放文件系统中指定的 inode
void freeInode(FileSystem* fs, ui16 inode_num) {
    assert(inode_num >= 0 && inode_num < fs->super_block.inode_num);   
    int int_number = inode_num >> 5;
    int bit_number = inode_num & 0x1f;
    fs->inode_bitmap[int_number] &= ~(1 << bit_number);
}

//占用文件系统中指定的 inode,检查 inode 号的合法性，然后计算出在位图中的位置，将相应位置的位设置为 1
void occupyInode(FileSystem* fs, ui16 inode_num) {
    assert(inode_num >= 0 && inode_num < fs->super_block.inode_num);   
    int int_number = inode_num >> 5;
    int bit_number = inode_num & 0x1f;
    fs->inode_bitmap[int_number] |= (1 << bit_number);
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

//从文件系统中读取数据块号为 data_block_num 的数据块到缓冲区 buf 中
void dataReadBlock(FileSystem* fs, int data_block_num, void* buf){
    int offset = 1 + fs->super_block.inode_bitmap_block + fs->super_block.block_bitmap_block + fs->super_block.inode_table_block;
    diskReadBlock(&fs->disk, data_block_num + offset, buf);
}

//将缓冲区 buf 中的数据写入文件系统中数据块号为 data_block_num 的数据块
void dataWriteBlock(FileSystem* fs, int data_block_num, ritevoid* buf){
    int offset = 1 + fs->super_block.inode_bitmap_block + fs->super_block.block_bitmap_block + fs->super_block.inode_table_block;
    dis(&fs->disk, data_block_num + offset, buf);
}

//创建一个新的 inode 并返回其编号
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

//释放一级索引块及其指向的数据块
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
        fprintf(stderr, "error: allocated_num > 0\n");
    }
    return !allocated_num;
}

//为一个 inode 分配数据块以满足指定的大小
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

//重新分配 inode 占用的数据块
void fsReAllocate(FileSystem* fs, INode* inode, int size) {
    fsFree(fs, inode);
    fsAllocate(fs, inode, size);
}

//从文件系统中读取指定 inode 的数据
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

//向文件系统中写入指定 inode 的数据
void fsWrite(FileSystem*fs, INode* inode, int offset, void* context, int size) {
    // 参考fsRead
    if (offset + size > inode->size) {
        fsAllocate(fs, inode, offset + size - inode->size);
    }
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
            int begin = cur % block_size;
            int end = (cur_block == end_block) ? end % block_size : block_size - 1;
            memcpy(buffer + begin, context + context_ptr, end - begin + 1);
            dataWriteBlock(fs, inode->direct_block[i], buffer);
            context_ptr += end - begin + 1;
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
            memcpy(buffer + begin, context + context_ptr, end - begin + 1);
            dataWriteBlock(fs, first_index[i], buffer);
            context_ptr += end - begin + 1;
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
                memcpy(buffer + begin, context + context_ptr, end - begin + 1);
                dataWriteBlock(fs, first_index[i], buffer);
                context_ptr += end - begin + 1;
                cur_block++;
                cur += end - begin + 1;
            }
            dataWriteBlock(fs, second_index[i], first_index);
            free(first_index);
        }

        dataWriteBlock(fs, inode->second_indexed_block, second_index);
        free(second_index);
    }

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
    INode inode = readInode(fs, inode_id);
    if (((inode.type >> 9) & 7) != 4) {
        fprintf(stderr, "error: not a directory\n");
        return res;
    }
    int offset = 0;
    int size = inode.size;
    void* buffer = malloc(size);
    // 从文件系统中读取数据到 buffer
    fsRead(fs, &inode, offset, buffer, size);
    // memcpy(&res.inode, buffer, sizeof(ui16));
    // offset += sizeof(ui16);
     // 解析数据并填充 Dentry 结构
    sendAndShift(&res.inode, &buffer, sizeof(ui16));
    sendAndShift(&res.father_inode, &buffer, sizeof(ui16));
    sendAndShift(&res.name_length, &buffer, sizeof(ui16));
    res.name = malloc(res.name_length);
    sendAndShift(res.name, &buffer, res.name_length);
    sendAndShift(&res.sub_dir_count, &buffer, sizeof(ui16));

    res.sub_dir_inode = malloc(res.sub_dir_count * sizeof(ui16));
    res.sub_dir_length = malloc(res.sub_dir_count * sizeof(ui16));
    res.sub_dir = malloc(res.sub_dir_count * sizeof(char*));

    for (int i = 0; i < res.sub_dir_count; ++i) {
        sendAndShift(res.sub_dir_inode + i, &buffer, sizeof(ui16));
        sendAndShift(res.sub_dir_length + i, &buffer, sizeof(ui16));
        res.sub_dir[i] = malloc(res.sub_dir_length[i]);
        sendAndShift(res.sub_dir[i], &buffer, res.sub_dir_length[i]);
    }
    return res;
}

// 把dentry翻译成流，然后调用fsWrite写入inode_id对应的inode，将给定的 Dentry 结构序列化，并将其保存到对应的 inode 中
void saveDentry(FileSystem* fs, Dentry* dentry) {
    // 计算需要的缓冲区大小
    size_t buffer_size = sizeof(ui16)*4+ dentry->name_length + 2 * dentry->sub_dir_count * sizeof(ui16);
    for(int i=0; i<dentry->sub_dir_count; i++){
        buffer_size += dentry->sub_dir_length[i];
    }
    // 分配缓冲区
    void* buffer = malloc(buffer_size);
    void* ptr = buffer;
    // 序列化数据到缓冲区
    recieveAndShift(&ptr, &dentry->inode, sizeof(ui16));
    receiveAndShift(&ptr, &dentry->father_inode, sizeof(ui16));
    receiveAndShift(&ptr, &dentry->name_length, sizeof(ui16));
    receiveAndShift(&ptr, dentry->name, dentry->name_length);
    receiveAndShift(&ptr, &dentry->sub_dir_count, sizeof(ui16));
    for(int i = 0; i < dentry->sub_dir_count; i++){
        receiveAndShift(&ptr, dentry->sub_dir_inode+i*sizeof(ui16), sizeof(ui16));
        receiveAndShift(&ptr, dentry->sub_dir_length+i*sizeof(ui16), sizeof(ui16));
        receiveAndShift(&ptr, dentry->sub_dir[i], dentry->sub_dir_length[i]);
    }
    // 读取对应的inode，将缓冲区写入文件系统
    INode inode = readInode(fs, dentry->inode);
    fsWrite(fs, &inode, 0, buffer, buffer_size);
    writeInode(fs, inode.inode_number, &inode);
    
    free(buffer);
}

//创建新的 Dentry，为当前目录（.）和父目录（..）初始化信息
Dentry createDentry(FileSystem* fs, ui16 father_inode_id, char* name, ui16 name_length) {
    Dentry res;
    // 创建新的inode
    res.inode = createNewInode(fs, 04777);
    res.father_inode = father_inode_id;
    res.name_length = name_length;
    res.name = malloc(name_length);
    memcpy(res.name, name, name_length);

    // 初始化子目录信息
    res.sub_dir_count = 2;
    res.sub_dir_inode = malloc(2 * sizeof(ui16));
    res.sub_dir_length = malloc(2 * sizeof(ui16));
    res.sub_dir = malloc(2 * sizeof(char*));

    // 初始化当前目录（.）的信息
    res.sub_dir = malloc(2 * sizeof(char*));
    res.sub_dir_inode[0] = res.inode;
    res.sub_dir_length[0] = 1;
    res.sub_dir[0] = malloc(1);
    memcpy(res.sub_dir[0], ".", 1);

    // 初始化父目录（..）的信息
    res.sub_dir_inode[1] = father_inode_id;
    res.sub_dir_length[1] = 2;
    res.sub_dir[1] = malloc(2);
    memcpy(res.sub_dir[1], "..", 2);

    // 将Dentry保存到文件系统
    saveDentry(fs, &res);
    return res;
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
void dentryAddSon(Dentry* father, Dentry* son) {
    ui16* new_inode = malloc((father->sub_dir_count + 1) * sizeof(ui16));
    ui16* new_sub_dir_length = malloc((father->sub_dir_count + 1) * sizeof(ui16));
    char** new_sub_dir = malloc((father->sub_dir_count + 1) * sizeof(char*));
    
    memcpy(new_inode, father->sub_dir_inode, father->sub_dir_count * sizeof(ui16));
    new_inode[father->sub_dir_count] = son->inode;
    memcpy(new_sub_dir_length, father->sub_dir_length, father->sub_dir_count * sizeof(ui16));
    new_sub_dir_length[father->sub_dir_count] = son->name_length;
    memcpy(new_sub_dir, father->sub_dir, father->sub_dir_count * sizeof(char*));
    for (int i = 0; i < father->sub_dir_count; ++i) {
        new_sub_dir[i] = malloc(father->sub_dir_length[i]);
        memcpy(new_sub_dir[i], father->sub_dir[i], father->sub_dir_length[i]);
    }
    new_sub_dir[father->sub_dir_count] = malloc(son->name_length);
    memcpy(new_sub_dir[father->sub_dir_count], son->name, son->name_length);

    free(father->sub_dir_inode);
    free(father->sub_dir_length);
    for (int i = 0; i < father->sub_dir_count; ++i) {
        free(father->sub_dir[i]);
    }
    free(father->sub_dir);

    father->sub_dir_inode = new_inode;
    father->sub_dir_length = new_sub_dir_length;
    father->sub_dir = new_sub_dir;
    father->sub_dir_count++;
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
        fprintf(stderr, "error: no such son\n");
        return;
    }

    ui16* new_inode = malloc((father.sub_dir_count - 1) * sizeof(ui16));
    ui16* new_sub_dir_length = malloc((father.sub_dir_count - 1) * sizeof(ui16));
    char** new_sub_dir = malloc((father.sub_dir_count - 1) * sizeof(char*));
    
    memcpy(new_inode, father.sub_dir_inode, index * sizeof(ui16));
    memcpy(new_inode + index, father.sub_dir_inode + index + 1, (father.sub_dir_count - index - 1) * sizeof(ui16));
    memcpy(new_sub_dir_length, father.sub_dir_length, index * sizeof(ui16));
    memcpy(new_sub_dir_length + index, father.sub_dir_length + index + 1, (father.sub_dir_count - index - 1) * sizeof(ui16));
    memcpy(new_sub_dir, father.sub_dir, index * sizeof(char*));
    memcpy(new_sub_dir + index, father.sub_dir + index + 1, (father.sub_dir_count - index - 1) * sizeof(char*));

    free(father.sub_dir_inode);
    free(father.sub_dir_length);
    for (int i = 0; i < father.sub_dir_count; ++i) {
        free(father.sub_dir[i]);
    }
    free(father.sub_dir);

    father.sub_dir_inode = new_inode;
    father.sub_dir_length = new_sub_dir_length;
    father.sub_dir = new_sub_dir;
    father.sub_dir_count--;
}

void deleteDnetry(FileSystem* fs, ui16 inode_id) {
    Dentry dentry = readDentry(fs, inode_id);
    if (dentry.sub_dir_count > 2) {
        fprintf(stderr, "error: directory is not empty\n");
        return;
    }
    dentryDeleteSon(fs, dentry.father_inode, inode_id);
    INode inode = readInode(fs, inode_id);
    fsFree(fs, &inode);
    freeInode(fs, inode_id);
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

    createDentry(fs, 0, "/", 1);
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

//无斜杠分割的路径
int dirGetType(FileSystem* fs, ui16 cur_id, char* paht) {
    ui16 cur_dir_id = cur_id;
    INode cur_inode = readInode(fs, cur_dir_id);
    if (((cur_inode.type >> 9) & 7) != 4) {
        fprintf(stderr, "error: not a directory\n");
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
        return 0;
    } else {
        INode inode = readInode(fs, cur_dir_id);
        return inode.type >> 9;
    }
}

void mkdir_(FileSystem* fs, char* path, ui16 cur_dir_id) {
    Dentry dentry = readDentry(fs, cur_dir_id);

    char* last_token = NULL;
    char* token = strtok(path, "/");

    while(token != NULL){
        last_token = token;
        token = strtok(NULL, "/");

        if (token == NULL) {
            //创建目录
            if (dirGet(fs, cur_dir_id, last_token)) {
                fprintf(stderr, "error: %s already exists\n", last_token);
                return;
            } else {
                Dentry son = createDentry(fs, cur_dir_id, last_token, strlen(last_token));
                dentryAddSon(&dentry, &son);
                saveDentry(fs, &dentry);
                freeDentry(&dentry);           
                dentry = son;
                cur_dir_id = dentry.inode;
            }
        } else {
            cd_(fs, &cur_dir_id, last_token);
        }
    }
}

void mkdir(FileSystem* fs, char* path) {
    // 解析路径，获取目录名和父目录名
    if (path[0] == '/') {
        mkdir_(fs, path, fs->root_inode);
    } else {
        mkdir_(fs, path, fs->current_dir_inode);
    }
}

void deleteFile(FileSystem* fs, ui16 dir_id, ui16 file_id) {
    INode fileInode = readInode(fs, file_id);
    
    if (((fileInode.type >> 9) & 7) == 4) {
        fprintf(stderr, "error: %s is a directory\n");
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

void rm_(FileSystem* fs, ui16 father_dir_id, ui16 cur_dir_id, int recursive) {
    INode cur_inode = readInode(fs, cur_dir_id);
    if (((cur_inode.type >> 9) & 7) == 4) {
        if (recursive) {
            Dentry dentry = readDentry(fs, cur_dir_id);
            while (dentry.sub_dir_count > 2) {
                rm_(fs, cur_dir_id, dentry.sub_dir_inode[dentry.sub_dir_count - 1], recursive);
            }
            deleteDnetry(fs, cur_dir_id);
            return;
        } else {
            fprintf(stderr, "cannot rm a directory, please use -r\n");
            return;
        }
    } else {
        // file or soft link
        deleteFile(fs, father_dir_id, cur_dir_id);
    }
}


void rm(FileSystem* fs, char* path, int recursive){
    // 解析路径，获取目录名和父目录名
    if (path[0] == '/') {
        rm_(fs, path, fs->root_inode, recursive);
    } else {
        rm_(fs, path, fs->current_dir_inode, recursive);
    }
}

void ls(FileSystem* fs, char* path){
    
    if(path == NULL){
        ls_(fs, fs->current_dir_inode);
    }    
    else if(path[0] == '/'){
        ls_(fs, fs->root_inode);
    }
}
