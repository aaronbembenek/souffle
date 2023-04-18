#pragma once

#include "souffle/utility/Iteration.h"
#include <cassert>
#include <oneapi/tbb/concurrent_set.h>

namespace souffle {

namespace detail {

template <typename Key, typename Comparator, bool isSet>
class TBB {
private:
    struct Less {
        bool operator()(const Key& l, const Key& r) const {
            Comparator cmp;
            return cmp(l, r) < 0;
        }
    };

    using inner_t = typename std::conditional<isSet, oneapi::tbb::concurrent_set<Key, Less>,
            oneapi::tbb::concurrent_multiset<Key, Less>>::type;

    inner_t inner;

public:
    struct operation_hints {};

    using iterator = typename inner_t::const_iterator;

    iterator begin() const {
        return inner.begin();
    }

    iterator end() const {
        return inner.end();
    }

    bool insert(const Key& k) {
        operation_hints h;
        return insert(k, h);
    }

    bool insert(const Key& k, operation_hints& /* h */) {
        return inner.emplace(k).second;
    }

    bool contains(const Key& k) const {
        operation_hints h;
        return contains(k, h);
    }

    bool contains(const Key& k, operation_hints& /* h */) const {
        return inner.contains(k);
    }

    iterator find(const Key& k) const {
        operation_hints h;
        return find(k, h);
    }

    iterator find(const Key& k, operation_hints& /* h */) const {
        return inner.find(k);
    }

    iterator lower_bound(const Key& k) const {
        operation_hints hints;
        return lower_bound(k, hints);
    }

    iterator lower_bound(const Key& k, operation_hints& /* h */) const {
        return inner.lower_bound(k);
    }

    iterator upper_bound(const Key& k) const {
        operation_hints hints;
        return upper_bound(k, hints);
    }

    iterator upper_bound(const Key& k, operation_hints& /* h */) const {
        return inner.upper_bound(k);
    }

    std::size_t size() const {
        return inner.size();
    }

    bool empty() const {
        return inner.empty();
    }

    void clear() {
        inner.clear();
    }

    void printStats(std::ostream& /* o */) const {}

    using size_type = std::size_t;
    using chunk = range<iterator>;

    std::vector<chunk> getChunks(size_type /* num */) const {
        assert(false && "should never get chunks during eager evaluation");
        return {};
    }
};

}  // namespace detail

template <typename Key, typename Comparator>
using eager_eval_set = detail::TBB<Key, Comparator, true>;

template <typename Key, typename Comparator>
using eager_eval_multiset = detail::TBB<Key, Comparator, false>;

}  // namespace souffle