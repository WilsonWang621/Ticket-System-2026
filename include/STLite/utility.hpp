#ifndef SJTU_UTILITY_HPP
#define SJTU_UTILITY_HPP

#include <cstdio>
#include <string>

namespace sjtu {

template <class T>
struct remove_reference {
    using type = T;
};

template <class T>
struct remove_reference<T &> {
    using type = T;
};

template <class T>
struct remove_reference<T &&> {
    using type = T;
};

template <class T>
constexpr T &&forward(typename remove_reference<T>::type &value) {
    return static_cast<T &&>(value);
}

template <class T>
constexpr T &&forward(typename remove_reference<T>::type &&value) {
    return static_cast<T &&>(value);
}

template <class T>
constexpr typename remove_reference<T>::type &&move(T &&value) {
    return static_cast<typename remove_reference<T>::type &&>(value);
}

template <class T>
void swap(T &lhs, T &rhs) {
    T tmp = sjtu::move(lhs);
    lhs = sjtu::move(rhs);
    rhs = sjtu::move(tmp);
}

template <class T>
struct less {
    bool operator()(const T &lhs, const T &rhs) const {
        return lhs < rhs;
    }
};

template <class T>
struct equal_to {
    bool operator()(const T &lhs, const T &rhs) const {
        return lhs == rhs;
    }
};

struct random_access_iterator_tag {};
struct bidirectional_iterator_tag {};

template <class T>
struct hash {
    size_t operator()(const T &value) const {
        return static_cast<size_t>(value);
    }
};

template <>
struct hash<std::string> {
    size_t operator()(const std::string &value) const {
        size_t result = 1469598103934665603ull;
        for (size_t i = 0; i < value.size(); ++i) {
            result ^= static_cast<unsigned char>(value[i]);
            result *= 1099511628211ull;
        }
        return result;
    }
};

template <class T1, class T2>
class pair {
   public:
    T1 first;
    T2 second;
    constexpr pair() : first(), second() {
    }
    pair(const pair &other) = default;
    pair(pair &&other) = default;
    pair(const T1 &x, const T2 &y) : first(x), second(y) {
    }
    template <class U1, class U2>
    pair(U1 &&x, U2 &&y) : first(sjtu::forward<U1>(x)), second(sjtu::forward<U2>(y)) {
    }
    template <class U1, class U2>
    pair(const pair<U1, U2> &other) : first(other.first), second(other.second) {
    }
    template <class U1, class U2>
    pair(pair<U1, U2> &&other)
        : first(sjtu::move(other.first)), second(sjtu::move(other.second)) {
    }
};

}  // namespace sjtu

#endif
