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
     //读取inode编号，大小为ui16
    sendAndShift(&res.inode, &ptr, sizeof(ui16));
    // printf("dentry inode: %d\n", res.inode);
    sendAndShift(&res.father_inode, &ptr, sizeof(ui16));
    // 读取父目录inode编号，大小为ui16
    // printf("dentry fatherinode: %d\n", res.father_inode);
    sendAndShift(&res.name_length, &ptr, sizeof(ui16));
    // printf("dentry namelength: %d\n", res.name_length);
    // 读取目录名，大小为name_length(因为是字符串，需要动态分配内存)
    res.name = malloc(res.name_length);
    sendAndShift(res.name, &ptr, res.name_length);
    // printf("dentry name: %s\n", res.name);
    // 读取子目录数量，大小为ui16
    sendAndShift(&res.sub_dir_count, &ptr, sizeof(ui16));
    // printf("dentry sub_dir_count: %d\n", res.sub_dir_count);
    // 动态分配子目录信息各个字段数组内存
    res.sub_dir_inode = malloc(res.sub_dir_count * sizeof(ui16));
    res.sub_dir_length = malloc(res.sub_dir_count * sizeof(ui16));
    res.sub_dir = malloc(res.sub_dir_count * sizeof(char*));
    // 遍历sub_dir_count个子目录，给相关信息赋值，这里形如sub_dir_inode为数组首地址，+1为下一个地址
    for (int i = 0; i < res.sub_dir_count; ++i) {
        sendAndShift(res.sub_dir_inode + i, &ptr, sizeof(ui16));
        sendAndShift(res.sub_dir_length + i, &ptr, sizeof(ui16));
        // printf("receiving : %d %d\n", res.sub_dir_inode[i], res.sub_dir_length[i]);
        // 这里要再次动态分配内存是因为子sub_dir是个字符串数组，char**类型，数组单元为char*类型，需要动态分配内存
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
    // void receiveAndShift(void **dst, void *src, int size)
    // 从 src 复制 size 字节的数据到 *dst，并更新 *dst 指向下一个位置，处理数据的序列化
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
    // 向文件系统中写入指定 inode 的数据
    fsWrite(fs, &inode, 0, buffer, buffer_size);
    // 将指定 inode 号的 inode 数据写入到文件系统中
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
    // 下面的realloc是为了给father的子目录数组扩容，重新分配内存，增加一个元素的内存
    // 扩容完毕后，将新的子目录信息添加到数组中
    father.sub_dir_inode = realloc(father.sub_dir_inode, (father.sub_dir_count + 1) * sizeof(ui16));
    father.sub_dir_length = realloc(father.sub_dir_length, (father.sub_dir_count + 1) * sizeof(ui16));
    father.sub_dir = realloc(father.sub_dir, (father.sub_dir_count + 1) * sizeof(char*));
    father.sub_dir_inode[father.sub_dir_count] = inode_id;
    father.sub_dir_length[father.sub_dir_count] = name_length + 1;
    father.sub_dir[father.sub_dir_count] = malloc(name_length + 1);
    memcpy(father.sub_dir[father.sub_dir_count], name, name_length + 1);
    father.sub_dir_count++;
    // 保存目录信息到文件系统，并释放内存
    saveDentry(fs, &father);
    freeDentry(&father);
}

//创建新的 Dentry，为当前目录（.）和父目录（..）初始化信息
ui16 createDentry(FileSystem* fs, ui16 father_inode_id, char* name, ui16 name_length) {
    Dentry res;
    // 创建新的inode
    // 创建新的inode，权限巨高，4代表目录类型
    // 赋值传入的参数
    // +1是为了在字符串后面加上一个'\0'，表示字符串结束
    res.inode = createNewInode(fs, 04777);
    res.father_inode = father_inode_id;
    res.name_length = name_length + 1;
    res.name = malloc(name_length);
    memcpy(res.name, name, name_length + 1);

    // 初始化子目录信息
    // res.sub_dir_count的值为2，表示在创建一个目录时，默认包含两个子目录项。这是因为每个目录都会包含两个特殊的目录项：
    // 当前目录（.）：指向目录本身的目录项。
    // 父目录（..）：指向目录的父目录的目录项。
    res.sub_dir_count = 2;
    res.sub_dir_inode = malloc(2 * sizeof(ui16));
    res.sub_dir_length = malloc(2 * sizeof(ui16));
    res.sub_dir = malloc(2 * sizeof(char*));

    // 初始化当前目录（.）的信息
    // 同理，这里length为2，是因为"."后面加上一个'\0'，表示字符串结束
    res.sub_dir = malloc(2 * sizeof(char*));
    res.sub_dir_inode[0] = res.inode;
    res.sub_dir_length[0] = 2;
    res.sub_dir[0] = malloc(res.sub_dir_length[0]);
    memcpy(res.sub_dir[0], ".", 2);
    res.sub_dir[0][1] = 0;

    // 初始化父目录（..）的信息
    // 同理，这里length为3，是因为".."后面加上一个'\0'，表示字符串结束
    res.sub_dir_inode[1] = father_inode_id;
    res.sub_dir_length[1] = 3;
    res.sub_dir[1] = malloc(res.sub_dir_length[1]);
    memcpy(res.sub_dir[1], "..", 3);
    res.sub_dir[1][2] = 0;
    // 0默认为根目录
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
    // 找到要删除的子目录的索引
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
    // 创建新的子目录信息数组，并动态分配内存
    ui16* new_inode = malloc((dir_count - 1) * sizeof(ui16));
    ui16* new_sub_dir_length = malloc((dir_count - 1) * sizeof(ui16));
    char** new_sub_dir = malloc((dir_count - 1) * sizeof(char*));
    // 每次更新分成2个步骤，以index为界限，分别更新index索引前后的子目录信息，去除index索引的子目录信息
    memcpy(new_inode, father.sub_dir_inode, index * sizeof(ui16));
    memcpy(new_inode + index, father.sub_dir_inode + index + 1, (dir_count - index - 1) * sizeof(ui16));
    memcpy(new_sub_dir_length, father.sub_dir_length, index * sizeof(ui16));
    memcpy(new_sub_dir_length + index, father.sub_dir_length + index + 1, (dir_count - index - 1) * sizeof(ui16));
    memcpy(new_sub_dir, father.sub_dir, index * sizeof(char*));
    memcpy(new_sub_dir + index, father.sub_dir + index + 1, (dir_count - index - 1) * sizeof(char*));
    // 释放原来的子目录信息数组
    free(father.sub_dir_inode);
    free(father.sub_dir_length);
    free(father.sub_dir[index]);
    free(father.sub_dir);
    // 更新父目录信息
    father.sub_dir_inode = new_inode;
    father.sub_dir_length = new_sub_dir_length;
    father.sub_dir = new_sub_dir;
    father.sub_dir_count--;
    
    saveDentry(fs, &father);
    freeDentry(&father);
}

void deleteDnetry(FileSystem* fs, ui16 inode_id) {
    Dentry dentry = readDentry(fs, inode_id);
    // 2个指自己和父亲目录，大于2个表示存在子目录，这里默认不支持递归删除
    if (dentry.sub_dir_count > 2) {
        // fprintf(stderr, "error: directory is not empty\n");
        return;
    }
    // 释放资源
    dentryDeleteSon(fs, dentry.father_inode, inode_id);
    freeDentry(&dentry);
    INode inode = readInode(fs, inode_id);
    fsFree(fs, &inode);
    freeInode(fs, inode_id);
}
