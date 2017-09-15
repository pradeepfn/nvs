#include "list.h"


namespace nvs {

    List::List(uint64_t head):listHead(head) {
    }

    List::~List() {



    }

    template<typename T, typename P>
    ErrorCode List::addNode(T node) {


    }


    template<typename T, typename P>
    T List::findNode(P id) {

    }


    template<typename T, typename P>
    ErrorCode List::removeNode(T node) {}

}