
#ifndef YUMA_SERIALIZATIONTYPES_H
#define YUMA_SERIALIZATIONTYPES_H

typedef struct store_t_{
    char storeId[100];



} store_t;

typedef struct objkey_t {
    char keyId[100];
    uint64_t next; // offset of the next element.




} objkey_t;

#endif //YUMA_SERIALIZATIONTYPES_H
