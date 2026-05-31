/**
 * implement a container like std::map
 */
#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

#include "exceptions.hpp"
#include "utility.hpp"

namespace sjtu {

template<class Key, class T, class Compare = std::less<Key>>
class map {
public:
    typedef pair<const Key, T> value_type;

private:
    enum colourT { RED, BLACK };

    struct RedBlackNode {
        value_type data;
        RedBlackNode *l;
        RedBlackNode *r;
        RedBlackNode *parent;
        colourT colour;

        RedBlackNode(const colourT c, const value_type &v,
                     RedBlackNode *left = nullptr,
                     RedBlackNode *right = nullptr,
                     RedBlackNode *p = nullptr)
            : data(v), l(left), r(right), parent(p), colour(c) {}
    };

    RedBlackNode *root;
    size_t _size;
    Compare comp;

    void transplant(RedBlackNode *u, RedBlackNode *v) {
        if (u->parent == nullptr) root = v;
        else if (u == u->parent->l) u->parent->l = v;
        else u->parent->r = v;

        if (v) v->parent = u->parent;
    }

    bool isBlack(RedBlackNode *x) const {
        return x == nullptr || x->colour == BLACK;
    }

    void rotateLeft(RedBlackNode *x) {
        RedBlackNode *y = x->r;
        if (y == nullptr) return;

        x->r = y->l;
        if (y->l) y->l->parent = x;

        y->parent = x->parent;

        if (x->parent == nullptr) root = y;
        else if (x == x->parent->l) x->parent->l = y;
        else x->parent->r = y;

        y->l = x;
        x->parent = y;
    }

    void rotateRight(RedBlackNode *x) {
        RedBlackNode *y = x->l;
        if (y == nullptr) return;

        x->l = y->r;
        if (y->r) y->r->parent = x;

        y->parent = x->parent;

        if (x->parent == nullptr) root = y;
        else if (x == x->parent->r) x->parent->r = y;
        else x->parent->l = y;

        y->r = x;
        x->parent = y;
    }

    void insertFix(RedBlackNode *node) {
        RedBlackNode *parent = nullptr;
        RedBlackNode *grand = nullptr;

        while (node != root && node->parent->colour == RED) {  //gp is black / gp.parent is black
            parent = node->parent;
            grand = parent->parent;

            if (parent == grand->l) {   //p is the left child of gp
                RedBlackNode *uncle = grand->r;
                if (uncle && uncle->colour == RED) {   //gp is black , p and uncle is red ,exchange the color ; send upwards
                    parent->colour = BLACK;
                    uncle->colour = BLACK;
                    grand->colour = RED;
                    node = grand;
                } else {  // gp is black , p is red , uncle is black
                    if (node == parent->r) {  //gp p node aren't a straight line -> Straighten the curved shape
                        rotateLeft(parent);
                        //update after rotate
                        node = parent;
                        parent = node->parent;
                    }
                    parent->colour = BLACK;
                    grand->colour = RED;
                    rotateRight(grand);
                    //break;  //The adjustment is complete. The parent of the node must be black.
                }
            } else {  //The logic is the same as be the left son.
                RedBlackNode *uncle = grand->l;
                if (uncle && uncle->colour == RED) {
                    parent->colour = BLACK;
                    uncle->colour = BLACK;
                    grand->colour = RED;
                    node = grand;
                } else {
                    if (node == parent->l) {
                        rotateRight(parent);
                        node = parent;
                        parent = node->parent;
                    }
                    parent->colour = BLACK;
                    grand->colour = RED;
                    rotateLeft(grand);
                }
            }
        }

        if (root) root->colour = BLACK;
    }

    void deleteFix(RedBlackNode *x, RedBlackNode *parent) {
        while (x != root && isBlack(x)) {
            //Always thinking that there is one less black node on the x side, so offset is our goal!
            if (x == (parent ? parent->l : nullptr)) { //x is the left child of parent
                RedBlackNode *w = parent ? parent->r : nullptr; //if parent isn't nullptr ; w = parent's right child (x's brother)
                //situation1  make brother's color black and then continue to fall into 2/3/4 situation
                if (w && w->colour == RED) {  //brother is red
                    w->colour = BLACK;
                    parent->colour = RED;
                    rotateLeft(parent);
                    w = parent->r;
                }
                //situation2
                if (isBlack(w ? w->l : nullptr) && isBlack(w ? w->r : nullptr)) { //The brother is black, and both of his children are also black.
                    //brother's side also "can't borrow the black node" , we can do nothing but push the "black gap" up to the father's level
                    if (w) w->colour = RED;
                    x = parent;
                    parent = x ? x->parent : nullptr;
                } else {
                    //situation 3
                    /*
                    * parent(?)
                    /        \
                  x(B)       w(B)
                            /    \
                        near(R)   far(B)

                    change color   ->  why? Because I need to rotate right around w later.
                    After right-rotating, near will take over. Therefore, first dye near black to ensure that
                    it can assume the role of a black node after being promoted. And w went down, dyeing it red first.

                    parent(?)
                   /        \
                x(B)       w(R)
                          /    \
                    near(B)   far(B)

                    after rotateRight(w);
                    parent(?)
                   /        \
                 x(B)      near(B)
                             \
                             w(R)
                              \
                             far(B)
                     */
                    if (isBlack(w ? w->r : nullptr)) {//Brothers are black. The distant nephew of the brothers is black while the close nephew is red.
                        //change into The situation where brother is black, while a distant nephew is red.
                        if (w && w->l) w->l->colour = BLACK;
                        if (w) {
                            w->colour = RED;
                            rotateRight(w);
                        }
                        w = parent ? parent->r : nullptr;
                    }
                    //the standard terminal form
                    /*
                    parent(?)                dye  ->   parent(B)
                   /        \
                 x(B)       w(B)                        w(parent原颜色)
                           /    \
                          A     far(R)
                     */
                    if (w) w->colour = parent->colour;
                    if (parent) parent->colour = BLACK;
                    if (w && w->r) w->r->colour = BLACK;
                    if (parent) rotateLeft(parent);
                /*
                        w(parent原颜色)
                        /          \
                  parent(B)        far(B)
                 /      \
               x(B)      A
                 */

                    x = root;
                    break;
                }
            } else {
                RedBlackNode *w = parent ? parent->l : nullptr;

                if (w && w->colour == RED) {
                    w->colour = BLACK;
                    parent->colour = RED;
                    rotateRight(parent);
                    w = parent->l;
                }

                if (isBlack(w ? w->l : nullptr) && isBlack(w ? w->r : nullptr)) {
                    if (w) w->colour = RED;
                    x = parent;
                    parent = x ? x->parent : nullptr;
                } else {
                    if (isBlack(w ? w->l : nullptr)) {
                        if (w && w->r) w->r->colour = BLACK;
                        if (w) {
                            w->colour = RED;
                            rotateLeft(w);
                        }
                        w = parent ? parent->l : nullptr;
                    }

                    if (w) w->colour = parent->colour;
                    if (parent) parent->colour = BLACK;
                    if (w && w->l) w->l->colour = BLACK;
                    if (parent) rotateRight(parent);

                    x = root;
                    break;
                }
            }
        }

        if (x) x->colour = BLACK;
    }

    void makeEmpty(RedBlackNode *node) {
        if (node != nullptr) {
            makeEmpty(node->l);
            makeEmpty(node->r);
            node->~RedBlackNode();
            ::operator delete(node);
        }
    }

    RedBlackNode *copy(RedBlackNode *parent, RedBlackNode *other) {
        if (other == nullptr) return nullptr;

        RedBlackNode *node = static_cast<RedBlackNode *>(::operator new(sizeof(RedBlackNode)));
        new (node) RedBlackNode(other->colour, other->data, nullptr, nullptr, parent);

        node->l = copy(node, other->l);
        node->r = copy(node, other->r);
        return node;
    }

    static RedBlackNode *findMin(RedBlackNode *node) {
        if (!node) return nullptr;
        while (node->l) node = node->l;
        return node;
    }

    static RedBlackNode *findMax(RedBlackNode *node) {
        if (!node) return nullptr;
        while (node->r) node = node->r;
        return node;
    }

public:
    class const_iterator;

    class iterator {
        friend class map;
        friend class const_iterator;
    private:
        RedBlackNode *ptr;
        map* _map;

    public:
        iterator(map *m = nullptr, RedBlackNode *p = nullptr) : ptr(p), _map(m) {}
        iterator(const iterator &other) : ptr(other.ptr), _map(other._map) {}

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        iterator &operator++() {
            if (ptr == nullptr) throw invalid_iterator();

            if (ptr->r != nullptr) {//has a right son
                ptr = ptr->r;
                while (ptr->l) ptr = ptr->l;
                return *this;
            }

            RedBlackNode *p = ptr->parent;
            while (p && ptr == p->r) {//if it points to the biggest element, it return nullptr which represents end()
                ptr = p;
                p = p->parent;
            }

            ptr = p;
            return *this;
        }

        iterator operator--(int) {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }

        iterator &operator--() {
            if (_map == nullptr) throw invalid_iterator();

            if (ptr == nullptr) {
                ptr = _map->findMax(_map->root);
                if (!ptr) throw invalid_iterator();
                return *this;
            }

            if (ptr->l) {//has a left son
                ptr = ptr->l;
                while (ptr->r) ptr = ptr->r;
                return *this;
            }

            RedBlackNode *p = ptr->parent;
            while (p && ptr == p->l) {
                ptr = p;
                p = p->parent;
            }

            if (p == nullptr) throw invalid_iterator();
            ptr = p;
            return *this;
        }

        value_type &operator*() const {
            if (ptr == nullptr) throw invalid_iterator();
            return ptr->data;
        }

        value_type *operator->() const {
            if (ptr == nullptr) throw invalid_iterator();
            return &(ptr->data);
        }

        bool operator==(const iterator &rhs) const {
            return ptr == rhs.ptr && _map == rhs._map;
        }

        bool operator==(const const_iterator &rhs) const {
            return ptr == rhs.ptr && _map == rhs._map;
        }

        bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }

        bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }
    };

    class const_iterator {
        friend class map;
        friend class iterator;
    private:
        RedBlackNode *ptr;
        const map * _map;

    public:
        const_iterator(const map *m = nullptr, RedBlackNode *p = nullptr) : ptr(p), _map(m) {}
        const_iterator(const const_iterator &other) : ptr(other.ptr), _map(other._map) {}
        const_iterator(const iterator &other) : ptr(other.ptr), _map(other._map) {}

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        const_iterator &operator++() {
            if (ptr == nullptr) throw invalid_iterator();

            if (ptr->r != nullptr) {
                ptr = ptr->r;
                while (ptr->l) ptr = ptr->l;
                return *this;
            }

            RedBlackNode *p = ptr->parent;
            while (p && ptr == p->r) {
                ptr = p;
                p = p->parent;
            }

            ptr = p;
            return *this;
        }

        const_iterator operator--(int) {
            const_iterator tmp = *this;
            --(*this);
            return tmp;
        }

        const_iterator &operator--() {
            if (_map == nullptr) throw invalid_iterator();

            if (ptr == nullptr) {
                ptr = map::findMax(_map->root);
                if (!ptr) throw invalid_iterator();
                return *this;
            }

            if (ptr->l) {
                ptr = ptr->l;
                while (ptr->r) ptr = ptr->r;
                return *this;
            }

            RedBlackNode *p = ptr->parent;
            while (p && ptr == p->l) {
                ptr = p;
                p = p->parent;
            }

            if (p == nullptr) throw invalid_iterator();
            ptr = p;
            return *this;
        }

        const value_type &operator*() const {
            if (ptr == nullptr) throw invalid_iterator();
            return ptr->data;
        }

        const value_type *operator->() const {
            if (ptr == nullptr) throw invalid_iterator();
            return &(ptr->data);
        }

        bool operator==(const iterator &rhs) const {
            return ptr == rhs.ptr && _map == rhs._map;
        }

        bool operator==(const const_iterator &rhs) const {
            return ptr == rhs.ptr && _map == rhs._map;
        }

        bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }

        bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }
    };

    map() : root(nullptr), _size(0), comp(Compare()) {}

    map(const map &other) : root(nullptr), _size(other._size), comp(other.comp) {
        root = copy(nullptr, other.root);
    }

    map &operator=(const map &other) {
        if (this == &other) return *this;
        clear();
        root = copy(nullptr, other.root);
        _size = other._size;
        comp = other.comp;
        return *this;
    }

    ~map() {
        makeEmpty(root);
        root = nullptr;
    }

    T &at(const Key &key) {
        iterator it = find(key);
        if (it == end()) throw index_out_of_bound();
        return it->second;
    }

    const T &at(const Key &key) const {
        const_iterator it = find(key);
        if (it == end()) throw index_out_of_bound();
        return it->second;
    }

    T &operator[](const Key &key) {
        RedBlackNode *t = root;
        RedBlackNode *parent = nullptr;

        while (t) {
            if (!(comp(key, t->data.first) || comp(t->data.first, key)))//in the map
                return t->data.second;
            parent = t;
            t = comp(key, t->data.first) ? t->l : t->r;
        }
        //insert
        RedBlackNode *node = new RedBlackNode(RED, value_type(key, T()), nullptr, nullptr, parent);
        if (!parent) root = node;
        else if (comp(key, parent->data.first)) parent->l = node;
        else parent->r = node;

        insertFix(node);
        ++_size;
        return node->data.second;
    }

    const T &operator[](const Key &key) const {
        const_iterator it = find(key);
        if (it == end()) throw index_out_of_bound();
        return it->second;
    }

    iterator begin() {
        return iterator(this, findMin(root));
    }

    const_iterator begin() const {
        return const_iterator(this, findMin(root));
    }

    const_iterator cbegin() const {
        return const_iterator(this, findMin(root));
    }

    iterator end() {
        return iterator(this, nullptr);
    }

    const_iterator end() const {
        return const_iterator(this, nullptr);
    }

    const_iterator cend() const {
        return const_iterator(this, nullptr);
    }

    bool empty() const {
        return _size == 0;
    }

    size_t size() const {
        return _size;
    }

    void clear() {
        makeEmpty(root);
        root = nullptr;
        _size = 0;
    }

    pair<iterator, bool> insert(const value_type &v) {
        RedBlackNode *t = root;
        RedBlackNode *parent = nullptr;

        while (t) {
            if (!(comp(v.first, t->data.first) || comp(t->data.first, v.first)))  // value already in the tree
                return pair<iterator, bool>(iterator(this, t), false);
            parent = t;
            t = comp(v.first, t->data.first) ? t->l : t->r;
        }

        RedBlackNode *node = new RedBlackNode(RED, v, nullptr, nullptr, parent); // guarantee same number of black node , just go against rule2
        //Maintain the parent pointer
        if (!parent) root = node;
        else if (comp(v.first, parent->data.first)) parent->l = node;
        else parent->r = node;

        insertFix(node);
        ++_size;
        return pair<iterator, bool>(iterator(this, node), true);
    }

    void erase(iterator pos) {
        if (pos._map != this || pos.ptr == nullptr) throw invalid_iterator();

        RedBlackNode *z = pos.ptr;
        RedBlackNode *y = z;
        RedBlackNode *x = nullptr;
        RedBlackNode *x_parent = nullptr;

        colourT y_original_color = y->colour;

        if (z->l == nullptr) {
            x = z->r;
            x_parent = z->parent;
            transplant(z, z->r);
        } else if (z->r == nullptr) {
            x = z->l;
            x_parent = z->parent;
            transplant(z, z->l);
        } else {
            y = findMin(z->r);
            y_original_color = y->colour;
            x = y->r;

            if (y->parent == z) {
                if (x) x->parent = y;
                x_parent = y;
            } else {
                //Attach the right subtree of y to y's parent.
                x_parent = y->parent;
                transplant(y, y->r);
                //Attach the right subtree of z to y.right
                y->r = z->r;
                y->r->parent = y;
            }
            //Replace z with y
            transplant(z, y);
            y->l = z->l;
            y->l->parent = y;
            //just replace the value, color doesn't change (inherited)
            y->colour = z->colour;
        }

        z->~RedBlackNode();
        ::operator delete(z);
        --_size;

        if (y_original_color == BLACK) {
            deleteFix(x, x_parent);  //adjust
        }
    }

    size_t count(const Key &key) const {
        return find(key) == end() ? 0 : 1;
    }

    iterator find(const Key &key) {
        RedBlackNode *t = root;
        while (t != nullptr && (comp(t->data.first, key) || comp(key, t->data.first))) {
            if (comp(key, t->data.first)) t = t->l;
            else t = t->r;
        }
        if (t == nullptr) return end();
        return iterator(this, t);
    }

    const_iterator find(const Key &key) const {
        RedBlackNode *t = root;
        while (t != nullptr && (comp(t->data.first, key) || comp(key, t->data.first))) {
            if (comp(key, t->data.first)) t = t->l;
            else t = t->r;
        }
        if (t == nullptr) return end();
        return const_iterator(this, t);
    }
};

} // namespace sjtu

#endif