#include <stdlib.h>
#include "nvs/store.h"
#include "nvs/store_manager.h"

#include "wrapper.h"

#define STORE_ID "amr_store"

extern "C" {
    nvs::Store *st;

    int nvs_init(int proc_id) {
        std::string store_name = std::string(STORE_ID) + std::string("/") +std::to_string(proc_id);
        st = nvs::StoreManager::GetInstance(store_name);
    }


    void *nvs_alloc(unsigned long size, char *s) {
        void *ptr;
        st->create_obj(std::string(s),size, &ptr);
        return ptr;
    }

    void nvs_free(void *ptr) {
		st->free_obj(ptr);
    }

    void nvs_snapshot(int proc_id) {
        st->put_all();
    }

    int nvs_finalize() {
        st->stats();
    }
}
