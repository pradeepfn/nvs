//
// Created by pradeep on 8/30/17.
//

#ifndef YUMA_ERRORCODE_H
#define YUMA_ERRORCODE_H

namespace nvs {
    enum ErrorCode {

        NO_ERROR = 0,
        DUPLICATE_KEY,
        ELEM_NOT_FOUND,
        // store specifics
        STORE_NOT_FOUND,

        //key error codes
        KEY_NOT_EXIST,


        // heap
        HEAP_NOT_EXIST,
        ID_IN_USE

    };

}


#endif //YUMA_ERRORCODE_H
