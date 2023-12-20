#ifndef DISK_H
#define DISK_H

typedef struct Disk {
    void* base;
    int block_size;
    int block_num;
}Disk;

void diskInit(Disk* disk, int block_size, int block_num);

//读一整块
void diskReadBlock(Disk* disk, int block_num, void* buf);

//写一整块
void diskWriteBlock(Disk* disk, int block_num, void* buf);

#endif