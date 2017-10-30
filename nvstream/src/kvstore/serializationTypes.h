
#ifndef YUMA_SERIALIZATIONTYPES_H
#define YUMA_SERIALIZATIONTYPES_H

/*
 * following data types are used to store the info on shared memory
 * heap. We cannot directly use C++ class types as they get allocated
 * from a different allocator. For now we don't want to fight that war.
 */


    typedef struct store_t_ {
        char storeId[100];
        uint64_t next; // offset of the next elem
        uint64_t key_root; // root of the key structure for shared memory based object keys
    } store_t;

    typedef struct objkey_t {
        char keyId[100];
        uint64_t *obj_addr; // address of the version-0 object
        uint64_t size;
        uint64_t next; // offset of the next element.
    } objkey_t;


    typedef struct version_t_ {

        uint64_t version; // version id
        uint64_t *addr; // chunk address
        // TODO: we only have one chunk for the moment
        uint64_t size; //chunk size
    } version_t;


#endif //YUMA_SERIALIZATIONTYPES_H
