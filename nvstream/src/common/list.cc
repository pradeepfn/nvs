#include "list.h"


namespace nvs {

    List::List(uint64_t head):listHead(head) {
    }

    List::~List() {



    }

    template<typename T, typename P>
    ErrorCode List::addNode(T *node) {
         node->next = listHead;
        listHead = 0;
        return NO_ERROR;
    }


    template<typename T, typename P>
    T List::findNode(P id) {

    }


    template<typename T, typename P>
    ErrorCode List::removeNode(P id) {}

}