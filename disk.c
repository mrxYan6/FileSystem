#include <string.h>
#include <stdlib.h> 
#include <stdio.h>
#include <assert.h>
#include "disk.h"

void diskInit(Disk* disk, int block_size, int block_num){
    disk->block_size = block_size;
    disk->block_num = block_num;
    disk->base = malloc(block_size * block_num);
    memset(disk->base, 0, block_size * block_num);
}

//读一整块
void diskReadBlock(Disk* disk, int block_num, void* buf){
    assert(block_num>=0&&block_num<disk->block_num);
    memcpy(buf, disk->base + block_num * disk->block_size, disk->block_size);
}

//写一整块
void diskWriteBlock(Disk* disk, int block_num, void* buf){
    assert(block_num >= 0 && block_num < disk->block_num);
    memcpy(disk -> base + block_num * disk->block_size, buf, disk->block_size);
}