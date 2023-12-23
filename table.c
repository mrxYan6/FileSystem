#include "fs.h"

void tbl_push_back(UserOpenTable* tb, UserOpenItem item){
    if(tb->size == tb->capacity){
        int new_capacity = !tb -> capacity ? 1 : tb -> capacity * 2;
        UserOpenItem* new_items = realloc(tb->items,new_capacity * sizeof(UserOpenItem));
        if (new_items == NULL){
            fprintf(stderr, "wrong, can't expand the table\n");
            return;
        }

        tb->items=new_items;
        tb->capacity=new_capacity;
    }
    tb->items[tb->size++]=item;
}

void tbl_pop_back(UserOpenTable* tb){
    if(tb->size==0){
        fprintf(stderr,"wrong,the table size is already zero\n");
    }
    else tb->size--;
    return;
}

void tbl_init(UserOpenTable* tb){
    tb->items=NULL;
    tb->size=0;
    tb->capacity=0;
}

void tbl_destroy(UserOpenTable* tb){
    free(tb->items);
    tb->items=NULL;
    tb->size=0;
    tb->capacity=0;
}

void tbl_resize(UserOpenTable* tb, int new_size){
    UserOpenItem* new_items=realloc(tb->items,new_size*sizeof(UserOpenItem));
    if(new_items==NULL){
        fprintf(stderr,"failed to resize");
        return;
    }
    tb->size=new_size<tb->size ? new_size:tb->size;
    tb->capacity=new_size;
}
void tbl_clear(UserOpenTable* tb){
    tb->size=0;
}
void tbl_remove(UserOpenTable* tb, int index){
    if(index < 0 || index >= tb->size){
        fprintf(stderr,"index is invaild");
        return;
    }
    for(int i=index;i<tb->size;i++){
        tb->items[i]=tb->items[i+1];
    }
    tb->size--;
}