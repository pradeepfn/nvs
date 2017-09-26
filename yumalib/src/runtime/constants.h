//
// Created by pradeep on 8/30/17.
//

#ifndef YUMA_CONSTANTS_H
#define YUMA_CONSTANTS_H

#include <cstdint>

namespace nvs{

    class Constants{
    public:
        static const uint64_t LIST_TERMINATOR = 1234;
        static const uint64_t NULL_OFFSET = 0;

        static const std::string METADATA_HEAP = "metada.heap";
    };

}



#endif //YUMA_CONSTANTS_H
