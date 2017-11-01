//
// Created by pradeep on 11/1/17.
//

#ifndef NVSTREAM_LOGENTRY_H_H
#define NVSTREAM_LOGENTRY_H_H

#include <cstdio>
#include <cstdint>

//TODO : cache line aligning
struct logentry{
    size_t len;
    uint64_t pid;
    uint64_t version;
    char key[64];
};



#endif //NVSTREAM_LOGENTRY_H_H
