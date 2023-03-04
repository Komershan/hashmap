#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <vector>
template <class KeyType, class ValueType> class HashMapElement {
public:
  std::pair<KeyType, ValueType> item;
  bool empty = true, deleted = false;
  HashMapElement(KeyType key, ValueType value) {
    item.first = key;
    item.second = value;
    empty = true;
  }
  HashMapElement() {}
};

template <class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap : public HashMapElement<KeyType, ValueType> {
protected:
  const size_t _primes_list[21] = {
      5,     11,     17,     37,     67,      131,     257,
      521,   1031,   2053,   4099,   8209,    16411,   32771,
      65537, 131101, 262147, 524309, 1048583, 2097169, 4194319};

  std::vector<HashMapElement<KeyType, ValueType>> _data;
  int _f_a, _f_b, _g_a, _g_b;
  size_t _table_size = 2, _size_index = 0, _deleted = 0, _inserted = 0,
         load_factor = 2;
  Hash hasher = Hash();
  std::mt19937_64 rnd;

  void _regenerate_functions() {
    _f_a = rnd() % (_primes_list[_size_index] - 1) + 1;
    _f_b = rnd() % (_primes_list[_size_index] - 1) + 1;
    _g_a = rnd() % (_primes_list[_size_index] - 1) + 1;
    _g_b = rnd() % (_primes_list[_size_index] - 1) + 1;
    return;
  }
  inline size_t _f(KeyType x) const {
    return ((static_cast<int64_t>(hasher(x)) % _primes_list[_size_index]) *
                _f_a +
            static_cast<int64_t>(_f_b)) %
           _primes_list[_size_index];
  }
  inline size_t _g(KeyType x) const {
    return (hasher(x) % (_primes_list[_size_index] - 1) * _g_a) % (_primes_list[_size_index] - 1) + 1;
  }

  inline size_t _find_index(const KeyType &x) const {
    size_t index = _f(x);
    if (!_data[index].empty && !_data[index].deleted &&
        _data[index].item.first == x) {
      return index;
    }
    index = (index + _g(x)) % _primes_list[_size_index];
    while (index != _f(x) && !_data[index].empty) {
      if (!_data[index].empty && !_data[index].deleted &&
          _data[index].item.first == x) {
        return index;
      }
      index = (index + _g(x)) % _primes_list[_size_index];
    }
    return _data.size();
  }
  inline void _naive_insert(const std::pair<KeyType, ValueType> &x) {
    size_t index = _f(x.first);

    if (!(_data[index].empty || _data[index].item.first == x.first)) {
      index = (index + _g(x.first)) % _primes_list[_size_index];
      while (index != _f(x.first) &&
             !(_data[index].empty || _data[index].item.first == x.first)) {
        index = (index + _g(x.first)) % _primes_list[_size_index];
      }
    }

    _inserted++;

    _data[index].empty = false;
    _data[index].deleted = false;
    _data[index].item = x;

    return;
  }
  void _rebuild() {
    std::vector<std::pair<KeyType, ValueType>> hashmap_elements;
    _regenerate_functions();
    for (iterator it = begin(); it != end(); ++it) {
      hashmap_elements.push_back(*it);
    }

    _deleted = _inserted = 0;
    for (size_t index = 0; index < 21; ++index) {
      if (hashmap_elements.size() * load_factor <= _primes_list[index]) {
        _table_size = _primes_list[index];
        _size_index = index;
        break;
      }
    }

    if (_data.size()) {
      _data.clear();
    }
    _data.resize(_table_size);

    for (auto x : hashmap_elements) {
      _naive_insert(x);
    }

    return;
  }

public:
  template <typename MapType, typename RetValue> class base_iterator {
  private:
    auto get_cur_pointer() {
      return reinterpret_cast<RetValue *>(&(_ptr->_data[_index].item));
    }
    MapType *_ptr = nullptr;
    friend MapType;

  public:
    size_t _index = 0;
    explicit base_iterator() : _ptr(nullptr) {}

    explicit base_iterator(MapType *ptr) : _ptr(ptr) {
      while (_index < _ptr->_data.size() &&
             (_ptr->_data[_index].empty || _ptr->_data[_index].deleted)) {
        ++_index;
      }
    }

    explicit base_iterator(MapType *ptr, size_t index)
        : _ptr(ptr), _index(index) {}

    template <typename OtherMapType, typename OtherRetValue>
    explicit base_iterator(
        const base_iterator<OtherMapType, OtherRetValue> &other)
        : _ptr(other._ptr), _index(other._index) {}

    base_iterator &operator++() {
      ++_index;
      while (_index < _ptr->_data.size() &&
             (_ptr->_data[_index].empty || _ptr->_data[_index].deleted)) {
        ++_index;
      }
      return *this;
    }

    base_iterator operator++(int) {
      auto current = *this;
      ++_index;
      while (_index < _ptr->_data.size() &&
             (_ptr->_data[_index].empty || _ptr->_data[_index].deleted)) {
        ++_index;
      }
      return current;
    }

    bool operator==(const base_iterator &other) const {
      return other._ptr == _ptr && other._index == _index;
    }

    bool operator!=(const base_iterator &other) const {
      return !(other == *this);
    }

    RetValue &operator*() { return *get_cur_pointer(); }

    RetValue *operator->() { return get_cur_pointer(); }

    ~base_iterator() {}
  };

  using iterator = base_iterator<HashMap, std::pair<const KeyType, ValueType>>;
  using const_iterator =
      base_iterator<const HashMap, const std::pair<const KeyType, ValueType>>;

  const_iterator begin() const { return const_iterator(this); }

  const_iterator end() const { return const_iterator(this, _data.size()); }

  iterator begin() { return iterator(this); }

  iterator end() { return iterator(this, _data.size()); }

  HashMap(std::initializer_list<std::pair<KeyType, ValueType>> list,
          Hash hasher = Hash(), int random_seed = time(NULL)) {
    rnd.seed(random_seed);
    this->hasher = hasher;
    _rebuild();
    for (auto element : list) {
      insert(element);
    }
  }

  HashMap(const_iterator begin, const_iterator end, Hash hasher = Hash(),
          int random_seed = time(NULL)) {
    rnd.seed(random_seed);
    this->hasher = hasher;
    _rebuild();
    for (begin; begin != end; begin++) {
      insert(*begin);
    }
  }

  HashMap(iterator begin, iterator end, Hash hasher = Hash(),
          int random_seed = time(NULL)) {
    rnd.seed(random_seed);
    this->hasher = hasher;
    _rebuild();
    for (iterator it = begin; it != end; it++) {
      insert(*it);
    }
  }

  HashMap(Hash hasher = Hash(), int random_seed = time(NULL)) : hasher(hasher) {
    rnd.seed(random_seed);
    _rebuild();
  }

  HashMap(const HashMap<KeyType, ValueType, Hash> &x) {
    this->_data = x._data;
    this->_table_size = x._table_size;
    this->_f_a = x._f_a;
    this->_f_b = x._f_b;
    this->_g_a = x._g_a;
    this->_g_b = x._g_b;
    this->_inserted = x._inserted;
    this->_deleted = x._deleted;
    this->hasher = x.hasher;
    this->rnd = x.rnd;
    this->_size_index = x._size_index;
  }

  HashMap<KeyType, ValueType, Hash> &
  operator=(const HashMap<KeyType, ValueType, Hash> &x) {
    this->_data = x._data;
    this->_table_size = x._table_size;
    this->_f_a = x._f_a;
    this->_f_b = x._f_b;
    this->_g_a = x._g_a;
    this->_g_b = x._g_b;
    this->_inserted = x._inserted;
    this->_deleted = x._deleted;
    this->hasher = x.hasher;
    this->rnd = x.rnd;
    this->_size_index = x._size_index;
    return *this;
  }

  Hash hash_function() const { return hasher; }

  iterator insert(std::pair<KeyType, ValueType> x) {
    if (_find_index(x.first) != _data.size()) {
      return find(x.first);
    }
    _naive_insert(x);
    if (_inserted * load_factor >= _table_size) {
      _rebuild();
    }
    return find(x.first);
  }

  void erase(KeyType x) {
    size_t index = _find_index(x);
    if (index != _data.size() && _data[index].item.first == x &&
        !_data[index].empty) {
      _deleted++;
      _data[index].deleted = true;
    }
    return;
  }

  iterator find(KeyType x) {
    if (!_data.size()) {
      return iterator(this, _data.size());
    }
    size_t index = _find_index(x);
    if (index == _data.size()) {
      return iterator(this, _data.size());
    } else {
      return iterator(this, index);
    }
  }

  const_iterator find(KeyType x) const {
    if (!_data.size()) {
      return end();
    }
    size_t index = _find_index(x);
    if (index == _data.size()) {
      return end();
    } else {
      return const_iterator(this, index);
    }
  }

  ValueType &operator[](KeyType key) {
    iterator it = find(key);
    if (it == end()) {
      it = insert(std::pair<KeyType, ValueType>{key, ValueType()});
    }
    return it->second;
  }

  const ValueType &at(const KeyType &key) const {
    const_iterator it = find(key);
    if (it == end()) {
      throw std::out_of_range("invalid at() work");
    }
    return it->second;
  }

  size_t size() const { return _inserted - _deleted; }

  bool empty() const { return _inserted - _deleted == 0; }

  void clear() { *this = HashMap<KeyType, ValueType, Hash>(hasher); }

  ~HashMap() { _data.clear(); }
};
