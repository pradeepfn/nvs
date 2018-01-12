#include <stdlib.h>
#include "nvs/store.h"
#include "nvs/store_manager.h"

#include "wrapper.h"

#define STORE_ID "gtc_store"

extern "C" {
    nvs::Store *st;

    int nvs_init_(int *proc_id) {
        std::string store_name = std::string(STORE_ID) + std::string("/") +std::to_string(*proc_id);
        st = nvs::StoreManager::GetInstance(store_name);
    }


    void *alloc(unsigned long *size, char *s) {
        void *ptr;
        st->create_obj(std::string(s),*size, &ptr);
        return ptr;
    }

    void nvs_free_(void *ptr) {
    }

    void nvs_snapshot_(int *proc_id) {
        //st->put_all();
    }

    int nvs_finalize_() {
    }
}