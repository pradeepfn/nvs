//
// Created by pradeep on 8/30/17.
//

#ifndef YUMA_CONSTANTS_H
#define YUMA_CONSTANTS_H

#include <cstdint>
#include <string>

namespace nvs{

    class Constants{
    public:
        static const uint64_t NULL_OFFSET = 0;
    };

    using ProcessId = uint64_t ;
}



#endif //YUMA_CONSTANTS_H
