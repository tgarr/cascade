
#include <algorithm>
#include "cascade/cache.hpp"

namespace derecho {
namespace cascade {

    // CascadeHostCache methods

    CascadeHostCache::CascadeHostCache(size_t size,unsigned int bucket_power,unsigned int lock_power){
        CacheLibConfig config;
        config
            .setCacheSize(size)
            .setCacheName(CACHELIB_NAME)
            .setAccessConfig({bucket_power, lock_power})
            .validate();
        cache = std::make_unique<CacheLibType>(config);
        cache_default_pool = cache->addPool("default_pool",cache->getCacheMemoryStats().cacheSize);
    }

    CascadeHostCache::~CascadeHostCache(){
        cache.reset();
    }

    std::tuple<const void*,size_t> CascadeHostCache::get(std::string key){
        auto handle = cache->find(key);
        if(!handle){
            return std::make_tuple(CASCADE_CACHE_MISS,0);
        }

        return std::make_tuple(handle->getMemory(),handle->getSize());
    }

    bool CascadeHostCache::put(std::string key,const void *data,size_t size){
        auto handle = cache->allocate(cache_default_pool,key,size);
        if (!handle){
            return false;
        }
        
        std::memcpy(handle->getMemory(),data,size);
        cache->insertOrReplace(handle);
        
        return true;
    }

    bool CascadeHostCache::is_cached(std::string key){
        auto handle = cache->find(key);
        if(!handle) return false;
        return true;
    }

    // CascadeCache methods

    CascadeCache::CascadeCache(){
        host_cache = std::make_unique<CascadeHostCache>(CACHELIB_SIZE,CACHELIB_BUCKET_POWER,CACHELIB_LOCK_POWER);
        device_cache = std::make_unique<CascadeDeviceCache>();
    }

    CascadeCache::~CascadeCache(){
        host_cache.reset();
        device_cache.reset();
        version_location_map.clear();
    }

    void CascadeCache::put(const PlaceHolderObject &obj,int location){
        // TODO location: put in the right location. Furthermore, if the new
        // location is different from thw location of previously cached
        // versions, cache the new one in the new location and remove the rest

        bool do_cache = false;

        if(version_location_map.count(obj.key) > 0){
            auto &item = version_location_map[obj.key];
            if(std::get<1>(item) != location){
                // TODO remove all version from the old location
                do_cache = true;
            }

            if(obj.version != CURRENT_VERSION){
                auto &version_set = std::get<0>(item);
                if(version_set.count(obj.version) == 0){
                    do_cache = true;
                }
            }
            else {
                do_cache = true;
            }
        }
        else {
            do_cache = true;
        }

        if(do_cache){
            std::string cache_key = obj.key;
            if(obj.version != CURRENT_VERSION){
                cache_key += "::" + std::to_string(obj.version);
            }

            if(host_cache->put(cache_key,obj.data,obj.size)){
                map_mtx.lock();

                if(version_location_map.count(obj.key) == 0){
                    std::unordered_set<persistent::version_t> version_set;
                    version_location_map[obj.key] = std::make_tuple(version_set,location);
                }

                auto &item = version_location_map[obj.key];
                std::get<0>(item).insert(obj.version);

                map_mtx.unlock();
            }
        }
    }

    const PlaceHolderObject CascadeCache::get(std::string key,persistent::version_t version){
        PlaceHolderObject obj;
        obj.key = key;
        persistent::version_t version_found;
        int location; // TODO

        if(is_cached(key,version,&version_found,&location)){
            std::string cache_key = key;
            if(version_found != CURRENT_VERSION){
                cache_key += "::" + std::to_string(version_found);
            }
            
            auto ret = host_cache->get(cache_key);
            if(std::get<0>(ret) != CASCADE_CACHE_MISS){
                obj.version = version_found;
                obj.data = std::get<0>(ret);
                obj.size = std::get<1>(ret);

                return obj;
            }
        }
        
        obj.version = CURRENT_VERSION;
        obj.data = CASCADE_CACHE_MISS;
        obj.size = 0;

        return obj;
    }

    bool CascadeCache::is_cached(std::string key,persistent::version_t version,persistent::version_t *version_found,int *location_found){
        bool found = false;
        
        map_mtx.lock();
        if(version_location_map.count(key) > 0){
            auto &item = version_location_map[key];
            auto &version_set = std::get<0>(item);
            int location = std::get<1>(item); // TODO
            std::string cache_key = key;

            if(version == CURRENT_VERSION || (version_set.count(version) > 0)){
                if(version == CURRENT_VERSION){
                    std::unordered_set<persistent::version_t>::iterator it;
                    it = std::max_element(version_set.begin(),version_set.end());
                    version = *it;
                }

                if(version != CURRENT_VERSION){
                    cache_key += "::" + std::to_string(version);
                }
            
                found = host_cache->is_cached(cache_key);
                if(!found){
                    version_set.erase(version);
                    if(version_set.empty()){
                        version_location_map.erase(key);
                    }
                }
                else {
                    if(version_found != nullptr){
                        *version_found = version;
                    }
                    if(location_found != nullptr){
                        *location_found = location;
                    }
                }
            }
        }
        map_mtx.unlock();

        return found;
    }

    bool CascadeCache::move(std::string key,int location){
        // TODO move location
        return false;
    }


} // cascade
} // derecho

