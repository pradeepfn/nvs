#include <nvs/log.h>
#include <boost/filesystem/operations.hpp>
#include "file_store.h"

namespace nvs{


    FileStore::FileStore(std::string storeId):storeId(storeId){
        boost::filesystem::path fsPath = boost::filesystem::path(ROOT_FILE_PATH);
        if(!boost::filesystem::exists(fsPath)){
            bool ret = boost::filesystem::create_directory(fsPath);
            if (ret == false)
            {
                LOG(fatal) << "FileStore: Failed to create ROOT_FILE_PATH"
                           << ROOT_FILE_PATH;
                exit(1);
            }
        }
    }

    ErrorCode FileStore::put(std::string key, uint64_t version) {

        std::map<std::string, Object *>::iterator it;
        // first traverse the map and find the key object
        if((it = objectMap.find(key)) != objectMap.end()) {

            Object *obj = it->second;
            // file name
            std::string file_name = std::string(ROOT_FILE_PATH) + "/" + this->storeId +
                                    key + std::to_string(version);

            FILE *file = fopen(file_name.c_str(), "w");
            if (file == NULL) {
                LOG(fatal) << "FileStore: file open failed";
                exit(1);
            }
            size_t tsize = fwrite(obj->getPtr(),sizeof(char),
                                  obj->getSize(),file);
            assert(tsize == obj->getSize());

            fsync(fileno(file));
            fclose(file);
            return NO_ERROR;
        }
        LOG(error) << "FileStore: key not found";
        return ELEM_NOT_FOUND;
    }

    ErrorCode FileStore::put_all() {
        std::map<std::string, Object *>::iterator it;
        // first traverse the map and find the key object
        for(it=objectMap.begin(); it!=objectMap.end(); it++) {

            Object *obj = it->second;
            std::string key = it->first;
            uint64_t version = obj->getVersion()+1;
            // file name
            std::string file_name = std::string(ROOT_FILE_PATH) + "/" + this->storeId +
                                    key + std::to_string(version);

            FILE *file = fopen(file_name.c_str(), "w");
            if (file == NULL) {
                LOG(fatal) << "FileStore: file open failed";
                exit(1);
            }
            size_t tsize = fwrite(obj->getPtr(),sizeof(char),
                                  obj->getSize(),file);
            assert(tsize == obj->getSize());

            fsync(fileno(file));
            fclose(file);
            obj->setVersion(version);
            return NO_ERROR;
        }
        LOG(error) << "FileStore: key not found";
        return ELEM_NOT_FOUND;
    }

    ErrorCode FileStore::get(std::string key, uint64_t version, void *addr) {

            // file name
            std::string file_name = std::string(ROOT_FILE_PATH) + "/" + this->storeId +
                                    key + std::to_string(version);

            //find the size of the file
            boost::filesystem::path path(file_name);
            boost::system::error_code ec;
            boost::uintmax_t  filesize = boost::filesystem::file_size(path, ec);
        if(ec){
            LOG(fatal) << "FileStore: file size error";
            return ELEM_NOT_FOUND;

        }

            FILE *file = fopen(file_name.c_str(), "r");
            if (file == NULL) {
                LOG(fatal) << "FileStore: file open failed";
                return ELEM_NOT_FOUND;
            }

            size_t tsize = fread(addr,sizeof(char),
                                  filesize,file);
            assert(tsize == filesize);
            fclose(file);
            return NO_ERROR;

    }

    ErrorCode FileStore::create_obj(std::string key, uint64_t size, void **obj_addr) {

        void *tmp_ptr = malloc(size);
        Object *obj = new Object(key,size,0,tmp_ptr);
        std::map<std::string, Object *>::iterator it =   objectMap.find(key);
        if(it == objectMap.end()){
            objectMap[key] = obj;
        }else{
            LOG(fatal) << "not implemented yet";
            exit(1);
        }
        *obj_addr = (uint64_t *)tmp_ptr;
        return NO_ERROR;
    }


    void FileStore::stats() {

    }
}
