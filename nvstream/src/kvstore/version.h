
#ifndef YUMA_VERSION_H
#define YUMA_VERSION_H

#include "serializationTypes.h"

namespace nvs{
    class Version {

        version_t *v;

    public:
        ErrorCode getData(uint64_t **buf_addr);
        ErrorCode putdata(uint64_t **buf_addr);
    };

}
#endif //YUMA_VERSION_H
