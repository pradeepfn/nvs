//
// Created by pradeep on 11/1/17.
//

#ifndef NVSTREAM_LOGENTRY_H_H
#define NVSTREAM_LOGENTRY_H_H

#include <cstdio>
#include <cstdint>

namespace nvs {
//TODO : cache line aligning
    struct logentry {
        size_t len;
        uint64_t pid;
        uint64_t version;
        char key[64];
    };

//structure for log traversing
    struct walkentry {
        size_t len;
        void *datap;
        uint64_t version;
        uint64_t start_offset;
        ErrorCode err;

    };

}
#endif //NVSTREAM_LOGENTRY_H_H
