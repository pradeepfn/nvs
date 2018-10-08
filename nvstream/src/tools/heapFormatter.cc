/*
 *
 * create the root heap in a node/machine. This bootstraps the shared heap
 *
 */

#include <sys/mman.h>
#include <fcntl.h>
#include <string>

#include "sheap/layout.h"
#include "common/util.h"

#define ROOT_SIZE 4096


int main(int argc, char *argv[]) {

    int fd,ret;
    char *addr;
    std::string root_file_path = "/dev/shm/unity_NVS_ROOT";

    fd = open(root_file_path.c_str(), O_CREAT|O_RDWR, 0600);
    if (fd <0) {
        std::cout << "root file open failed" << strerror(errno);
        exit(1);
    }
    ret = fallocate(fd,0,0,ROOT_SIZE);
    if (ret < 0) {
        std::cout << "fallocate failed";
        exit(1);
    }


    addr = (char *) mmap(NULL,ROOT_SIZE,PROT_READ| PROT_WRITE,
                MAP_SHARED,fd,0);

    struct nvs_root *root = (struct nvs_root *)addr;
    root->length = 0;
    _clflush((uintptr_t *)&(root->length));
    close(fd);
    std::cout << "successfully created the root heap";

    return 0;
}