// ============================================================================
// CannonCruise - level resource registry (RwsResources.cpp)
// ============================================================================

#include "RwsResources.h"

#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace {

struct Entry {
    RwsResource info;
    std::vector<RwUInt8> bytes;
};

// Keyed by the raw 16 GUID bytes.
std::map<std::string, Entry> &Table() {
    static std::map<std::string, Entry> s_table;
    return s_table;
}

std::string Key(const RwUInt8 guid[16]) {
    return std::string(reinterpret_cast<const char *>(guid), 16);
}

} // namespace

void RwsResources_Clear(void) { Table().clear(); }

RwBool RwsResources_Add(const RwUInt8 guid[16], RwUInt32 embeddedType,
                        const RwChar *pName, const void *pData,
                        RwUInt32 size) {
    if (!guid || !pData || size == 0) {
        return FALSE;
    }
    std::map<std::string, Entry> &table = Table();
    const std::string key = Key(guid);
    if (table.find(key) != table.end()) {
        return FALSE;
    }
    Entry &e = table[key];
    e.bytes.assign(static_cast<const RwUInt8 *>(pData),
                   static_cast<const RwUInt8 *>(pData) + size);
    std::memset(&e.info, 0, sizeof(e.info));
    std::memcpy(e.info.guid, guid, 16);
    e.info.embeddedType = embeddedType;
    if (pName) {
        std::strncpy(e.info.name, pName, sizeof(e.info.name) - 1);
    }
    // std::map nodes never move, so the pointer stays valid until Clear().
    e.info.pData = &e.bytes[0];
    e.info.size = size;
    return TRUE;
}

const RwsResource *RwsResources_Find(const RwUInt8 guid[16]) {
    if (!guid) {
        return 0;
    }
    std::map<std::string, Entry> &table = Table();
    std::map<std::string, Entry>::const_iterator it = table.find(Key(guid));
    return it == table.end() ? 0 : &it->second.info;
}

const RwsResource *RwsResources_FindByName(const RwChar *pName) {
    if (!pName) {
        return 0;
    }
    std::map<std::string, Entry> &table = Table();
    for (std::map<std::string, Entry>::const_iterator it = table.begin();
         it != table.end(); ++it) {
        if (std::strcmp(it->second.info.name, pName) == 0) {
            return &it->second.info;
        }
    }
    return 0;
}

RwUInt32 RwsResources_Count(void) { return (RwUInt32)Table().size(); }
