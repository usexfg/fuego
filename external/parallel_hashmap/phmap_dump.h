#if !defined(phmap_dump_h_guard_)
#define phmap_dump_h_guard_

// ---------------------------------------------------------------------------
// Copyright (c) 2019, Gregory Popovitch - greg7mdp@gmail.com
//
//       providing dump/load/mmap_load
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ---------------------------------------------------------------------------

#include <iostream>
#include <fstream>
#include <sstream>
#include "phmap.h"
namespace phmap
{

namespace type_traits_internal {

#if defined(__GLIBCXX__) && __GLIBCXX__ < 20150801
    template<typename T> struct IsTriviallyCopyable : public std::integral_constant<bool, __has_trivial_copy(T)> {};
#else
    template<typename T> struct IsTriviallyCopyable : public std::is_trivially_copyable<T> {};
#endif

template <class T1, class T2>
struct IsTriviallyCopyable<std::pair<T1, T2>> {
    static constexpr bool value = IsTriviallyCopyable<T1>::value && IsTriviallyCopyable<T2>::value;
};
}

namespace container_internal {

// ------------------------------------------------------------------------
// dump/load for raw_hash_set
// ------------------------------------------------------------------------
template <typename Key, typename Hash = std::hash<Key>>
bool dump_unordered_set(const std::unordered_set<Key, Hash>& set, BinaryOutputArchive& ar) {
    // Simple implementation for std::unordered_set - just serialize the size and elements
    size_t size = set.size();
    if (!ar.dump(size)) {
        std::cerr << "Failed to dump set size" << std::endl;
        return false;
    }

    for (const auto& item : set) {
        if (!ar.dump(item)) {
            std::cerr << "Failed to dump set item" << std::endl;
            return false;
        }
    }
    return true;
}

// Load function for std::unordered_set
template <typename Key, typename Hash = std::hash<Key>>
bool load_unordered_set(std::unordered_set<Key, Hash>& set, BinaryInputArchive& ar) {
    set.clear();
    size_t size;
    if (!ar.load(size)) {
        std::cerr << "Failed to load set size" << std::endl;
        return false;
    }

    for (size_t i = 0; i < size; ++i) {
        Key item;
        if (!ar.load(item)) {
            std::cerr << "Failed to load set item" << std::endl;
            return false;
        }
        set.insert(item);
    }
    return true;
}

// ------------------------------------------------------------------------
// dump/load for unordered_map (used instead of parallel_hash_map)
// ------------------------------------------------------------------------
template <typename Key, typename Value, typename Hash = std::hash<Key>>
bool dump_unordered_map(const std::unordered_map<Key, Value, Hash>& map, BinaryOutputArchive& ar) {
    // Simple implementation for std::unordered_map - just serialize the size and key-value pairs
    size_t size = map.size();
    if (!ar.dump(size)) {
        std::cerr << "Failed to dump map size" << std::endl;
        return false;
    }

    for (const auto& pair : map) {
        if (!ar.dump(pair.first) || !ar.dump(pair.second)) {
            std::cerr << "Failed to dump map item" << std::endl;
            return false;
        }
    }
    return true;
}

template <typename Key, typename Value, typename Hash = std::hash<Key>>
bool load_unordered_map(std::unordered_map<Key, Value, Hash>& map, BinaryInputArchive& ar) {
    map.clear();
    size_t size;
    if (!ar.load(size)) {
        std::cerr << "Failed to load map size" << std::endl;
        return false;
    }

    for (size_t i = 0; i < size; ++i) {
        Key key;
        Value value;
        if (!ar.load(key) || !ar.load(value)) {
            std::cerr << "Failed to load map item" << std::endl;
            return false;
        }
        map[key] = value;
    }
    return true;
}

} // namespace container_internal



// ------------------------------------------------------------------------
// BinaryArchive
//       File is closed when archive object is destroyed
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
class BinaryOutputArchive {
public:
    BinaryOutputArchive(const char *file_path) {
        ofs_.open(file_path, std::ios_base::binary);
    }

    bool dump(const char *p, size_t sz) {
        ofs_.write(p, sz);
        return true;
    }

    template<typename V>
    typename std::enable_if<type_traits_internal::IsTriviallyCopyable<V>::value, bool>::type
    dump(const V& v) {
        ofs_.write(reinterpret_cast<const char *>(&v), sizeof(V));
        return true;
    }

private:
    std::ofstream ofs_;
};


class BinaryInputArchive {
public:
    BinaryInputArchive(const char * file_path) {
        ifs_.open(file_path, std::ios_base::binary);
    }

    bool load(char* p, size_t sz) {
        ifs_.read(p, sz);
        return true;
    }

    template<typename V>
    typename std::enable_if<type_traits_internal::IsTriviallyCopyable<V>::value, bool>::type
    load(V* v) {
        ifs_.read(reinterpret_cast<char *>(v), sizeof(V));
        return true;
    }

private:
    std::ifstream ifs_;
};

} // namespace phmap

#endif // phmap_dump_h_guard_
