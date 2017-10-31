#include "object.h"


namespace nvs{

    Object::Object(std::string name, uint64_t size, uint64_t version, void *ptr):
        name(name),size(size),version(version),ptr(ptr)
    {

    }


    uint64_t Object::getVersion() {
        return this->version;
    }

    void Object::setVersion(uint64_t version) {
        this->version = version;
    }


}