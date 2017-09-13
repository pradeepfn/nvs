
#include <cstdint>
#include <nvs/errorCode.h>
#include "key.h"

namespace nvs{

    ErrorCode Key::createVersion(uint64_t version, Version *vob) {

        vob = new Version(version);
        return NO_ERROR;
    }

    ErrorCode Key::deleteVesion(uint64_t version) {



    }

    ErrorCode Key::getVersion(uint64_t version, uint64_t **addr) {

    }

    uint64_t Key::getSize() {

    }



}