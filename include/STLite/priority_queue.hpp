#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cmath>       // in case you need it
#include <cstddef>     // for size_t
#include <functional>  // for std::less

#include "exceptions.hpp"

namespace sjtu {

/**
 * @brief A container automatically sorting its contents, similar to
 * std::priority_queue but with extra functionalities.
 *
 * The extra functionalities are:
 * - Merge two priority queues into one (with good time complexity).
 * - Clear all elements in the queue.
 * - Limited exception safety for some operations (e.g. push, pop, top, merge)
 * when the comparator throws exceptions from `Compare` only.
 *
 * This @priority_queue does not support passing an underlying container as a template parameter.
 * Also, it does not support passing a comparator object as a constructor argument.
 *
 */
template <class T, class Compare = std::less<T>>
class priority_queue {
<<<<<<< HEAD
private:
    struct Node {
        T value;
        Node* left;
        Node* right;

        Node():left(nullptr), right(nullptr){};
        explicit Node(T v):value(v), left(nullptr), right(nullptr){};
    };

    Node* root;
    int current_size;
    Compare cmp;

    Node* copy(Node* node) {
        if (node == nullptr) {
            return nullptr;
        }

        Node* newNode = new Node(node->value);
        newNode->left = copy(node->left);
        newNode->right = copy(node->right);
        return newNode;
    }

    void clear(Node* node) {
        if (!node)  return;

        clear(node->left);
        clear(node->right);
        delete node;
    }

    Node* merge(Node* r1, Node* r2) {
        if (!r1) return r2;
        if (!r2) return r1;

        try {
            if (cmp(r1->value, r2->value))
                std::swap(r1, r2);
        }
        catch (...) {
            throw;
        }

        r1->right = merge(r1->right, r2);

        std::swap(r1->left, r1->right);

        return r1;
    }
   public:
    priority_queue(): root(nullptr), current_size(0){};
    priority_queue(const priority_queue &other) {
        current_size = other.current_size;
        root = copy(other.root);
        cmp = other.cmp;
    }


    ~priority_queue() {
        clear(root);
    }

    priority_queue &operator=(const priority_queue &other) {
        if (this == &other) {
            return *this;
        }

        clear(root);
        root = copy(other.root);
        current_size = other.current_size;
        cmp = other.cmp;

        return *this;
    }

    /** Adds one element to the queue. */
    void push(const T &e) {
        Node* newNode = new Node(e);
        Node* oldRoot = root;

        try {
            root = merge(oldRoot, newNode);
            ++current_size;
        }
        catch (...) {
            delete newNode;
            throw;
        }
    }
=======
   public:
    priority_queue();
    priority_queue(const priority_queue&);
    ~priority_queue();

    priority_queue& operator=(const priority_queue&);

    /** Adds one element to the queue. */
    void push(const T&);
>>>>>>> acm-course/main

    /**
     * Returns a read-only reference of the first element in the queue.
     *
     * @throws container_is_empty when the first element does not exist.
     */
<<<<<<< HEAD
    const T & top() const {
        if (empty()) {
            throw container_is_empty();
        }

        return root->value;
    }
=======
    const T& top() const;
>>>>>>> acm-course/main

    /**
     * Removes the first element in the queue.
     *
     * @throws container_is_empty when the first element does not exist.
     */
<<<<<<< HEAD
    void pop() {
        if (empty())
            throw container_is_empty();

        Node* oldRoot = root;
        Node* leftChild = root->left;
        Node* rightChild = root->right;

        try {
            root = merge(leftChild, rightChild);
            --current_size;
            delete oldRoot;
        }
        catch (...) {
            throw;
        }
    }

    /** Returns the number of elements in the queue. */
    size_t size() const {
        return current_size;
    }

    /** Returns whether there is any element in the queue. */
    bool empty() const {
        return current_size == 0;
    }

    /** Clears all elements in the queue. */
    void clear() {
        clear(root);
        root = nullptr;
        current_size = 0;
    }
=======
    void pop();

    /** Returns the number of elements in the queue. */
    size_t size() const;

    /** Returns whether there is any element in the queue. */
    bool empty() const;

    /** Clears all elements in the queue. */
    void clear();
>>>>>>> acm-course/main

    /**
     * @brief Merges two priority queues into one.
     *
     * The merged data shall be stored in the current priority queue and the
     * other priority queue shall be cleared after merging.
     *
     * The time complexity shall be O(log n) or better.
     */
<<<<<<< HEAD
    void merge(priority_queue &other) {
        if (this == &other) return;

        try {
            root = merge(root, other.root);
            current_size += other.current_size;

            other.root = nullptr;
            other.current_size = 0;
        }
        catch (...) {
            throw;
        }
    }
=======
    void merge(priority_queue&);
>>>>>>> acm-course/main
};

}  // namespace sjtu

#endif