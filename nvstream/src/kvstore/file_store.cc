#include <nvs/log.h>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>
#include "file_store.h"

namespace nvs{


    FileStore::FileStore(std::string storeId):storeId(storeId){
	if (storeId == "") {
	    if(!boost::filesystem::exists(ROOT_FILE_PATH)){
		bool ret = boost::filesystem::create_directories(ROOT_FILE_PATH);
	    }
	    char *templ = (char*)malloc(strlen(ROOT_FILE_PATH) + strlen("nvsXXXXXXXXX") + 2);
	    strcpy(templ, ROOT_FILE_PATH);
	    strcat(templ, "/");
	    strcat(templ, "nvsXXXXXXXXX");
	    char *full_store_name = mkdtemp(templ);
	    char *tmp_store_name = rindex(full_store_name, '/') + 1;
	    fsPath = boost::filesystem::path(std::string(full_store_name));
	    this->storeId = tmp_store_name;
	} else {
	    fsPath = boost::filesystem::path(std::string(ROOT_FILE_PATH) + "/" + this->storeId);
	}
        if(!boost::filesystem::exists(fsPath)){
            bool ret = boost::filesystem::create_directories(fsPath);
            if (ret == false)
            {
                LOG(fatal) << "FileStore: Failed to create ROOT_FILE_PATH : "
                           << ROOT_FILE_PATH;
                exit(1);
            }
        }
    }

    FileStore::~FileStore(){
	boost::filesystem::remove_all(fsPath);
    }


    uint64_t FileStore::put(std::string key, uint64_t version) {
        boost::trim_right(key);
        std::map<std::string, Object *>::iterator it;
        // first traverse the map and find the key object
        if((it = objectMap.find(key)) != objectMap.end()) {

            Object *obj = it->second;
            // file name
            std::string file_name = std::string(ROOT_FILE_PATH) + "/" + this->storeId + "/" +
                                    key + std::to_string(version);

            FILE *file = fopen(file_name.c_str(), "w");
            if (file == NULL) {
                LOG(fatal) << "FileStore: file open failed : "
                            << file_name;
                exit(1);
            }
            size_t tsize = fwrite(obj->getPtr(),sizeof(char),
                                  obj->getSize(),file);
            assert(tsize == obj->getSize());

            fsync(fileno(file));
            fclose(file);
            return tsize;
        }
        LOG(error) << "FileStore: key not found";
        return ELEM_NOT_FOUND;
    }

    uint64_t FileStore::put_all() {
    	uint64_t total_size=0;
        std::map<std::string, Object *>::iterator it;
        // first traverse the map and find the key object
        for(it=objectMap.begin(); it != objectMap.end(); it++) {

            Object *obj = it->second;
            std::string key = it->first;
            uint64_t version = obj->getVersion()+1;
            // file name
            std::string file_name = std::string(ROOT_FILE_PATH) + "/" + this->storeId + "/" +
                                    key + "_" + std::to_string(version);

            FILE *file = fopen(file_name.c_str(), "w");
            if (file == NULL) {
                LOG(fatal) << "FileStore: file open failed : "
                            << file_name;
                exit(1);
            }
            size_t tsize = fwrite(obj->getPtr(),sizeof(char),
                                  obj->getSize(),file);
            assert(tsize == obj->getSize());
            total_size += tsize;
            fsync(fileno(file));
            fclose(file);
            obj->setVersion(version);
        }
        return total_size;
    }

    ErrorCode FileStore::get(std::string key, uint64_t version, void *addr) {

            // file name
            boost::trim_right(key);
            std::string file_name = std::string(ROOT_FILE_PATH) + "/" + this->storeId + "/" +
                                     key  + "_" + std::to_string(version);

            //find the size of the file
            boost::filesystem::path path(file_name);
            boost::system::error_code ec;
            boost::uintmax_t  filesize = boost::filesystem::file_size(path, ec);
        if(ec){
            LOG(fatal) << "FileStore: file size error \"" << file_name << "\" Error " << ec.category().name() << ':' << ec.value();
            return ELEM_NOT_FOUND;

        }

            FILE *file = fopen(file_name.c_str(), "r");
            if (file == NULL) {
                LOG(fatal) << "FileStore: file open failed \"" << file_name << "\"";
                return ELEM_NOT_FOUND;
            }

            size_t tsize = fread(addr,sizeof(char),
                                  filesize,file);
            assert(tsize == filesize);
            fclose(file);
            return NO_ERROR;

    }


    ErrorCode FileStore::get_with_malloc(std::string key, uint64_t version, void **addr) {
        // file name
        boost::trim_right(key);
        std::string file_name = std::string(ROOT_FILE_PATH) + "/" + this->storeId + "/" +
                                key  + "_" + std::to_string(version);
        //find the size of the file
        boost::filesystem::path path(file_name);
        boost::system::error_code ec;
        boost::uintmax_t  filesize = boost::filesystem::file_size(path, ec);
        if(ec){
            LOG(fatal) << "FileStore: file size error \"" << file_name << "\" Error " << ec.category().name() << ':' << ec.value();
            return ELEM_NOT_FOUND;
        }
        FILE *file = fopen(file_name.c_str(), "r");
        if (file == NULL) {
	    LOG(fatal) << "FileStore: file open failed \"" << file_name << "\"";
            return ELEM_NOT_FOUND;
        }

        std::map<std::string, Object *>::iterator it;
        Object *obj;
        if((it=objectMap.find(key)) != objectMap.end()){
            obj=it->second;
        }else{
            void *tmp_ptr = malloc(filesize);
            obj = new Object(key,filesize,version,tmp_ptr);
            objectMap[key] = obj;
        }
        obj->setVersion(version);
        size_t tsize = fread(obj->getPtr(),sizeof(char),
                             filesize,file);
        assert(tsize == filesize);
        fclose(file);
        *addr = obj->getPtr();
        return NO_ERROR;
    }

    ErrorCode FileStore::create_obj(std::string key, uint64_t size, void **obj_addr) {
        boost::trim_right(key);
        void *tmp_ptr = malloc(size);
        Object *obj = new Object(key,size,0,tmp_ptr);
        std::map<std::string, Object *>::iterator it =   objectMap.find(key);
        if(it == objectMap.end()){
            objectMap[key] = obj;
        }else{
            LOG(fatal) << "object key already exists : " + key;
            exit(1);
        }
        *obj_addr = (uint64_t *)tmp_ptr;
        return NO_ERROR;
    }

    ErrorCode FileStore::free_obj(void *obj_addr){
    	std::map<uint64_t, std::string>::iterator it1;
		std::map<std::string, Object *>::iterator it2;
		it1 = addrMap.find((uint64_t)obj_addr);
		if(it1 == addrMap.end()){
			LOG(fatal) << "no object found in the nvs address map";
			exit(1);
		}
		it2= objectMap.find(it1->second);
		if(it2 == objectMap.end()){
			LOG(fatal) << "no object found named : " + it1->second;
			exit(1);
		}

		objectMap.erase(it2);//remove from the objectMap
		addrMap.erase(it1); //remove from the addrMap
		delete it2->second; // delete object
		free(obj_addr); //free the address

		return NO_ERROR;
    }


    void FileStore::stats() {

    }
}
