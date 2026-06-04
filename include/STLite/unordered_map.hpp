#ifndef SJTU_UNORDERED_MAP_HPP
#define SJTU_UNORDERED_MAP_HPP

#include "exceptions.hpp"
#include "map.hpp"
#include "utility.hpp"

#include <cstdio>

namespace sjtu {

template <class Key, class T, class Hash = sjtu::hash<Key>, class KeyEqual = sjtu::equal_to<Key>>
class unordered_map {
public:
    using value_type = pair<const Key, T>;

private:
    using bucket_type = map<Key, T>;

    bucket_type *buckets;
    size_t bucket_count;
    size_t _size;
    Hash hasher;
    KeyEqual equal;

    size_t bucket_index(const Key &key) const {
        if (bucket_count == 0) {
            return 0;
        }
        return hasher(key) % bucket_count;
    }

    void init(size_t count) {
        bucket_count = count == 0 ? 8 : count;
        buckets = new bucket_type[bucket_count];
        _size = 0;
    }

    void rehash(size_t new_bucket_count) {
        if (new_bucket_count < 8) {
            new_bucket_count = 8;
        }
        bucket_type *new_buckets = new bucket_type[new_bucket_count];
        for (size_t i = 0; i < bucket_count; ++i) {
            for (auto it = buckets[i].begin(); it != buckets[i].end(); ++it) {
                const size_t idx = hasher(it->first) % new_bucket_count;
                new_buckets[idx].insert(*it);
            }
        }
        delete[] buckets;
        buckets = new_buckets;
        bucket_count = new_bucket_count;
    }

    void ensure_capacity(size_t needed_size) {
        if (needed_size <= bucket_count * 2) {
            return;
        }
        rehash(bucket_count == 0 ? 8 : bucket_count * 2);
    }

public:
    class const_iterator;

    class iterator {
        friend class unordered_map;
        friend class const_iterator;
    private:
        unordered_map *owner;
        size_t bucket;
        typename bucket_type::iterator it;

    public:
        using difference_type = long long;
        using value_type = typename unordered_map::value_type;
        using pointer = value_type *;
        using reference = value_type &;
        using iterator_category = sjtu::bidirectional_iterator_tag;

        iterator(unordered_map *map_ptr = nullptr, size_t bucket_idx = 0,
                 typename bucket_type::iterator iter = typename bucket_type::iterator())
            : owner(map_ptr), bucket(bucket_idx), it(iter) {
        }

        iterator(const iterator &other) = default;

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        iterator &operator++() {
            if (owner == nullptr || bucket >= owner->bucket_count) {
                throw invalid_iterator();
            }
            ++it;
            while (bucket < owner->bucket_count && it == owner->buckets[bucket].end()) {
                ++bucket;
                if (bucket == owner->bucket_count) {
                    it = typename bucket_type::iterator();
                    return *this;
                }
                it = owner->buckets[bucket].begin();
            }
            return *this;
        }

        iterator operator--(int) {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }

        iterator &operator--() {
            if (owner == nullptr) {
                throw invalid_iterator();
            }
            if (bucket == owner->bucket_count) {
                if (owner->_size == 0) {
                    throw invalid_iterator();
                }
                size_t idx = owner->bucket_count;
                while (idx > 0) {
                    --idx;
                    if (!owner->buckets[idx].empty()) {
                        bucket = idx;
                        it = owner->buckets[idx].end();
                        --it;
                        return *this;
                    }
                }
                throw invalid_iterator();
            }
            if (it == owner->buckets[bucket].begin()) {
                size_t idx = bucket;
                while (idx > 0) {
                    --idx;
                    if (!owner->buckets[idx].empty()) {
                        bucket = idx;
                        it = owner->buckets[idx].end();
                        --it;
                        return *this;
                    }
                }
                throw invalid_iterator();
            }
            --it;
            return *this;
        }

        value_type &operator*() const {
            if (owner == nullptr || bucket >= owner->bucket_count || it == owner->buckets[bucket].end()) {
                throw invalid_iterator();
            }
            return *it;
        }

        value_type *operator->() const {
            if (owner == nullptr || bucket >= owner->bucket_count || it == owner->buckets[bucket].end()) {
                throw invalid_iterator();
            }
            return &(*it);
        }

        bool operator==(const iterator &rhs) const {
            return owner == rhs.owner && bucket == rhs.bucket && it == rhs.it;
        }

        bool operator==(const const_iterator &rhs) const;

        bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }

        bool operator!=(const const_iterator &rhs) const;
    };

    class const_iterator {
        friend class unordered_map;
        friend class iterator;
    private:
        const unordered_map *owner;
        size_t bucket;
        typename bucket_type::const_iterator it;

    public:
        using difference_type = long long;
        using value_type = typename unordered_map::value_type;
        using pointer = const value_type *;
        using reference = const value_type &;
        using iterator_category = sjtu::bidirectional_iterator_tag;

        const_iterator(const unordered_map *map_ptr = nullptr, size_t bucket_idx = 0,
                       typename bucket_type::const_iterator iter = typename bucket_type::const_iterator())
            : owner(map_ptr), bucket(bucket_idx), it(iter) {
        }

        const_iterator(const iterator &other) : owner(other.owner), bucket(other.bucket), it(other.it) {
        }

        const_iterator(const const_iterator &other) = default;

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        const_iterator &operator++() {
            if (owner == nullptr || bucket >= owner->bucket_count) {
                throw invalid_iterator();
            }
            ++it;
            while (bucket < owner->bucket_count && it == owner->buckets[bucket].end()) {
                ++bucket;
                if (bucket == owner->bucket_count) {
                    it = typename bucket_type::const_iterator();
                    return *this;
                }
                it = owner->buckets[bucket].begin();
            }
            return *this;
        }

        const_iterator operator--(int) {
            const_iterator tmp = *this;
            --(*this);
            return tmp;
        }

        const_iterator &operator--() {
            if (owner == nullptr) {
                throw invalid_iterator();
            }
            if (bucket == owner->bucket_count) {
                if (owner->_size == 0) {
                    throw invalid_iterator();
                }
                size_t idx = owner->bucket_count;
                while (idx > 0) {
                    --idx;
                    if (!owner->buckets[idx].empty()) {
                        bucket = idx;
                        it = owner->buckets[idx].end();
                        --it;
                        return *this;
                    }
                }
                throw invalid_iterator();
            }
            if (it == owner->buckets[bucket].begin()) {
                size_t idx = bucket;
                while (idx > 0) {
                    --idx;
                    if (!owner->buckets[idx].empty()) {
                        bucket = idx;
                        it = owner->buckets[idx].end();
                        --it;
                        return *this;
                    }
                }
                throw invalid_iterator();
            }
            --it;
            return *this;
        }

        const value_type &operator*() const {
            if (owner == nullptr || bucket >= owner->bucket_count || it == owner->buckets[bucket].end()) {
                throw invalid_iterator();
            }
            return *it;
        }

        const value_type *operator->() const {
            if (owner == nullptr || bucket >= owner->bucket_count || it == owner->buckets[bucket].end()) {
                throw invalid_iterator();
            }
            return &(*it);
        }

        bool operator==(const iterator &rhs) const {
            return owner == rhs.owner && bucket == rhs.bucket && it == rhs.it;
        }

        bool operator==(const const_iterator &rhs) const {
            return owner == rhs.owner && bucket == rhs.bucket && it == rhs.it;
        }

        bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }

        bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }
    };

    unordered_map() : buckets(nullptr), bucket_count(0), _size(0), hasher(Hash()), equal(KeyEqual()) {
        init(8);
    }

    unordered_map(const unordered_map &other)
        : buckets(nullptr), bucket_count(0), _size(0), hasher(other.hasher), equal(other.equal) {
        init(other.bucket_count);
        for (size_t i = 0; i < other.bucket_count; ++i) {
            buckets[i] = other.buckets[i];
        }
        _size = other._size;
    }

    unordered_map &operator=(const unordered_map &other) {
        if (this == &other) {
            return *this;
        }
        delete[] buckets;
        buckets = nullptr;
        bucket_count = 0;
        _size = 0;
        hasher = other.hasher;
        equal = other.equal;
        init(other.bucket_count);
        for (size_t i = 0; i < other.bucket_count; ++i) {
            buckets[i] = other.buckets[i];
        }
        _size = other._size;
        return *this;
    }

    ~unordered_map() {
        delete[] buckets;
        buckets = nullptr;
        bucket_count = 0;
        _size = 0;
    }

    bool empty() const {
        return _size == 0;
    }

    size_t size() const {
        return _size;
    }

    void clear() {
        for (size_t i = 0; i < bucket_count; ++i) {
            buckets[i].clear();
        }
        _size = 0;
    }

    void reserve(size_t n) {
        if (n <= bucket_count) {
            return;
        }
        rehash(n * 2 + 1);
    }

    iterator begin() {
        for (size_t i = 0; i < bucket_count; ++i) {
            if (!buckets[i].empty()) {
                return iterator(this, i, buckets[i].begin());
            }
        }
        return end();
    }

    const_iterator begin() const {
        for (size_t i = 0; i < bucket_count; ++i) {
            if (!buckets[i].empty()) {
                return const_iterator(this, i, buckets[i].begin());
            }
        }
        return end();
    }

    const_iterator cbegin() const {
        return begin();
    }

    iterator end() {
        return iterator(this, bucket_count, typename bucket_type::iterator());
    }

    const_iterator end() const {
        return const_iterator(this, bucket_count, typename bucket_type::const_iterator());
    }

    const_iterator cend() const {
        return end();
    }

    iterator find(const Key &key) {
        const size_t idx = bucket_index(key);
        auto it = buckets[idx].find(key);
        if (it == buckets[idx].end()) {
            return end();
        }
        return iterator(this, idx, it);
    }

    const_iterator find(const Key &key) const {
        const size_t idx = bucket_index(key);
        auto it = buckets[idx].find(key);
        if (it == buckets[idx].end()) {
            return end();
        }
        return const_iterator(this, idx, it);
    }

    template <class K, class V>
    pair<iterator, bool> emplace(K &&key, V &&value) {
        ensure_capacity(_size + 1);
        const size_t idx = bucket_index(key);
        auto found = buckets[idx].find(key);
        if (found != buckets[idx].end()) {
            return pair<iterator, bool>(iterator(this, idx, found), false);
        }
        auto inserted = buckets[idx].insert(value_type(sjtu::forward<K>(key), sjtu::forward<V>(value)));
        ++_size;
        return pair<iterator, bool>(iterator(this, idx, inserted.first), true);
    }

    pair<iterator, bool> insert(const value_type &v) {
        return emplace(v.first, v.second);
    }

    size_t erase(const Key &key) {
        const size_t idx = bucket_index(key);
        auto it = buckets[idx].find(key);
        if (it == buckets[idx].end()) {
            return 0;
        }
        buckets[idx].erase(it);
        --_size;
        return 1;
    }

    T &operator[](const Key &key) {
        return emplace(key, T()).first->second;
    }

    const T &operator[](const Key &key) const {
        auto it = find(key);
        if (it == end()) {
            throw index_out_of_bound();
        }
        return it->second;
    }
};

template <class Key, class T, class Hash, class KeyEqual>
bool unordered_map<Key, T, Hash, KeyEqual>::iterator::operator==(const const_iterator &rhs) const {
    return owner == rhs.owner && bucket == rhs.bucket && it == rhs.it;
}

template <class Key, class T, class Hash, class KeyEqual>
bool unordered_map<Key, T, Hash, KeyEqual>::iterator::operator!=(const const_iterator &rhs) const {
    return !(*this == rhs);
}

}  // namespace sjtu

#endif
