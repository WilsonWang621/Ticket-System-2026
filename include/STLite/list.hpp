#ifndef SJTU_LIST_HPP
#define SJTU_LIST_HPP

#include "exceptions.hpp"

#include <cstddef>
#include <iterator>
#include <utility>

namespace sjtu {

template <class T>
class list {
private:
    struct NodeBase {
        NodeBase *prev;
        NodeBase *next;

        NodeBase() : prev(this), next(this) {
        }
    };

    struct Node : NodeBase {
        T value;

        template <class... Args>
        explicit Node(Args &&...args) : value(std::forward<Args>(args)...) {
        }
    };

    NodeBase head;
    std::size_t _size;

    static Node *as_node(NodeBase *ptr) {
        return static_cast<Node *>(ptr);
    }

    static const Node *as_node(const NodeBase *ptr) {
        return static_cast<const Node *>(ptr);
    }

    void insert_before(NodeBase *pos, NodeBase *node) {
        node->prev = pos->prev;
        node->next = pos;
        pos->prev->next = node;
        pos->prev = node;
    }

    void unlink(NodeBase *node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        node->prev = node;
        node->next = node;
    }

    void copy_from(const list &other) {
        for (const auto &value : other) {
            emplace_back(value);
        }
    }

public:
    class const_iterator;

    class iterator {
        friend class list;
        friend class const_iterator;
    private:
        NodeBase *ptr;
        list *owner;

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = T *;
        using reference = T &;
        using iterator_category = std::bidirectional_iterator_tag;

        iterator(NodeBase *p = nullptr, list *lst = nullptr) : ptr(p), owner(lst) {
        }

        iterator(const iterator &other) = default;

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        iterator &operator++() {
            if (owner == nullptr || ptr == nullptr || ptr == &owner->head) {
                throw invalid_iterator();
            }
            ptr = ptr->next;
            return *this;
        }

        iterator operator--(int) {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }

        iterator &operator--() {
            if (owner == nullptr || ptr == nullptr) {
                throw invalid_iterator();
            }
            if (ptr == &owner->head) {
                if (owner->_size == 0) {
                    throw invalid_iterator();
                }
                ptr = owner->head.prev;
                return *this;
            }
            if (ptr->prev == &owner->head) {
                throw invalid_iterator();
            }
            ptr = ptr->prev;
            return *this;
        }

        T &operator*() const {
            if (owner == nullptr || ptr == nullptr || ptr == &owner->head) {
                throw invalid_iterator();
            }
            return as_node(ptr)->value;
        }

        T *operator->() const {
            if (owner == nullptr || ptr == nullptr || ptr == &owner->head) {
                throw invalid_iterator();
            }
            return &(as_node(ptr)->value);
        }

        bool operator==(const iterator &rhs) const {
            return ptr == rhs.ptr && owner == rhs.owner;
        }

        bool operator==(const const_iterator &rhs) const;

        bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }

        bool operator!=(const const_iterator &rhs) const;
    };

    class const_iterator {
        friend class list;
        friend class iterator;
    private:
        const NodeBase *ptr;
        const list *owner;

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = const T *;
        using reference = const T &;
        using iterator_category = std::bidirectional_iterator_tag;

        const_iterator(const NodeBase *p = nullptr, const list *lst = nullptr) : ptr(p), owner(lst) {
        }

        const_iterator(const iterator &other) : ptr(other.ptr), owner(other.owner) {
        }

        const_iterator(const const_iterator &other) = default;

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        const_iterator &operator++() {
            if (owner == nullptr || ptr == nullptr || ptr == &owner->head) {
                throw invalid_iterator();
            }
            ptr = ptr->next;
            return *this;
        }

        const_iterator operator--(int) {
            const_iterator tmp = *this;
            --(*this);
            return tmp;
        }

        const_iterator &operator--() {
            if (owner == nullptr || ptr == nullptr) {
                throw invalid_iterator();
            }
            if (ptr == &owner->head) {
                if (owner->_size == 0) {
                    throw invalid_iterator();
                }
                ptr = owner->head.prev;
                return *this;
            }
            if (ptr->prev == &owner->head) {
                throw invalid_iterator();
            }
            ptr = ptr->prev;
            return *this;
        }

        const T &operator*() const {
            if (owner == nullptr || ptr == nullptr || ptr == &owner->head) {
                throw invalid_iterator();
            }
            return as_node(ptr)->value;
        }

        const T *operator->() const {
            if (owner == nullptr || ptr == nullptr || ptr == &owner->head) {
                throw invalid_iterator();
            }
            return &(as_node(ptr)->value);
        }

        bool operator==(const iterator &rhs) const {
            return ptr == rhs.ptr && owner == rhs.owner;
        }

        bool operator==(const const_iterator &rhs) const {
            return ptr == rhs.ptr && owner == rhs.owner;
        }

        bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }

        bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }
    };

    list() : head(), _size(0) {
    }

    list(const list &other) : head(), _size(0) {
        copy_from(other);
    }

    list &operator=(const list &other) {
        if (this == &other) {
            return *this;
        }
        clear();
        copy_from(other);
        return *this;
    }

    ~list() {
        clear();
    }

    iterator begin() {
        return iterator(head.next, this);
    }

    const_iterator begin() const {
        return const_iterator(head.next, this);
    }

    const_iterator cbegin() const {
        return const_iterator(head.next, this);
    }

    iterator end() {
        return iterator(&head, this);
    }

    const_iterator end() const {
        return const_iterator(&head, this);
    }

    const_iterator cend() const {
        return const_iterator(&head, this);
    }

    bool empty() const {
        return _size == 0;
    }

    std::size_t size() const {
        return _size;
    }

    void clear() {
        NodeBase *cur = head.next;
        while (cur != &head) {
            NodeBase *next = cur->next;
            as_node(cur)->~Node();
            ::operator delete(cur);
            cur = next;
        }
        head.next = &head;
        head.prev = &head;
        _size = 0;
    }

    template <class... Args>
    iterator emplace_front(Args &&...args) {
        Node *node = static_cast<Node *>(::operator new(sizeof(Node)));
        try {
            new (node) Node(std::forward<Args>(args)...);
        } catch (...) {
            ::operator delete(node);
            throw;
        }
        insert_before(head.next, node);
        ++_size;
        return iterator(node, this);
    }

    void push_back(const T &value) {
        emplace_back(value);
    }

    template <class... Args>
    iterator emplace_back(Args &&...args) {
        Node *node = static_cast<Node *>(::operator new(sizeof(Node)));
        try {
            new (node) Node(std::forward<Args>(args)...);
        } catch (...) {
            ::operator delete(node);
            throw;
        }
        insert_before(&head, node);
        ++_size;
        return iterator(node, this);
    }

    void pop_back() {
        if (_size == 0) {
            throw container_is_empty();
        }
        NodeBase *node = head.prev;
        unlink(node);
        as_node(node)->~Node();
        ::operator delete(node);
        --_size;
    }

    void splice(iterator pos, list &other, iterator it) {
        if (pos.owner != this || it.owner != &other) {
            throw invalid_iterator();
        }
        if (it.ptr == nullptr || it.ptr == &other.head) {
            return;
        }

        NodeBase *node = const_cast<NodeBase *>(it.ptr);
        if (this == &other && (pos.ptr == node || pos.ptr == node->next)) {
            return;
        }

        unlink(node);
        if (this != &other) {
            --other._size;
            ++_size;
        }
        insert_before(pos.ptr, node);
    }
};

template <class T>
bool list<T>::iterator::operator==(const const_iterator &rhs) const {
    return ptr == rhs.ptr && owner == rhs.owner;
}

template <class T>
bool list<T>::iterator::operator!=(const const_iterator &rhs) const {
    return !(*this == rhs);
}

}  // namespace sjtu

#endif
