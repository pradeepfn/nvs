//
// Created by pradeep on 8/27/17.
//

#ifndef YUMA_RUNTIMEMANAGER_H
#define YUMA_RUNTIMEMANAGER_H


namespace nvs{



    class Version{


    public:
    };

    /*
     * Key
     */
    class Key{

    private:
        gptr *ptr;
        uint64_t size;
        /* bit field to keep track of modified pages of the buffer */
        Byte *barry;
        /*vector of versions */


    public:
        Version *getVersion(uint64_t version);
        uint64_t getSize();

    };



    class Root{
    private:
    protected:
    public:
        void *create_obj(std::string key, uint64_t version);
        int put_obj(std::string key, uint64_t version);
        int get_obj(std::string key, uint64_t version, std::string range);
    };






    class RuntimeManager{
    private:
        static std::atomic<RuntimeManager *> instance_;
        static std::mutex mutex_;

        RuntimeManager();
        ~RuntimeManager();

        class Impl_;
        std::unique_ptr<Impl_> privateImpl;

    protected:
    public:
        static RuntimeManager *getInstance();
        /*
         * Root of meta-data structures associated with this workflow
         */
        int createStore(std::string rootId);

        /*
         *  get hold of root structure
         */
        Root findSore(std::string rootId, Root *root);


    };


}


#endif //YUMA_RUNTIMEMANAGER_H
