#pragma once
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include "cachelib/allocator/CacheAllocator.h"
#include "cascade.hpp"
#include "object.hpp"

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

// configuration
#define CASCADE_HOST_CACHE_SIZE_CONF "CASCADE/max_host_cache_size"
#define CASCADE_HOST_CACHE_DEFAULT_SIZE (1024 * 1024 * 1024 * 1) // 1 GB

#define CACHELIB_BUCKET_POWER 25
#define CACHELIB_LOCK_POWER 10

namespace derecho {
namespace cascade {
    // cache on main system memory
    class CascadeHostCache {
        private:

        std::unique_ptr<CacheLibType> cache; // CacheLib cache
        CacheLibPoolId cache_default_pool; // CacheLib default pool

        public:

        CascadeHostCache();
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

        std::unique_ptr<CascadeHostCache> host_cache; // cache on main memory
        std::unique_ptr<CascadeDeviceCache> device_cache; // cache on gpu memory

        // map for registering which versions are cached and where
        std::unordered_map<std::string,std::tuple<std::unordered_set<persistent::version_t>,int>> version_location_map;
        std::mutex map_mtx; // lock needed to manipulate the map

        public:

        CascadeCache();
        ~CascadeCache();

        // put a Cascade object in the specified cache (host or device)
        void put(const ObjectWithStringKey&,int=CASCADE_CACHE_LOCATION_HOST);

        // get a reconstructed Cascade object
        const ObjectWithStringKey get(std::string,persistent::version_t=CURRENT_VERSION);

        // check whether an object (of a given version, or the latest known
        // version if not specified) is cached or not.
        bool is_cached(std::string,persistent::version_t=CURRENT_VERSION,persistent::version_t* = nullptr,int* = nullptr);

        // move an object from one cache to another
        bool move(std::string,int);
    };

} // cascade
} // derecho

