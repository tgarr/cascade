#include <cascade/cascade.hpp>
#include <cascade/object.hpp>
#include <cascade/service.hpp>
#include <cascade/service_types.hpp>
#include <derecho/conf/conf.hpp>
#include <derecho/utils/logger.hpp>
#include <dlfcn.h>
#include <sys/prctl.h>

#include "server.hpp"

#define PROC_NAME "cascadechain_s"

namespace derecho::cascade {
// specialize create_null_object_cb for Cascade Types...
using opm_t = ObjectPoolMetadata<PersistentCascadeStoreWithStringKey, SignatureCascadeStoreWithStringKey>;
template <>
opm_t create_null_object_cb<std::string, opm_t, &opm_t::IK, &opm_t::IV>(const std::string& key) {
    opm_t opm;
    opm.pathname = key;
    opm.subgroup_type_index = opm_t::invalid_subgroup_type_index;
    return opm;
}
}  // namespace derecho::cascade

using namespace derecho::cascade;

int main(int argc, char** argv) {
    // set proc name
    if(prctl(PR_SET_NAME, PROC_NAME, 0, 0, 0) != 0) {
        dbg_default_warn("Cannot set proc name to {}.", PROC_NAME);
    }

    CascadeServiceCDPO<PersistentCascadeStoreWithStringKey, ChainContextType> cdpo_pcss;
    CascadeServiceCDPO<SignatureCascadeStoreWithStringKey, ChainContextType> cdpo_scss;

    auto meta_factory = [](persistent::PersistentRegistry* pr, derecho::subgroup_id_t, ICascadeContext* context_ptr) {
        // critical data path for metadata service is currently disabled. But we can leverage it later for object pool
        // metadata handling.
        return std::make_unique<CascadeMetadataService<PersistentCascadeStoreWithStringKey, SignatureCascadeStoreWithStringKey>>(
                pr, nullptr, context_ptr);
    };
    auto pcss_factory = [&cdpo_pcss](persistent::PersistentRegistry* pr, derecho::subgroup_id_t,
                                     ICascadeContext* context_ptr) {
        return std::make_unique<PersistentCascadeStoreWithStringKey>(pr, &cdpo_pcss, context_ptr);
    };
    auto scss_factory = [&cdpo_scss](persistent::PersistentRegistry* pr, derecho::subgroup_id_t,
                                     ICascadeContext* context_ptr) {
        return std::make_unique<SignatureCascadeStoreWithStringKey>(pr, &cdpo_scss, context_ptr);
    };
    dbg_default_trace("starting service...");
    ChainServiceType::start(
            {&cdpo_pcss, &cdpo_scss},
            meta_factory,
            pcss_factory, scss_factory);
    dbg_default_trace("started service, waiting till it ends.");
    std::cout << "Press Enter to Shutdown." << std::endl;
    std::cin.get();
    // wait for service to quit.
    ChainServiceType::shutdown(false);
    dbg_default_trace("shutdown service gracefully");
    // you can do something here to parallel the destructing process.
    ChainServiceType::wait();
    dbg_default_trace("Finish shutdown.");

    return 0;
}