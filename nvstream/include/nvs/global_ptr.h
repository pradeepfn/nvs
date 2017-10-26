
#ifndef NVS_GLOBAL_PTR_H_H
#define NVS_GLOBAL_PTR_H_H

#include <cstdint>
#include <ostream>

#include "pool_id.h"

namespace  nvs{

    // NVStream GlobalPtr consists of two parts: pool ID of the log, and offset
    template<class GlobalPtrT, size_t GlobalPtrTSize, class PoolIdT, size_t PoolIdTSize,class OffsetT, size_t OffsetTSize>
    class GlobalPtrClass
    {
        static_assert(sizeof(GlobalPtrT)== 8, "Invalid GlobalPtrT");
        static_assert(sizeof(PoolIdT)*8 >= PoolIdTSize, "Invalid PoolIdT");
        static_assert(sizeof(OffsetT)*8 >= OffsetTSize, "Invalid OffsetT");
        static_assert(PoolIdTSize+OffsetTSize <= GlobalPtrTSize, "Invalid sizes");
        static_assert(GlobalPtrTSize<=64, "GlobalPtr is at most 8 bytes"); // must fit into one uint64_t

    public:
        GlobalPtrClass()
                : global_ptr_{EncodeGlobalPtr(0, 0)}
        {
        }

        GlobalPtrClass(GlobalPtrT global_ptr)
                : global_ptr_{global_ptr}
        {
        }

        GlobalPtrClass(PoolIdT pool_id, OffsetT offset)
                : global_ptr_{EncodeGlobalPtr(pool_id, offset)}
        {
        }


        ~GlobalPtrClass()
        {

        }

        operator uint64_t() const
        {
            return (uint64_t)global_ptr_;
        }

        inline bool IsValid() const
        {
            return GetPoolId().IsValid() && IsValidOffset(GetOffset());
        }

        inline static bool IsValidOffset(OffsetT offset)
        {
            return offset != 0;
        }

        inline PoolIdT GetPoolId() const
        {
            return DecodePoolId(global_ptr_);
        }

        inline OffsetT GetOffset() const
        {
            return DecodeOffset(global_ptr_);
        }


        inline uint64_t ToUINT64() const
        {
            return (uint64_t)global_ptr_;
        }

        // operators
        friend std::ostream& operator<<(std::ostream& os, const GlobalPtrClass& global_ptr)
        {
            os << "[" << global_ptr.GetPoolId()
               << ":" << global_ptr.GetOffset() << "]";
            return os;
        }

    private:
        static int const kPoolIdShift = OffsetTSize;

        inline GlobalPtrT EncodeGlobalPtr(PoolIdT pool_id, OffsetT offset) const
        {
            return ((GlobalPtrT)pool_id.GetPoolId() << (kPoolIdShift)) + offset;
        }

        inline PoolIdT DecodePoolId(GlobalPtrT global_ptr) const
        {
            return PoolIdT((PoolIdStorageType)(global_ptr >> kPoolIdShift));
        }

        inline OffsetT DecodeOffset(GlobalPtrT global_ptr) const
        {
            OffsetT offset = (OffsetT)((((GlobalPtrT)1 << OffsetTSize) - 1) & global_ptr);
            return offset;
        }

        GlobalPtrT global_ptr_;
    };

// internal type of GlobalPtr
    using GlobalPtrStorageType = uint64_t;
// Offset
    using Offset = uint64_t;

// GlobalPtr
    using GlobalPtr = GlobalPtrClass<GlobalPtrStorageType, 64, PoolId, 8, Offset, 56>;


}

#endif //NVS_GLOBAL_PTR_H_H
