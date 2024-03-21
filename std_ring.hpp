#include <memory>
#include <cassert>
#include <iostream>
#include <type_traits>
#include <ranges>
#include <iterator>
#include <cstring>
#include <algorithm>

#ifndef _RING_HPP
#define _RING_HPP 1

namespace std {

    // Please remember this only takes an allocator in its constructor
    template<typename _Tp, size_t _N, typename _Alloc>
    struct _Ring_base {
    
    typedef typename std::allocator_traits<_Alloc>::template rebind_alloc<_Tp> _Tp_alloc_type; // Get the allocator, then 'set' the allocator to our type

    struct _Ring_impl : _Tp_alloc_type {

        typename std::allocator_traits<_Tp_alloc_type>::pointer _M_start;
        typename std::allocator_traits<_Tp_alloc_type>::pointer _M_end;
        typename std::allocator_traits<_Tp_alloc_type>::pointer _M_head;
        typename std::allocator_traits<_Tp_alloc_type>::pointer _M_tail;

        _Ring_impl() 
        : _Tp_alloc_type(),
        _M_head(0),
        _M_tail(0),
        _M_start(0),
        _M_end(0)
        { };

        explicit _Ring_impl(_Tp_alloc_type const& arg_alloc) 
        : _Tp_alloc_type(arg_alloc), // Only changed here
        _M_head(0),
        _M_tail(0),
        _M_start(0),
        _M_end(0)
        { };

    };

    public:
        typedef _Alloc allocator_type;

    _Tp_alloc_type& _M_get_Tp_allocator()
    { return *static_cast<_Tp_alloc_type*>(&this->_M_impl); } // Return typed allocator
 
    const _Tp_alloc_type& _M_get_Tp_allocator() const
    { return *static_cast<const _Tp_alloc_type*>(&this->_M_impl); } // Return constant typed allocator

    allocator_type get_allocator() const
    { return allocator_type(_M_get_Tp_allocator()); } // Return OUR typedef from the template allocator (default initialized)
 
    explicit _Ring_base(const allocator_type& arg_alloc) : _M_impl(arg_alloc) { // Same, but with custom allocator
        try {
            this->_M_impl._M_start = this->_M_allocate(_N); // Beginning of memory
        } catch (...) {
            // Ensure no resources are leaked if allocation fails
            _M_deallocate(this->_M_impl._M_start, _N);
            throw; // Rethrow the exception to signal failure
        }
        this->_M_impl._M_head = this->_M_impl._M_start; // Head and tail start at the same place
        this->_M_impl._M_tail = this->_M_impl._M_start;
        this->_M_impl._M_end = this->_M_impl._M_start + _N; // Offset by amount of elements
    }
 
    _Ring_base() : _M_impl() {
        try {
            this->_M_impl._M_start = this->_M_allocate(_N); // Beginning of memory
        } catch (...) {
            // Ensure no resources are leaked if allocation fails
            _M_deallocate(this->_M_impl._M_start, _N);
            throw; // Rethrow the exception to signal failure
        }
        this->_M_impl._M_head = this->_M_impl._M_start; // Head and tail start at the same place
        this->_M_impl._M_tail = this->_M_impl._M_start;
        this->_M_impl._M_end = this->_M_impl._M_start + _N; // Offset by amount of elements
    }

    _Ring_base(_Ring_base&& arg_other) noexcept : _M_impl(arg_other._M_get_Tp_allocator()) { // Move constructor, steal the others resources
        this->_M_impl._M_head = arg_other._M_impl._M_head;
        this->_M_impl._M_tail = arg_other._M_impl._M_tail;
        this->_M_impl._M_start = arg_other._M_impl._M_start;
        this->_M_impl._M_end = arg_other._M_impl._M_end;
        arg_other._M_impl._M_head = nullptr;
        arg_other._M_impl._M_tail = nullptr;
        arg_other._M_impl._M_start = nullptr;
        arg_other._M_impl._M_end = nullptr;
    }

    ~_Ring_base() { 
        _M_deallocate(this->_M_impl._M_start, _N); // Simplest deconstructor ever, just lets go of the memory, nothing better
    };

    public:

    _Ring_impl _M_impl;
 
    typename std::allocator_traits<_Tp_alloc_type>::pointer _M_allocate(size_t arg_n)
    { return arg_n != 0 ? _M_impl.allocate(arg_n) : 0; } // Quick null pointer check
 
    void _M_deallocate(typename std::allocator_traits<_Tp_alloc_type>::pointer arg_ptr, size_t arg_n) {
    if (arg_ptr != nullptr) // Valid?
        _M_impl.deallocate(arg_ptr, arg_n);
    };

    }; // _Ring_base

// -------------------------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------------------------------------

    template<typename _Tp, size_t _N, typename _Alloc = std::allocator<_Tp>>
    class ring : protected _Ring_base<_Tp, _N, _Alloc> {

    typedef typename _Alloc::value_type _Alloc_value_type;

    typedef _Ring_base<_Tp, _N, _Alloc> _Base;
    typedef typename _Ring_base<_Tp, _N, _Alloc>::_Tp_alloc_type _Tp_alloc_type;

    template<typename _Up>
    class ring_iterator {
    public:
        using value_type = _Up;
        using difference_type = std::ptrdiff_t;
        using pointer = _Up*;
        using reference = _Up&;
        using iterator_category = std::bidirectional_iterator_tag;

    // Constructor
    ring_iterator(pointer arg_ptr, pointer arg_start, pointer arg_end) 
        : _M_ptr(arg_ptr), _M_start(arg_start), _M_end(arg_end) {}

        ring_iterator(const ring_iterator& other) 
        : _M_ptr(other._M_ptr), _M_start(other._M_start), _M_end(other._M_end) {}

        ring_iterator(ring_iterator&& other) 
            : _M_ptr(other._M_ptr), _M_start(other._M_start), _M_end(other._M_end) {
            other._M_ptr = nullptr;
            other._M_start = nullptr;
            other._M_end = nullptr;
        }

        // Overloaded operators
        reference operator*() const { 
            assert(_M_ptr >= _M_start && _M_ptr < _M_end && "Compile time bad-index access");
            return *_M_ptr; 
        }

        pointer operator->() const { 
            return &operator*(); 
        }

        ring_iterator operator++() {
            if (_M_ptr != _M_end) {
                ++_M_ptr;
            }
            return *this; 
        }

        ring_iterator operator++(int) { 
            ring_iterator temp = *this;
            operator++();
            return temp;
        }

        ring_iterator operator--() { 
            if (_M_ptr != _M_start) {
                --_M_ptr;
            }
            return *this; 
        }

        ring_iterator operator--(int) { 
            ring_iterator temp = *this;
            operator--();
            return temp; 
        }

        bool operator==(const ring_iterator& other) const { 
            return (_M_ptr == other._M_ptr);
        }
        
        bool operator!=(const ring_iterator& other) const { 
            return !(*this == other); 
        }

        void next(){
            if (++_M_ptr == _M_end) {
                _M_ptr = _M_start;
            }
        }

        void back() {
            if (_M_ptr == _M_start) {
                _M_ptr = _M_end - 1;
            } else {
                --_M_ptr;
            }
        }

        ~ring_iterator() = default;

    private:
        pointer _M_ptr;
        pointer _M_start;
        pointer _M_end;
    };

protected:

    using _Base::_M_allocate;
    using _Base::_M_deallocate;
    using _Base::_M_impl;
    using _Base::_M_get_Tp_allocator;

    static constexpr bool trivial = std::is_trivial<_Tp>::value;
    static constexpr bool not_trivial_equ_over_ptr = !std::is_trivial<_Tp>::value && (sizeof(_Tp) >= sizeof(void*));
    static constexpr bool not_trivial_under_ptr = !std::is_trivial<_Tp>::value && (sizeof(_Tp) <  sizeof(void*));

public:

    typedef _Tp value_type;
    typedef typename std::allocator_traits<_Tp_alloc_type>::pointer pointer;
    typedef typename std::allocator_traits<_Tp_alloc_type>::const_pointer const_pointer;
    typedef typename _Tp_alloc_type::value_type& reference;
    typedef const typename _Tp_alloc_type::value_type& const_reference;
// ----------------------------------------------------------------------------------------------------
    typedef std::size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef _Alloc allocator_type;

    typedef ring_iterator<_Tp> iterator;
    typedef ring_iterator<const _Tp> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    static const size_t size = _N;

    // Default constructor
    ring() noexcept : _Base() {};
    // Allocator constructor
    ring(const allocator_type& __a) noexcept : _Base(__a) {}
    // Value constructor
    ring(const value_type& __value) noexcept : _Base() {
        std::uninitialized_fill_n(this->_M_impl._M_start, _N, __value);
    }
    // Value and allocator constructor
    ring(const value_type& __value, const allocator_type& __a) noexcept : _Base(__a) { 
        std::uninitialized_fill_n(this->begin(), _N, __value);
    }
    // Copy constructor
    ring(const ring<_Tp, _N>& ring_other) noexcept : _Base(ring_other.get_allocator()){ 
        std::uninitialized_copy(ring_other.begin(), ring_other.end(), this->_M_impl._M_start);
    }
    // Move constructor
    ring(ring<_Tp, _N, _Alloc>&& __x) noexcept : _Base(std::move(__x)) {}
    // Ininitalizer list constructor
    ring(std::initializer_list<_Tp> __l, const allocator_type& __a = allocator_type()) noexcept : _Base(__a) { // cppcheck-suppress noExplicitConstructor
        assert(std::distance(__l.begin(), __l.end()) <= _N && "The initializer list is too large for the ring.");
        std::copy(__l.begin(), __l.end(), this->begin());
    };
    // Range constructor
    template <typename _It>
    ring(_It first, _It last, const allocator_type& __a = allocator_type()) noexcept : _Base(__a) {
        assert(std::distance(first, last) <= _N && "The range is too large for the ring.");
        std::copy(first, last, this->begin());
    }
    // C-Style array constructor
    explicit ring(_Tp* __ptr) noexcept : _Base() {
        #if __cplusplus > 201703L
        std::uninitialized_default_construct_n(this->_M_impl._M_start, _N);
        #else
        for (std::size_t _it = 0; _it < _N; ++_it) {
            new (this->_M_impl._M_start + _it) _Tp();
        }
        #endif
        std::copy(__ptr, (__ptr+_N), this->begin());
    }
    
    // Simple 'std::cout' printing
    friend std::ostream& operator<<(std::ostream& __os, const ring<_Tp, _N, _Alloc>& __x) {
        __os << "[";
        for (size_t _it = 0; _it < _N; ++_it) {
            __os << __x._M_impl._M_start[_it];
            if (_it != _N - 1) {
                __os << ", ";
            }
        }
        __os << "]";
        return __os;
    }

    friend bool operator==(const ring& lhs, const ring& rhs) {
        if (lhs.size != rhs.size) return false;
        for (size_t i = 0; i < lhs.size; ++i) {
            if (lhs._M_impl._M_start[i] != rhs._M_impl._M_start[i]) return false;
        }
        return true;
    }

    iterator begin() { 
        return iterator(_M_impl._M_start, _M_impl._M_start, _M_impl._M_end); 
    }
    iterator end() { 
        return iterator(_M_impl._M_end, _M_impl._M_start, _M_impl._M_end); 
    }
    const_iterator begin() const { 
        return const_iterator(_M_impl._M_start, _M_impl._M_start, _M_impl._M_end); 
    }
    const_iterator end() const { 
        return const_iterator(_M_impl._M_end, _M_impl._M_start, _M_impl._M_end); 
    }
    const_iterator cbegin() const { 
        return const_iterator(_M_impl._M_start, _M_impl._M_start, _M_impl._M_end); 
    }
    const_iterator cend() const { 
        return const_iterator(_M_impl._M_end, _M_impl._M_start, _M_impl._M_end); 
    }
    reverse_iterator rbegin() { 
        return reverse_iterator(end()); 
    }
    reverse_iterator rend() { 
        return reverse_iterator(begin()); 
    }
    const_reverse_iterator rbegin() const { 
        return const_reverse_iterator(end()); 
    }
    const_reverse_iterator rend() const { 
        return const_reverse_iterator(begin()); 
    }
    const_reverse_iterator crbegin() const { 
        return const_reverse_iterator(cend()); 
    }
    const_reverse_iterator crend() const { 
        return const_reverse_iterator(cbegin()); 
    }

    const _Tp& read_head() const {
        return *(this->_M_impl._M_head);
    };

    const _Tp& read_tail() const {
        return *(this->_M_impl._M_tail);
    };

    // Emplace functions for non-trivial types
    template <typename... _Args, typename _Cond = _Tp*>
    typename std::enable_if<!std::is_trivial<_Tp>::value, _Cond>::type emplace_head(_Args&&... __args) {
        if (_N == 0) {
            return nullptr;
        }
        #if __cplusplus > 201703L
        std::construct_at(this->_M_impl._M_head, std::forward<_Args>(__args)...);
        #else
        new (this->_M_impl._M_head) _Tp(std::forward<_Args>(__args)...);
        #endif
        return this->_M_impl._M_head;
    }

    // Emplace functions for trivial types
    template <typename... _Args, typename _Cond = _Tp*>
    typename std::enable_if<std::is_trivial<_Tp>::value, _Cond>::type emplace_head(_Args&&... __args) {
        if (_N == 0) {
            return nullptr;
        }
        std::memcpy(this->_M_impl._M_head, &__args..., sizeof(_Tp));
        return this->_M_impl._M_head;
    }
    
    // Emplace functions for non-trivial types
    template <typename... _Args, typename _Cond = _Tp*>
    typename std::enable_if<!std::is_trivial<_Tp>::value, _Cond>::type emplace_tail(_Args&&... __args) {
        if (_N == 0) {
            return nullptr;
        }
        #if __cplusplus > 201703L
        std::construct_at(this->_M_impl._M_tail, std::forward<_Args>(__args)...);
        #else
        new (this->_M_impl._M_tail) _Tp(std::forward<_Args>(__args)...);
        #endif
        return this->_M_impl._M_tail;
    }

    // Emplace functions for trivial types
    template <typename... _Args, typename _Cond = _Tp*>
    typename std::enable_if<std::is_trivial<_Tp>::value, _Cond>::type emplace_tail(_Args&&... __args) {
        if (_N == 0) {
            return nullptr;
        }
        std::memcpy(this->_M_impl._M_tail, &__args..., sizeof(_Tp));
        return this->_M_impl._M_tail;
    }

    // Step functions for head and tail
    void step_head() {
        if (++(this->_M_impl._M_head) == this->_M_impl._M_end) {
            this->_M_impl._M_head = this->_M_impl._M_start;
        }
    }

    void step_tail() {
        if (++(this->_M_impl._M_tail) == this->_M_impl._M_end) {
            this->_M_impl._M_tail = this->_M_impl._M_start;
        }
    }

    // Enter function for any types
    void enter(const _Tp& __arg) {
        if (_N == 0) {
            return;
        }
        emplace_head(__arg);
        step_head();
    }

    // Consume function for non-trivial types
    template <typename _Cond = _Tp>
    typename std::enable_if<!std::is_trivial<_Tp>::value, _Cond>::type consume() {
        _Tp __x = read_tail();
        this->_M_impl._M_tail->~_Tp();
        #if __cplusplus > 201703L
        std::construct_at(this->_M_impl._M_tail, _Tp());
        #else
        new (this->_M_impl._M_tail) _Tp();
        #endif
        step_tail();
        return __x;
    }

    // Consume function for trivial types
    template <typename _Cond = _Tp>
    typename std::enable_if<std::is_trivial<_Tp>::value, _Cond>::type consume() {
        _Tp __x = read_tail();
        std::memcpy(this->_M_impl._M_tail, &__x, sizeof(_Tp));
        step_tail();
        return __x;
    }

    // Rotate functions for left and right
    void rotate_left() {
        std::rotate(this->begin(), ++this->begin(), this->end());
    }

    void rotate_right() {
        std::rotate(this->rbegin(), ++this->rbegin(), this->rend());
    }

    // Destructor
    ~ring() = default;

    private:

    }; // End Ring

} // namespace std

#endif // _RING_HPP 