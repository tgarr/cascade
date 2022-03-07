
#include <cascade/service_types.hpp>
#include <unordered_map>
#include <string>
#include <iostream>

using namespace derecho::cascade;

using opm_t = ObjectPoolMetadata<VolatileCascadeStoreWithStringKey>;

#define NUM_OBJECTS 10000
#define NUM_AFSETS 10
#define NUM_SHARDS 3

int main(int argc, char** argv) {
    std::cout << "Number of objects: " << NUM_OBJECTS << std::endl;
    std::cout << "Number of affinity sets: " << NUM_AFSETS << std::endl;
    std::cout << "Number of shards: " << NUM_SHARDS << std::endl << std::endl;

    // divide objects into affinity sets
    std::unordered_map<std::string,std::string> afsets;
    for(int i=0; i<NUM_OBJECTS; i++)
        afsets[std::to_string(i)] = std::to_string(i % NUM_AFSETS);

    // object pool, with and without affinity sets
    opm_t opm1("",0,0,HASH,{},false,{});
    opm_t opm2("",0,0,HASH,{},false,afsets);

    // count the division of objects among shards
    int shard_count1[NUM_SHARDS], shard_count2[NUM_SHARDS];
    for(int i=0; i<NUM_SHARDS; i++){
        shard_count1[i] = 0;
        shard_count2[i] = 0;
    }

    // count and check the resulting shards for the objects
    std::unordered_map<std::string,uint32_t> expected_shard;
    int afset_error_count = 0;
    for(int i=0; i<NUM_OBJECTS; i++){
        std::string key = std::to_string(i);
        std::string afset = std::to_string(i % NUM_AFSETS);

        // without afsets
        uint32_t shard = opm1.key_to_shard_index(key,NUM_SHARDS);
        shard_count1[shard]++;

        // with afsets
        shard = opm2.key_to_shard_index(key,NUM_SHARDS);
        shard_count2[shard]++;
        if(expected_shard.find(afset) == expected_shard.end())
            expected_shard[afset] = shard;
        else 
            if(expected_shard[afset] != shard) afset_error_count++;
    }

    // summary
    std::cout << "Without affinity sets:" << std::endl;
    for(int i=0; i<NUM_SHARDS; i++){
        std::cout << "  Shard " << i << ": " << shard_count1[i] << " objects" << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "With affinity sets:" << std::endl;
    for(int i=0; i<NUM_SHARDS; i++){
        std::cout << "  Shard " << i << ": " << shard_count2[i] << " objects" << std::endl;
    }
    std::cout << std::endl;

    // placement
    std::cout << "Affinity sets placement errors: " << afset_error_count << std::endl;
    for(int i=0; i<NUM_AFSETS; i++){
        std::string afset = std::to_string(i);
        std::cout << "  AFSET " << afset << ": shard " << expected_shard[afset] << std::endl;
    }
   
    return 0;
}
