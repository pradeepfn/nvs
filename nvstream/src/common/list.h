//
// Created by pradeep on 9/15/17.
//

#ifndef YUMA_LIST_H
#define YUMA_LIST_H

#include <nvs/errorCode.h>
#include <cstdint>

namespace nvs {
    template<typename T, typename P>
    class List<T> {

    private:
        uint64_t listHead;

    public:
        List(uint64_t head);

        ~List();

        ErrorCode addNode(T *node);

        ErrorCode removeNode(P id);

        T findNode(P id, T **node);
    };


}

#endif //YUMA_LIST_H
