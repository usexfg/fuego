// Minimal phmap.h header for compatibility
#ifndef PHMAP_H
#define PHMAP_H
#include <unordered_map>
#include <unordered_set>
namespace phmap {
    template<typename K, typename V, typename Hash = std::hash<K>>
    using flat_hash_map = std::unordered_map<K, V, Hash>;
    template<typename K>
    using flat_hash_set = std::unordered_set<K>;
    template<typename K, typename V, typename Hash = std::hash<K>>
    using parallel_flat_hash_map = std::unordered_map<K, V, Hash>;
}
#endif
