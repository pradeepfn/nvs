//
// Created by pradeep on 9/12/17.
//

#ifndef YUMA_KEY_H
#define YUMA_KEY_H

#include <cstdint>
#include "nvs/errorCode.h"
#include "version.h"

namespace nvs {
/*
     * Key
     */
    class Key {

    private:
        //gptr *ptr;
        uint64_t size;
        /* bit field to keep track of modified pages of the buffer */
        char *barry;
        /*vector of versions */



    public:
        Version *getVersion(uint64_t version);

        uint64_t getSize();

        ErrorCode createVersion(uint64_t version);

        ErrorCode deleteVesion(uint64_t version);

        ErrorCode getVersion(uint64_t version, uint64_t **addr);

    };

}

#endif //YUMA_KEY_H
