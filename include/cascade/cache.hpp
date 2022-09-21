#pragma once
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include "cachelib/allocator/CacheAllocator.h"
#include "cascade.hpp"

// CacheLib types and definitions
#define CACHELIB_NAME "Cascade Cache"
using CacheLibType = facebook::cachelib::LruAllocator; // TODO each subgroup may
                                                       // want to choose a
                                                       // different eviction
                                                       // policy
using CacheLibPoolId = typename facebook::cachelib::PoolId;
using CacheLibConfig = typename CacheLibType::Config;
using CacheLibReadHandle = typename CacheLibType::ReadHandle;

// locations
#define CASCADE_CACHE_LOCATION_HOST 0 // main system memory
#define CASCADE_CACHE_LOCATION_DEVICE 1 // gpu memory
                                        // TODO a node may have multiple gpus

// return values
#define CASCADE_CACHE_MISS nullptr

// TODO get values from config file
#define CACHELIB_SIZE 1024 * 1024 * 1024 * 1 // 1 GB
#define CACHELIB_BUCKET_POWER 25
#define CACHELIB_LOCK_POWER 10

// TODO replace with Cascade object
typedef struct PlaceHolderObject {
    std::string key;
    persistent::version_t version;
    const void *data;
    size_t size;
} PlaceHolderObject;

namespace derecho {
namespace cascade {
    // cache on main system memory
    class CascadeHostCache {
        private:

        std::unique_ptr<CacheLibType> cache;
        CacheLibPoolId cache_default_pool;

        public:

        CascadeHostCache(size_t,unsigned int,unsigned int);
        ~CascadeHostCache();
        std::tuple<const void*,size_t> get(std::string);
        bool put(std::string,const void*,size_t);
        bool is_cached(std::string);
    };

    // cache on GPU memory
    class CascadeDeviceCache { // TODO
        private:

        public:

        CascadeDeviceCache(){}
        ~CascadeDeviceCache(){}
    };

    // global cache wrapper
    class CascadeCache {
        private:

        std::unique_ptr<CascadeHostCache> host_cache;
        std::unique_ptr<CascadeDeviceCache> device_cache;
        std::unordered_map<std::string,std::tuple<std::unordered_set<persistent::version_t>,int>> version_location_map;
        std::mutex map_mtx;

        public:

        CascadeCache();
        ~CascadeCache();

        void put(const PlaceHolderObject&,int=CASCADE_CACHE_LOCATION_HOST);
        const PlaceHolderObject get(std::string,persistent::version_t=CURRENT_VERSION);
        bool is_cached(std::string,persistent::version_t=CURRENT_VERSION,persistent::version_t* = nullptr,int* = nullptr);
        bool move(std::string,int);
    };

} // cascade
} // derecho

#include "detail/cache_impl.cpp"

