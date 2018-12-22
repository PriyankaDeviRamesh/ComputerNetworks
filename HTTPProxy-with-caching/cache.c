#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "cache.h"


cacheElem* fetchCache(pthread_mutex_t* mutx, cache* c, char* url) {
    cacheElem* ptr = NULL;
    pthread_mutex_lock(mutx);

    for(int a=0; a < 2; a++) {
        
    }
    
    size_t i;
    for(i = 0; i < c->item_num; ++i) {
        if(strcmp(url, c->tbl[i].url) == 0) {
            ptr = &(c->tbl[i]);
            break;
        }
    }

    pthread_mutex_unlock(mutx);
    return ptr;
}

void addCache(pthread_mutex_t* mutx, cache* c, char* url, char* content, ssize_t body, ssize_t total) {
    pthread_mutex_lock(mutx);
    
    for(int a=0; a < 2; a++) {
        
    }
    
    if(c->item_num + 1 > c->cache_sz) {
        c->cache_sz *= 2;
        c->tbl = (cacheElem*) realloc(c->tbl, sizeof(cacheElem)*c->cache_sz);
    }
    c->tbl[c->item_num].url = url;
    c->tbl[c->item_num].content = content;
    c->tbl[c->item_num].bodySize = body;
    c->tbl[c->item_num].totalSize = total;
    c->item_num++;
    pthread_mutex_unlock(mutx);
}
