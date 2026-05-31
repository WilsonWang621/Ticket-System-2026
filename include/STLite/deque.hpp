#ifndef SJTU_DEQUE_HPP
#define SJTU_DEQUE_HPP

#include "exceptions.hpp"
#include <cstddef>

namespace sjtu {

template<class T>
class deque {
private:
    static constexpr int chunk_size = 16;
    static constexpr int map_base = 8;
    struct Chunk {
        T* arr;   //整块内存起点
        T* first;   //当前有效元素起点
        T* last;  //最后的下一个
        Chunk() {
            arr = static_cast<T*>(::operator new(sizeof(T) * chunk_size));
            first = last = arr;
        }
        ~Chunk() {
            for (T* p = first; p != last; ++p) {
                p->~T();
            }
            ::operator delete(arr);
        }
    };
    Chunk** map;   //map[i] 指向一个 chunk
    int map_size, start, last, sum;   //[start, last] 是有效 chunk 区间

    void map_extend() {
        map_size *= 2;
        auto **new_map = static_cast<Chunk **>(::operator new(sizeof(Chunk*) * map_size));

        for (int i = 0; i < map_size; ++i)
            new_map[i] = nullptr;

        int count = last - start + 1;
        int new_start = (map_size - count) / 2;

        for (int i = 0; i < count; ++i)
            new_map[new_start + i] = map[start + i];

        ::operator delete(map);

        map = new_map;

        start = new_start;
        last = new_start + count - 1;
    }

public:
	class const_iterator;
	class iterator {
	    friend class deque;
	private:
		/**
		 * TODO add data members
		 *   just add whatever you want.
		 */
	    T* ptr;   //当前元素位置
	    T* first; //当前 chunk 的有效起点  支持 ++ / -- 跨 chunk  ptr = (*node)->first;
	    T* last; //当前 chunk 的有效终点（后一个）  判断“是否到达 chunk 边界” if (ptr == last)
	    Chunk** node;    //指向 指向chunk*的map[i]的指针   指向中央控制器中指向当前块的指针
	    deque<T>* dq;
	    int begin_offset;  //当前 iterator 距离 begin() 的偏移,用来实现：it1 - it2,否则无法 O(1) 算距离
	public:
	    explicit iterator(T* p = nullptr, T* f = nullptr, T* l = nullptr, Chunk** n = nullptr, deque<T>* d = nullptr, int off = 0)
	    :ptr(p), first(f), last(l), node(n), dq(d), begin_offset(off) {};
		/**
		 * return a new iterator which pointer n-next elements
		 *   even if there are not enough elements, the behaviour is **undefined**.
		 * as well as operator-
		 */
	    iterator operator+(const int &n) const {
	        //不连续数组 → 分段跳
	        iterator tmp = *this;

	        if (n >= 0) {
	            int remain = n;

	            while (remain > 0) {
	                int space = tmp.last - tmp.ptr;
	                //情况1：当前 chunk 够走
	                if (remain < space) {
	                    tmp.ptr += remain;
	                    remain = 0;
	                } else {//情况2：当前 chunk 不够
	                    remain -= space;
	                    //找下一个非空 chunk
	                    Chunk** next = tmp.node + 1;
	                    while (next <= dq->map + dq->last && (*next == nullptr || (*next)->first == (*next)->last))
	                        ++next;
	                    //已经越界：直接变成 end()
	                    if (next > dq->map + dq->last) {
	                        tmp.node = dq->map + dq->last;
	                        tmp.ptr = tmp.last;
	                        break;
	                    }
	                    //重置到新 chunk 开头
	                    tmp.node = next;
	                    tmp.first = (*next)->first;
	                    tmp.last = (*next)->last;
	                    tmp.ptr = tmp.first;
	                }
	            }
	            tmp.begin_offset = begin_offset + n;
	            return tmp;
	        } else {
	            return *this - (-n);
	        }
	    }


	    iterator operator-(const int &n) const {
	        iterator tmp = *this;

	        if (n >= 0) {
	            int remain = n;

	            while (remain > 0) {
	                int space = tmp.ptr - tmp.first;

	                if (remain <= space) {
	                    tmp.ptr -= remain;
	                    remain = 0;
	                } else {
	                    remain -= space + 1;

	                    Chunk** prev = tmp.node - 1;

	                    while (prev >= dq->map + dq->start && (*prev == nullptr || (*prev)->first == (*prev)->last))
	                        --prev;

	                    if (prev < dq->map + dq->start) {
	                        tmp.node = dq->map + dq->start;
	                        tmp.ptr = tmp.first;
	                        break;
	                    }

	                    tmp.node = prev;
	                    tmp.first = (*prev)->first;
	                    tmp.last = (*prev)->last;
	                    tmp.ptr = tmp.last - 1;
	                }
	            }
	        } else {
	            return *this + (-n);
	        }

	        tmp.begin_offset = begin_offset - n;
	        return tmp;
	    }

		// return th distance between two iterator,
		// if these two iterators points to different vectors, throw invaild_iterator.
	    int operator-(const iterator &rhs) const {
	        if (dq != rhs.dq) throw invalid_iterator();
	        return begin_offset - rhs.begin_offset;
	    }
		iterator operator+=(const int &n) {
			//TODO
		    *this = *this + n;
		    return *this;
		}
		iterator operator-=(const int &n) {
			//TODO
		    *this = *this - n;
		    return *this;
		}
		/**
		 * TODO iter++
		 */
		iterator operator++(int) {
		    iterator tmp = *this;
		    ++(*this);
		    return tmp;
		}
		/**
		 * TODO ++iter
		 */
	    iterator& operator++() {
	        if (ptr == last && node == dq->map + dq->last)
	            throw invalid_iterator();

	        ++ptr;
	        ++begin_offset;

	        if (ptr == last) {
	            if (node != dq->map + dq->last) {
	                ++node;
	                first = (*node)->first;
	                last  = (*node)->last;
	                ptr   = first;
	            }
	        }

	        return *this;
	    }

		/**
		 * TODO iter--
		 */
		iterator operator--(int) {
		    iterator tmp = *this;
		    --(*this);
		    return tmp;
		}
		/**
		 * TODO --iter
		 */
	    iterator& operator--() {
	        if (node == dq->map + dq->start && ptr == first)
	            throw invalid_iterator();

	        --begin_offset;

	        if (ptr == first) {
	            --node;
	            first = (*node)->first;
	            last  = (*node)->last;
	            ptr   = last - 1;
	        } else {
	            --ptr;
	        }


	        return *this;
	    }
		/**
		 * TODO *it
		 */
		T& operator*() const {
	            if (!dq || !node || ptr == last) throw invalid_iterator();
	            return *ptr;
		}
		/**
		 * TODO it->field
		 */
		T* operator->() const noexcept {
	        if (!dq || !node || ptr == last) throw invalid_iterator();
	            return ptr;
		}
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory).
		 */
		bool operator==(const iterator &rhs) const {
		    return dq == rhs.dq && node == rhs.node && ptr == rhs.ptr;
		}
		bool operator==(const const_iterator &rhs) const {
		    return dq == rhs.dq && node == rhs.node && ptr == rhs.ptr;
		}
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const {
		    return !(*this == rhs);
		}
		bool operator!=(const const_iterator &rhs) const {
		    return !(*this == rhs);
		}
	};
	class const_iterator {
	    friend class deque;
		// it should has similar member method as iterator.
		//  and it should be able to construct from an iterator.
		private:
			// data members.
	    const T* ptr;
	    const T* first;
	    const T* last;
	    Chunk** node;
	    const deque<T>* dq;
	    int begin_offset;

		public:
			    const_iterator() : ptr(nullptr), first(nullptr), last(nullptr),node(nullptr), dq(nullptr), begin_offset(0) {}

			    explicit const_iterator(const iterator &other) : ptr(other.ptr), first(other.first), last(other.last), node(other.node), dq(other.dq), begin_offset(other.begin_offset) {}

	                const_iterator(T* ptr,T* first,T* last,Chunk** node,const deque* dq, int off = 0) : ptr(ptr), first(first), last(last), node(node), dq(dq), begin_offset(off) {}
	                const_iterator operator+(const int &n) const {
			        const_iterator tmp = *this;

			        if (n >= 0) {
			            int remain = n;

			            while (remain > 0) {
			                int space = tmp.last - tmp.ptr;

			                if (remain < space) {
			                    tmp.ptr += remain;
			                    remain = 0;
			                } else {
			                    remain -= space;

			                    Chunk** next = tmp.node + 1;

			                    while (next <= dq->map + dq->last &&
                                                   (*next == nullptr ||
                                                    (*next)->first == (*next)->last))
			                        next++;

			                    if (next > dq->map + dq->last) {
			                        tmp.node = dq->map + dq->last;
			                        tmp.ptr = tmp.last;
			                        break;
			                    }

			                    tmp.node = next;
			                    tmp.first = (*next)->first;
			                    tmp.last = (*next)->last;
			                    tmp.ptr = tmp.first;
			                }
			            }
			        } else {
			            return *this - (-n);
			        }

			        tmp.begin_offset = begin_offset + n;
			        return tmp;
			    }


	                    const_iterator operator-(const int &n) const {
			        const_iterator tmp = *this;

			        if (n >= 0) {
			            int remain = n;

			            while (remain > 0) {
			                int space = tmp.ptr - tmp.first;

			                if (remain <= space) {
			                    tmp.ptr -= remain;
			                    remain = 0;
			                } else {
			                    remain -= space + 1;

			                    Chunk** prev = tmp.node - 1;

			                    while (prev >= dq->map + dq->start &&
                                                   (*prev == nullptr ||
                                                    (*prev)->first == (*prev)->last))
			                        prev--;

			                    if (prev < dq->map + dq->start) {
			                        tmp.node = dq->map + dq->start;
			                        tmp.ptr = tmp.first;
			                        break;
			                    }

			                    tmp.node = prev;
			                    tmp.first = (*prev)->first;
			                    tmp.last = (*prev)->last;
			                    tmp.ptr = tmp.last - 1;
			                }
			            }
			        } else {
			            return *this + (-n);
			        }

			        tmp.begin_offset = begin_offset - n;
			        return tmp;
			    }
	                int operator-(const const_iterator &rhs) const {
			        if (dq != rhs.dq) throw invalid_iterator();
			        return begin_offset - rhs.begin_offset;
			    }
			const_iterator operator+=(const int &n) {
				//TODO
			        *this = *this + n;
			        return *this;
			}
			const_iterator operator-=(const int &n) {
				//TODO
			        *this = *this - n;
			        return *this;
			}
			const_iterator operator++(int) {
			        const_iterator tmp = *this;
			        ++(*this);
			        return tmp;
			    }
	                const_iterator& operator++() {
			        if (!dq || !node || (node == dq->map + dq->last && ptr == last))
			            throw invalid_iterator();

			        ++ptr;
			        ++begin_offset;
			        if (ptr == last) {
			            Chunk** next_node = node + 1;
			            while (next_node <= dq->map + dq->last &&
                                           (*next_node == nullptr || (*next_node)->first == (*next_node)->last)) {
			                next_node++;
                                           }

			            if (next_node <= dq->map + dq->last) {
			                node = next_node;
			                first = (*node)->first;
			                last = (*node)->last;
			                ptr = first;
			            } else {
			                node = dq->map + dq->last;
			                ptr = last;
			            }
			        }

			        return *this;
			    }

			const_iterator operator--(int) {
			        const_iterator tmp = *this;
			        --(*this);
			        return tmp;
			    }
	                const_iterator& operator--() {
			        if (!dq || !node || (node == dq->map + dq->start && ptr == first))
			            throw invalid_iterator();
			        --begin_offset;

			        if (ptr == first) {
			            Chunk** prev_node = node - 1;
			            while (prev_node >= dq->map + dq->start &&
                                           (*prev_node == nullptr || (*prev_node)->first == (*prev_node)->last)) {
			                prev_node--;
                                           }

			            if (prev_node >= dq->map + dq->start) {
			                node = prev_node;
			                first = (*node)->first;
			                last = (*node)->last;
			                ptr = last - 1;
			            } else {
			                ptr = first;
			            }
			        } else {
			            --ptr;
			        }

			        return *this;
			    }
			const T& operator*() const {
			        if (!dq || !node || ptr == last) throw invalid_iterator();
			        return *ptr;
			    }
			const T* operator->() const noexcept {
			        if (!dq || !node || ptr == last) throw invalid_iterator();
			        return ptr;
			    }
			bool operator==(const iterator &rhs) const {
			        return dq == rhs.dq && node == rhs.node && ptr == rhs.ptr;
			    }
			bool operator==(const const_iterator &rhs) const {
			        return dq == rhs.dq && node == rhs.node && ptr == rhs.ptr;
			    }
			bool operator!=(const iterator &rhs) const {
			        return !(*this == rhs);
			    }
			bool operator!=(const const_iterator &rhs) const {
			        return !(*this == rhs);
			    }
	};
	/**
	 * TODO Constructors
	 */
	deque():map_size(map_base), sum(0) {
	    map = static_cast<Chunk**>(::operator new (sizeof(Chunk*) * map_base));

	    for (int i = 0; i < map_size; ++i) {
	        map[i] = nullptr;
	    }

	    int mid = map_base / 2;
	    start = last = mid;
	}
	deque(const deque &other):map_size(other.map_size), start(other.start), last(other.last), sum(other.sum) {
	    map = static_cast<Chunk**>(::operator new (sizeof(Chunk*) * other.map_size));
	    for (int i = 0; i < map_size; ++i) {
	        map[i] = nullptr;
	    }
	    for (int i = start; i <= last; ++i) {
	        if (other.map[i]) {

	            map[i] = new Chunk();

	            Chunk* src = other.map[i];
	            Chunk* dst = map[i];

	            size_t first_offset = src->first - src->arr;
	            size_t last_offset  = src->last  - src->arr;

	            dst->first = dst->arr + first_offset;
	            dst->last  = dst->arr + last_offset;

	            T* s = src->first;
	            T* d = dst->first;

	            while (s != src->last) {
	                new (d) T(*s);
	                ++s;
	                ++d;
	            }
	        }
	    }
	}
	/**
	 * TODO Deconstructor
	 */
	~deque() {
	    for (int i = 0; i < map_size; ++i) {
	        if (map[i]) {
	            delete map[i];
	        }
	    }
	    ::operator delete(map);
	}
	/**
	 * TODO assignment operator
	 */
	deque &operator=(const deque &other) {
	    if (this == &other) return *this;

	    for (int i = start; i <= last; ++i) {
	        if (map[i]) delete map[i];
	    }
	    ::operator delete(map);

	    map_size = other.map_size;
	    start = other.start;
	    last = other.last;
	    sum = other.sum;

	    map = static_cast<Chunk**>(::operator new(sizeof(Chunk*) * map_size));
	    for (int i = 0; i < map_size; ++i)   map[i] = nullptr;

	    for (int i = start; i <= last; ++i) {
	        if (other.map[i]) {
	            map[i] = new Chunk();

	            Chunk* src = other.map[i];
	            Chunk* dst = map[i];

	            size_t first_offset = src->first - src->arr;
	            size_t last_offset  = src->last  - src->arr;

	            dst->first = dst->arr + first_offset;
	            dst->last  = dst->arr + last_offset;

	            T* s = src->first;
	            T* d = dst->first;

	            while (s != src->last) {
	                new (d) T(*s);
	                ++s;
	                ++d;
	            }
	        }
	    }

	    return *this;
	}
	/**
	 * access specified element with bounds checking
	 * throw index_out_of_bound if out of bound.
	 */
    T & at(const size_t &pos) {
	    if (pos >= sum) throw index_out_of_bound();
	    //第一个块可能不满
	    size_t offset = pos + (map[start]->first - map[start]->arr);

	    int chunk_id = start + offset / chunk_size;
	    int inner = offset % chunk_size;

	    return map[chunk_id]->arr[inner];
	}
	const T & at(const size_t &pos) const {
	    if (pos >= sum) throw index_out_of_bound();

	    size_t offset = pos + (map[start]->first - map[start]->arr);

	    int chunk_id = start + offset / chunk_size;
	    int inner = offset % chunk_size;

	    return map[chunk_id]->arr[inner];
	}
	T & operator[](const size_t &pos) {
	    if (pos >= sum) throw index_out_of_bound();

	    size_t offset = pos + (map[start]->first - map[start]->arr);

	    int chunk_id = start + offset / chunk_size;
	    int inner = offset % chunk_size;

	    return map[chunk_id]->arr[inner];
	}
	const T & operator[](const size_t &pos) const {
	    if (pos >= sum) throw index_out_of_bound();

	    size_t offset = pos + (map[start]->first - map[start]->arr);

	    int chunk_id = start + offset / chunk_size;
	    int inner = offset % chunk_size;

	    return map[chunk_id]->arr[inner];
	}
	/**
	 * access the first element
	 * throw container_is_empty when the container is empty.
	 */
	const T & front() const {
	    if (empty()) throw container_is_empty();

	    return *(map[start]->first);
	}
	/**
	 * access the last element
	 * throw container_is_empty when the container is empty.
	 */
	const T & back() const {
	    if (empty()) throw container_is_empty();
	    return *(map[last]->last - 1);
	}
	/**
	 * returns an iterator to the beginning.
	 */
	iterator begin() {
	    if (empty()) return end();

	    Chunk* c = map[start];
	    return iterator(c->first, c->first, c->last, map + start, this, 0);
	}
        const_iterator begin() const {
	    if (empty()) return end();
	    Chunk* c = map[start];
	    return const_iterator(c->first, c->first, c->last, map + start, this, 0);
	}
	const_iterator cbegin() const {
	    if (empty()) return end();

	    Chunk* c = map[start];
	    return const_iterator(c->first, c->first, c->last, map + start, this, 0);
	}
	/**
	 * returns an iterator to the end.
	 */
	iterator end() {
	    if (empty())
	        return iterator(nullptr, nullptr, nullptr, map + start, this);

	    Chunk* c = map[last];
	    return iterator(c->last, c->first, c->last, map + last, this, sum);
	}
        const_iterator end() const {
	    if (empty())
	        return const_iterator(nullptr, nullptr, nullptr, map + start, this, 0);

	    Chunk* c = map[last];
	    return const_iterator(c->last, c->first, c->last, map + last, this, sum);
	}
	const_iterator cend() const {
            if (empty())
                return const_iterator(nullptr, nullptr, nullptr, map + start, this, 0);

	    Chunk* c = map[last];
            return const_iterator(c->last, c->first, c->last, map + last, this, sum);
        }
	/**
	 * checks whether the container is empty.
	 */
	bool empty() const {
	    return sum == 0;
	}
	/**
	 * returns the number of elements
	 */
	size_t size() const {
	    return sum;
	}
	/**
	 * clears the contents
	 */
    void clear() {
	    for (int i = start; i <= last; ++i) {
	        if (map[i]) {
	            delete map[i];
	            map[i] = nullptr;
	        }
	    }

	    start = last = map_size / 2;
	    sum = 0;
	}
	/**
	 * inserts elements at the specified locat on in the container.
	 * inserts value before pos
	 * returns an iterator pointing to the inserted value
	 *     throw if the iterator is invalid or it point to a wrong place.
	 */
        iterator insert(iterator pos, const T &value) {
	    if (pos.dq != this)
	        throw invalid_iterator();

	    if (pos.begin_offset > sum)
	        throw invalid_iterator();

	    size_t rank = pos.begin_offset;

	    if (rank > sum) throw invalid_iterator();

	    if (empty()) {
	        push_back(value);
	        return begin();
	    }

	    if (rank <= sum / 2) {
	        push_front(front());

	        iterator it = begin();
	        ++it;

	        iterator target = begin() + rank;

	        while (it != target + 1) {
	            *(it - 1) = *it;
	            ++it;
	        }

	        *(begin() + rank) = value;

	        return begin() + rank;
	    }
	    else {
	        push_back(back());

	        iterator it = end() - 1;
	        iterator target = begin() + rank;

	        while (it != target) {
	            *it = *(it - 1);
	            --it;
	        }

	        *target = value;

	        return target;
	    }
	}
	/**
	 * removes specified element at pos.
	 * removes the element at pos.
	 * returns an iterator pointing to the following element, if pos pointing to the last element, end() will be returned.
	 * throw if the container is empty, the iterator is invalid or it points to a wrong place.
	 */
        iterator erase(iterator pos) {
	    if (empty()) throw container_is_empty();

	    if (pos.dq != this || pos == end())
	        throw invalid_iterator();

	    if (pos.begin_offset >= sum)
	        throw invalid_iterator();

	    size_t rank = pos.begin_offset;
	    iterator target = pos;
	    if (rank < sum / 2) {
	        iterator it = pos;

	        while (it != begin()) {
	            *it = *(it - 1);
	            --it;
	        }

	        pop_front();

	        return begin() + rank;
	    }
	    else {
	        iterator it = pos;
	        iterator nxt = pos;
	        ++nxt;

	        while (nxt != end()) {
	            *it = *nxt;
	            ++it;
	            ++nxt;
	        }
	        pop_back();
	        if (rank >= sum) return end();

	        return begin() + rank;
	    }
	}
	/**
	 * adds an element to the end
	 */
    void push_back(const T &value) {
	    if (empty()) {
	        map[start] = new Chunk();
	        map[start]->first = map[start]->arr;
	        map[start]->last  = map[start]->arr;
	    }

	    Chunk* c = map[last];
	    //最后一个块刚好满了
	    if (c->last == c->arr + chunk_size) {
	        if (last == map_size - 1) map_extend();
	        last++;

	        if (map[last] == nullptr)
	            map[last] = new Chunk();

	        c = map[last];
	        c->first = c->arr;
	        c->last  = c->arr;
	    }

	    new (c->last) T(value);
	    ++c->last;
	    ++sum;
	}
	/**
	 * removes the last element
	 *     throw when the container is empty.
	 */
    void pop_back() {
	    if (empty()) throw container_is_empty();

	    Chunk* c = map[last];

	    --c->last;
	    c->last->~T();

	    --sum;
	    //减完之后块空了
	    if (c->last == c->first) {
	        delete c;
	        map[last] = nullptr;

	        if (start != last) {
	            last--;
	        } else {
	            //一个不剩，复位
	            start = last = map_size / 2;
	        }
	    }
	}
	/**
	 * inserts an element to the beginning.
	 */
    void push_front(const T &value) {
	    if (empty()) {
	        map[start] = new Chunk();
	        map[start]->first = map[start]->arr + chunk_size/2;
	        map[start]->last  = map[start]->arr + chunk_size/2;
	    }

	    Chunk* c = map[start];

	    if (c->first == c->arr) {
	        if (start == 0) map_extend();

	        start--;

	        if (map[start] == nullptr)
	            map[start] = new Chunk();

	        c = map[start];
	        c->first = c->arr + chunk_size;
	        c->last  = c->arr + chunk_size;
	    }

	    --c->first;
	    new (c->first) T(value);
	    ++sum;
	}
	/**
	 * removes the first element.
	 *     throw when the container is empty.
	 */
    void pop_front() {
	    if (empty()) throw container_is_empty();

	    Chunk* c = map[start];

	    c->first->~T();
	    ++c->first;

	    --sum;

	    if (c->first == c->last) {
	        delete c;
	        map[start] = nullptr;

	        if (start != last)
	            start++;
	        else
	            start = last = map_size / 2;
	    }
	}
};

}

#endif