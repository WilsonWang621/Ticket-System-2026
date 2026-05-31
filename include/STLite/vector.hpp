#ifndef SJTU_VECTOR_HPP
#define SJTU_VECTOR_HPP

#include "exceptions.hpp"

#include <climits>
#include <cstddef>

namespace sjtu
{ 
/**
 * a data container like std::vector
 * store data in a successive memory and support random access.
 */
template<typename T>
class vector
{
public:
	/**cd
	 * TODO
	 * a type for actions of the elements of a vector, and you should write
	 *   a class named const_iterator with same interfaces.
	 */
	/**
	 * you can see RandomAccessIterator at CppReference for help.
	 */
	class const_iterator;
	class iterator
	{
	// The following code is written for the C++ type_traits library.
	// Type traits is a C++ feature for describing certain properties of a type.
	// For instance, for an iterator, iterator::value_type is the type that the
	// iterator points to.
	// STL algorithms and containers may use these type_traits (e.g. the following
	// typedef) to work properly. In particular, without the following code,
	// @code{std::sort(iter, iter1);} would not compile.
	// See these websites for more information:
	// https://en.cppreference.com/w/cpp/header/type_traits
	// About value_type: https://blog.csdn.net/u014299153/article/details/72419713
	// About iterator_category: https://en.cppreference.com/w/cpp/iterator
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using iterator_category = std::output_iterator_tag; 

	private:
		/**
		 * TODO add data members
		 *   just add whatever you want.
		 */
		
		 T* ptr1;
		 vector<T>* vec;

	public:
		iterator(T *p = nullptr, vector<T> *v = nullptr) : ptr1(p), vec(v) {}
		/**
		 * return a new iterator which pointer n-next elements
		 * as well as operator-
		 */
		iterator operator+(const int &n) const{
		    //TODO
		    return iterator(ptr1 + n, vec);
		}
		iterator operator-(const int &n) const{
		    //TODO
		    return iterator(ptr1 - n, vec);
		}
		// return the distance between two iterators,
		// if these two iterators point to different vectors, throw invaild_iterator.
		int operator-(const iterator &rhs) const{
		    //TODO
		    if(vec != rhs.vec){
		        throw invalid_iterator();
		    }

		    return ptr1 - rhs.ptr1;
		}
		iterator& operator+=(const int &n){
		    //TODO
		    ptr1 += n;
		    return *this;
		}
		iterator& operator-=(const int &n){
		    //TODO
		    ptr1 -= n;
		    return *this;
		}
		/**
		 * TODO iter++
		 */
		iterator operator++(int) {
		    iterator tmp = *this;
		    ptr1++;
		    return tmp;
		}
		/**
		 * TODO ++iter
		 */
		iterator& operator++() {
		    ptr1++;
		    return *this;
		}
		/**
		 * TODO iter--
		 */
		iterator operator--(int) {
		    iterator tmp = *this;
		    ptr1--;
		    return tmp;
		}
		/**
		 * TODO --iter
		 */
		iterator& operator--() {
		    ptr1--;
		    return *this;
		}
		/**
		 * TODO *it
		 */
		T& operator*() const{
		    return *ptr1;
		}
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory address).
		 */
		bool operator==(const iterator &rhs) const {
		    return ptr1 == rhs.ptr1 && vec == rhs.vec;
		}
		bool operator==(const const_iterator &rhs) const {
		    return ptr1 == rhs.ptr1 && vec == rhs.vec;
		}
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const {
		    return ptr1 != rhs.ptr1 || vec != rhs.vec;
		}
		bool operator!=(const const_iterator &rhs) const {
		    return ptr1 != rhs.ptr1 || vec != rhs.vec;
		}
	};
	/**
	 * TODO
	 * has same function as iterator, just for a const object.
	 */
	class const_iterator
	{
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using iterator_category = std::output_iterator_tag;

	private:
		/*TODO*/
        const T *ptr;
        const vector<T> *vec;
	public:
        const_iterator(const T *p = nullptr, const vector<T> *v = nullptr) : ptr(p), vec(v) {}
        
        const_iterator(const iterator &it) : ptr(it.ptr), vec(it.vec) {}

        const_iterator operator+(const int &n) const{
            return const_iterator(ptr + n, vec);
        }

        const_iterator operator-(const int &n) const{
            return const_iterator(ptr - n, vec);
        }
	int operator-(const const_iterator &rhs) const{
            if (vec != rhs.vec)
                throw invalid_iterator();
            return ptr - rhs.ptr;
        }

        const_iterator& operator+=(const int &n){
            ptr += n;
            return *this;
        }

	const_iterator& operator-=(const int &n){
            ptr -= n;
            return *this;
        }

        const_iterator operator++(int)
        {
            const_iterator tmp = *this;
            ptr++;
            return tmp;
        }

        const_iterator& operator++()
        {
            ptr++;
            return *this;
        }

	const_iterator operator--(int){
            const_iterator tmp = *this;
            ptr--;
            return tmp;
        }

        const_iterator& operator--(){
            ptr--;
            return *this;
        }

        const T& operator*() const{
            return *ptr;
        }

        const T* operator->() const{
            return ptr;
        }

	bool operator==(const iterator &rhs) const{
            return ptr == rhs.ptr;
        }

        bool operator==(const const_iterator &rhs) const{
            return ptr == rhs.ptr;
        }

        bool operator!=(const iterator &rhs) const{
            return ptr != rhs.ptr;
        }

        bool operator!=(const const_iterator &rhs) const{
            return ptr != rhs.ptr;
        }
	};
	/**
	 * TODO Constructs
	 * At least two: default constructor, copy constructor
	 */
	private:
            T* p;
	    int current_size;
            int capacity;
            static const int base = 8;

        void extend(){
            int new_capacity = capacity * 2;

            T* ptr = static_cast<T*>(::operator new(sizeof(T) * new_capacity));

            for(int i = 0; i < current_size; i++){
                new (ptr + i) T(std::move(p[i]));
            }

            for(int i = 0; i < current_size; i++){
                p[i].~T();
            }

            ::operator delete(p);

            p = ptr;
            capacity = new_capacity;
        }
    public:
	vector() {
            p = static_cast<T*>(::operator new(sizeof(T) * base));
            current_size = 0;
            capacity = base;
	}
	vector(const vector &other) {
	    current_size = other.current_size;
            capacity = other.capacity;
	    p = static_cast<T*>(::operator new(sizeof(T) * capacity));

            for(int i = 0; i < current_size; i++){
                new (p + i) T(other.p[i]);
            }
	}
	/**
	 * TODO Destructor
	 */
	~vector() {
            for(int i = 0; i < current_size; i++){
                p[i].~T();
            }
            ::operator delete(p);
	}
	/**
	 * TODO Assignment operator
	 */

	vector &operator=(const vector &other) {
	    if(this == &other) return *this;

	    for(int i = 0; i < current_size; i++){
	        p[i].~T();
	    }

	    ::operator delete(p);
	    current_size = other.current_size;
            capacity = other.capacity;

            p = static_cast<T*>(::operator new(sizeof(T) * capacity));

            for(int i = 0; i < current_size; i++){
                new (p + i) T(other.p[i]);
            }

	    return *this;
	}
	/**
	 * assigns specified element with bounds checking
	 * throw index_out_of_bound if pos is not in [0, size)
	 */
	T & at(const size_t &pos) {
	    if(pos >= current_size || pos < 0){
	        throw index_out_of_bound();
	    }

	    return p[pos];
	}
	const T & at(const size_t &pos) const {
	    if(pos >= current_size || pos < 0){
	        throw index_out_of_bound();
	    }

	    return p[pos];
	}
	/**
	 * assigns specified element with bounds checking
	 * throw index_out_of_bound if pos is not in [0, size)
	 * !!! Pay attentions
	 *   In STL this operator does not check the boundary but I want you to do.
	 */
	T & operator[](const size_t &pos) {
	    if(pos >= current_size || pos < 0){
	        throw index_out_of_bound();
	    }

	    return p[pos];
	}
	const T & operator[](const size_t &pos) const {
	    if(pos >= current_size || pos < 0){
	        throw index_out_of_bound();
	    }

	    return p[pos];
	}
	/**
	 * access the first element.
	 * throw container_is_empty if size == 0
	 */
	const T & front() const {
	    if(current_size == 0){
	        throw container_is_empty();
	    }

	    return p[0];
	}
	/**
	 * access the last element.
	 * throw container_is_empty if size == 0
	 */
	const T & back() const {
	    if(current_size == 0){
	        throw container_is_empty();
	    }

	    return p[current_size - 1];
	}
	/**
	 * returns an iterator to the beginning.
	 */
	iterator begin() {
	    return iterator(p , this);
	}
	const_iterator begin() const {
	    return iterator(p , this);
	}
	const_iterator cbegin() const {
	    return const_iterator(p , this);
	}
	/**
	 * returns an iterator to the end.
	 */
	iterator end() {
	    return iterator(p + current_size, this);
	}
	const_iterator end() const {
	    return const_iterator(p + current_size, this);
	}
	const_iterator cend() const {
	    return const_iterator(p + current_size, this);
	}
	/**
	 * checks whether the container is empty
	 */
	bool empty() const {
	    return current_size == 0;
	}
	/**
	 * returns the number of elements
	 */
	size_t size() const {
	    return current_size;
	}
	/**
	 * clears the contents
	 */
	void clear() {
            for(int i = 0; i < current_size; i++){
                p[i].~T();
            }
            current_size = 0;
	}
	/**
	 * inserts value before pos
	 * returns an iterator pointing to the inserted value.
	 */
	iterator insert(iterator pos, const T &value) {
	    int index = pos - begin();
	    return insert(index, value);
	}
	/**
	 * inserts value at index ind.
	 * after inserting, this->at(ind) == value
	 * returns an iterator pointing to the inserted value.
	 * throw index_out_of_bound if ind > size (in this situation ind can be size because after inserting the size will increase 1.)
	 */
	iterator insert(const size_t &ind, const T &value) {
	    if(ind > current_size){
	        throw index_out_of_bound();
	    }

	    if(current_size == capacity) {
	        extend();
	    }
	    if (ind < current_size) {
	        new (p + current_size) T(std::move(p[current_size - 1]));

	        for(int i = current_size - 1; i > ind; i--) {
	            p[i] = std::move(p[i-1]);
	        }
	    }

	    if (ind == current_size) {
	        new (p + ind) T(value);
	    } else {
	        p[ind].~T();
	        p[ind] = value;
	    }
	    current_size++;
	    return iterator(p + ind, this);
	}
	/**
	 * removes the element at pos.
	 * return an iterator pointing to the following element.
	 * If the iterator pos refers the last element, the end() iterator is returned.
	 */
	iterator erase(iterator pos) {
	    int index = pos - begin();
	    return erase(index);
	}
	/**
	 * removes the element with index ind.
	 * return an iterator pointing to the following element.
	 * throw index_out_of_bound if ind >= size
	 */
	iterator erase(const size_t &ind) {
	    if(ind >= current_size){
	        throw index_out_of_bound();
	    }

            p[ind].~T();
	    for(int i = ind; i < current_size; i++){
	        new (p + i) T(std::move(p[i+1]));
	        p[i+1].~T();
	    }

	    current_size--;
	    return iterator(p + ind, this);
	}
	/**
	 * adds an element to the end.
	 */
	void push_back(const T &value) {
	    new (p + current_size) T(value);
	    current_size++;
	    if(current_size == capacity){
	        extend();
	    }
	}
	/**
	 * remove the last element from the end.
	 * throw container_is_empty if size() == 0
	 */
	void pop_back() {
	    if (current_size == 0)   throw container_is_empty();

	    p[--current_size].~T();
	}
    };
}

#endif