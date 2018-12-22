
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifndef CACHE_H
#define CACHE_H


typedef struct {
    size_t bodySize;
    size_t totalSize;
    char* content;
    char* url;
} cacheElem;

typedef struct {
    size_t cache_sz;
    size_t item_num;
    cacheElem* tbl;
} cache;

cacheElem* fetchCache(pthread_mutex_t*, cache*, char* url);
void addCache(pthread_mutex_t*, cache*, char* url, char* content, ssize_t body, ssize_t total);

#endif
