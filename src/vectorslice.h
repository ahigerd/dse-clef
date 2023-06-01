#ifndef SLICEREF_H
#define SLICEREF_H

#include <vector>
#include <type_traits>
#include <stdexcept>

template <typename T, typename Container = std::vector<T>>
class VectorSlice;

template <typename T, typename _Container = std::vector<T>, typename Iter = typename _Container::const_iterator>
class ConstVectorSlice
{
  friend class VectorSlice<T, _Container>;
public:
  enum { IsConst = std::is_same<Iter, typename _Container::const_iterator>::value };

  using ThisType = ConstVectorSlice<T, _Container, Iter>;
  using MutableType = VectorSlice<T, _Container>;
  using Container = typename std::conditional<IsConst, const _Container, _Container>::type;
  using size_type = typename Container::size_type;
  using difference_type = typename Container::difference_type;
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using pointer = typename Container::pointer;
  using const_pointer = typename Container::const_pointer;
  using iterator = typename Container::iterator;
  using const_iterator = typename Container::const_iterator;

  ConstVectorSlice() : _container(nullptr) {}
  ConstVectorSlice(const ThisType& other) = default;
  ConstVectorSlice(ThisType&& other) = default;
  ConstVectorSlice& operator=(const ThisType& other) = default;
  ConstVectorSlice& operator=(ThisType&& other) = default;
  ConstVectorSlice(const MutableType& other) : ConstVectorSlice(other._container, other._begin, other._end) {}
  ConstVectorSlice(MutableType&& other) : ConstVectorSlice(other._container, other._begin, other._end) {}

  ConstVectorSlice(const Iter& begin, const Iter end)
  : _container(nullptr), _begin(begin), _end(end), _offset(0) {
    if (end < begin) {
      throw std::out_of_range("invalid slice bounds");
    }
  }
  ConstVectorSlice(Container& container)
  : ThisType(&container, container.cbegin(), container.cend()) {}
  ConstVectorSlice(Container& container, int start)
  : ThisType(&container, container.cbegin() + start, container.cend(), start) {}
  ConstVectorSlice(Container& container, int start, int length)
  : ThisType(&container, container.cbegin() + start, container.cbegin() + start + length, start) {}

  operator _Container() const { return _Container(_begin, _end); }
  const_iterator begin() const { return _begin; }
  const_iterator cbegin() const { return _begin; }
  const_iterator end() const { return _end; }
  const_iterator cend() const { return _end; }

  size_type size() const { return _end - _begin; }

  const_reference operator[](size_type pos) const { return _container ? (*_container)[_offset + pos] : *(_begin + pos); }

protected:
  ConstVectorSlice(Container* container, const Iter& begin, const Iter& end, size_t offset = 0)
  : _container(container), _begin(begin), _end(end), _offset(offset) {
    if (end < begin) {
      throw std::out_of_range("invalid slice bounds");
    }
    if (container && (begin < container->begin() || end > container->end())) {
      throw std::out_of_range("slice exceeds container");
    }
  }

  Container* _container;
  Iter _begin, _end;
  size_t _offset;
};

template <typename T, typename Container>
class VectorSlice : public ConstVectorSlice<T, Container, typename Container::iterator> {
  friend class ConstVectorSlice<T, Container, typename Container::iterator>;
  friend class ConstVectorSlice<T, Container, typename Container::const_iterator>;
public:
  using Iter = typename Container::iterator;
  using BaseType = ConstVectorSlice<T, Container, Iter>;
  using ConstType = ConstVectorSlice<T, Container, typename Container::const_iterator>;
  using ThisType = VectorSlice<T, Container>;
  using size_type = typename Container::size_type;
  using difference_type = typename Container::difference_type;
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using pointer = typename Container::pointer;
  using const_pointer = typename Container::const_pointer;
  using iterator = typename Container::iterator;
  using const_iterator = typename Container::const_iterator;

  VectorSlice() : BaseType() {}
#if !defined(_MSC_VER) || _MSC_VER >= 1910
  VectorSlice(const ThisType& other) = default;
  VectorSlice(ThisType&& other) = default;
  VectorSlice& operator=(const ThisType& other) = default;
  VectorSlice& operator=(ThisType&& other) = default;
#else
  VectorSlice(const ThisType& other): BaseType(reinterpret_cast<const BaseType&>(other)) {}
  VectorSlice(ThisType&& other): BaseType(reinterpret_cast<BaseType&&>(other)) {}
  VectorSlice& operator=(const ThisType& other) { *this = (const BaseType&)other; }
  VectorSlice& operator=(ThisType&& other) { *this = std::forward((const BaseType&)other); }
#endif

  VectorSlice(const Iter& begin, const Iter end)
  : BaseType(nullptr, begin, end) {}
  VectorSlice(Container& container)
  : BaseType(&container, container.cbegin(), container.cend()) {}
  VectorSlice(Container& container, int start)
  : BaseType(&container, container.cbegin() + start, container.cend(), start) {}
  VectorSlice(Container& container, int start, int length)
  : BaseType(&container, container.cbegin() + start, container.cbegin() + start + length, start) {}

  iterator begin() { return this->_begin; }
  iterator end() { return this->_end; }

  reference operator[](size_type pos) { return this->_container ? (*this->_container)[this->offset + pos] : *(this->_begin + pos); }

  operator ConstType() const { return ConstType(this->_container, this->_begin, this->_end); }
};

#endif
