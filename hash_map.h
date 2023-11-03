#pragma once
#include <functional>
#include <stdexcept>

template <class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
public:
    HashMap(Hash hash = Hash()) : hash_(hash) {
        InitMemory(kInitialSize);
    }

    template <typename init_iterator>
    HashMap(init_iterator begin, init_iterator end, Hash hash = Hash()) : hash_(hash) {
        InitMemory(kInitialSize);
        for (auto it = begin; it != end; it++) {
            Insert(*it);
        }
    }

    HashMap(const std::initializer_list<std::pair<KeyType, ValueType>>& initial_list,
            Hash hash = Hash())
        : hash_(hash) {
        InitMemory(kInitialSize);
        for (auto it = initial_list.begin(); it != initial_list.end(); it++) {
            Insert(*it);
        }
    }

    HashMap(const HashMap& other)
        : hash_(other.hash_), size_(other.size_), capacity_(other.capacity_) {
        try {
            pairs_ = new std::pair<KeyType, ValueType>[other.capacity_];
            std::copy(other.pairs_, other.pairs_ + other.capacity_, pairs_);
            used_ = new uint8_t[other.capacity_];
            std::copy(other.used_, other.used_ + other.capacity_, used_);
        } catch (...) {
            delete[] pairs_;
            delete[] used_;
            throw;
        }
    }

    HashMap(HashMap&& other)
        : hash_(other.hash_),
          size_(other.size_),
          capacity_(other.capacity_),
          used_(other.used_),
          pairs_(other.pairs_) {
        other.used_ = nullptr;
        other.pairs_ = nullptr;
    }

    HashMap& operator=(const HashMap& other) {
        if (this == &other) {
            return *this;
        }
        ClearMemory();
        InitMemory(other.capacity_);

        for (size_t i = 0; i < capacity_; i++) {
            if (other.used_[i] == 1) {
                CheckOverload();
                CreatePair(i, other.pairs_[i].first, other.pairs_[i].second);
            }
        }
        return *this;
    }

    HashMap& operator=(HashMap&& other) {
        if (this == &other) {
            return *this;
        }
        ClearMemory();
        std::swap(used_, other.used_);
        std::swap(pairs_, other.pairs_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        std::swap(hash_, other.hash_);
        return *this;
    }

    ~HashMap() {
        delete[] used_;
        delete[] pairs_;
        used_ = nullptr;
        pairs_ = nullptr;
    }

    void Insert(const std::pair<KeyType, ValueType>& item) {
        CheckOverload();
        size_t index = FindPosition(item.first);
        if (used_[index] == 1) {
            return;
        }
        CreatePair(index, item.first, item.second);
    }

    void Erase(const KeyType& key) {
        size_t index = FindPosition(key);
        if (used_[index] != 1) {
            return;
        }
        DeletePair(index);
        CheckInsufficientLoad();
    }

    ValueType& operator[](const KeyType& key) {
        size_t index = FindPosition(key);
        if (used_[index] != 1) {
            if (CheckOverload()) {
                index = FindPosition(key);
            }
            CreatePair(index, key, ValueType{});
        }
        return pairs_[index].second;
    };

    const ValueType& At(const KeyType& key) const {
        size_t index = FindPosition(key);
        if (used_[index] != 1) {
            throw std::out_of_range("The key doesn't exist");
        }
        return pairs_[index].second;
    };

    size_t Size() const {
        return size_;
    }

    bool Empty() const {
        return size_ == 0;
    }

    void Clear() {
        ClearMemory();
        InitMemory(kInitialSize);
    }

    Hash HashFunction() const {
        return hash_;
    }

    class iterator {  // NOLINT
    public:
        iterator() : ptr_pair_(nullptr), ptr_used_(nullptr), end_used_(nullptr) {
        }
        iterator(std::pair<KeyType, ValueType>* ptr_pair, uint8_t* ptr_used, uint8_t* end_used)
            : ptr_pair_(reinterpret_cast<std::pair<const KeyType, ValueType>*>(ptr_pair)),
              ptr_used_(ptr_used),
              end_used_(end_used) {
        }

        std::pair<const KeyType, ValueType>& operator*() {
            return *ptr_pair_;
        }

        std::pair<const KeyType, ValueType>* operator->() {
            return ptr_pair_;
        }

        iterator& operator++() {
            ++ptr_pair_;
            while (++ptr_used_ != end_used_ && *(ptr_used_) != 1) {
                ++ptr_pair_;
            }
            return *this;
        }
        iterator operator++(int) {
            iterator cur = *this;
            ++ptr_pair_;
            while (++ptr_used_ != end_used_ && *(ptr_used_) != 1) {
                ++ptr_pair_;
            }
            return cur;
        }

        bool operator==(const iterator& other) const {
            return ptr_used_ == other.ptr_used_ && ptr_pair_ == other.ptr_pair_;
        }

        bool operator!=(const iterator& other) const {
            return ptr_used_ != other.ptr_used_ || ptr_pair_ != other.ptr_pair_;
        }

    private:
        std::pair<const KeyType, ValueType>* ptr_pair_ = nullptr;
        uint8_t *ptr_used_ = nullptr, *end_used_ = nullptr;
    };

    class const_iterator {  // NOLINT
    public:
        const_iterator() : ptr_pair_(nullptr), ptr_used_(nullptr), end_used_(nullptr) {
        }
        const_iterator(const std::pair<KeyType, ValueType>* ptr_pair, uint8_t* ptr_used,
                       uint8_t* end_used)
            : ptr_pair_(ptr_pair), ptr_used_(ptr_used), end_used_(end_used) {
        }

        const std::pair<KeyType, ValueType>& operator*() {
            return *ptr_pair_;
        }

        const std::pair<KeyType, ValueType>* operator->() {
            return ptr_pair_;
        }

        const_iterator& operator++() {
            ++ptr_pair_;
            while ((++ptr_used_) != end_used_ && *ptr_used_ != 1) {
                ++ptr_pair_;
            }
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator cur = *this;
            ++ptr_pair_;
            while ((++ptr_used_) != end_used_ && *ptr_used_ != 1) {
                ++ptr_pair_;
            }
            return cur;
        }

        bool operator==(const const_iterator& other) const {
            return ptr_used_ == other.ptr_used_ && ptr_pair_ == other.ptr_pair_;
        }

        bool operator!=(const const_iterator& other) const {
            return ptr_used_ != other.ptr_used_ || ptr_pair_ != other.ptr_pair_;
        }

    private:
        const std::pair<KeyType, ValueType>* ptr_pair_ = nullptr;
        uint8_t *ptr_used_ = nullptr, *end_used_ = nullptr;
    };

    iterator begin() {  // NOLINT
        uint8_t* begin = used_;
        while (begin != used_ + capacity_ && *(begin) != 1) {
            ++begin;
        }
        return iterator(pairs_ + (begin - used_), begin, used_ + capacity_);
    }
    iterator end() {  // NOLINT
        return iterator(pairs_ + capacity_, used_ + capacity_, used_ + capacity_);
    }

    const_iterator begin() const {  // NOLINT
        uint8_t* begin = used_;
        while (begin != used_ + capacity_ && *(begin) != 1) {
            ++begin;
        }
        return const_iterator(pairs_ + (begin - used_), begin, used_ + capacity_);
    }
    const_iterator end() const {  // NOLINT
        return const_iterator(pairs_ + capacity_, used_ + capacity_, used_ + capacity_);
    }

    const_iterator Find(const KeyType& key) const {
        size_t index = FindPosition(key);
        if (used_[index] != 1) {
            return end();
        }
        return const_iterator(pairs_ + index, used_ + index, used_ + capacity_);
    }

    iterator Find(const KeyType& key) {
        size_t index = FindPosition(key);
        if (used_[index] != 1) {
            return end();
        }
        return iterator(pairs_ + index, used_ + index, used_ + capacity_);
    }

private:
    constexpr static const size_t kInitialSize = 2;
    constexpr static const size_t kShiftHashFactors[] = {239, 179, 191};
    constexpr static const size_t kBottomLoadFactor = 25;
    constexpr static const size_t kTopLoadFactor = 50;
    constexpr static const size_t kMaxPercent = 100;
    Hash hash_;
    std::pair<KeyType, ValueType>* pairs_ = nullptr;
    size_t size_;
    size_t capacity_;
    uint8_t* used_ = nullptr;
    void InitMemory(size_t new_capacity) {
        try {
            pairs_ = new std::pair<KeyType, ValueType>[new_capacity];
            size_ = 0;
            capacity_ = new_capacity;
            used_ = new uint8_t[new_capacity];
        } catch (...) {
            delete[] pairs_;
            throw;
        }
        for (size_t i = 0; i < capacity_; i++) {
            used_[i] = 0;
        }
    }

    void ClearMemory() {
        delete[] used_;
        delete[] pairs_;
        used_ = nullptr;
        pairs_ = nullptr;
    }

    size_t ComputeShiftHash(size_t primary_hash) const {
        size_t res = 0;
        for (size_t rate : kShiftHashFactors) {
            res = (res * primary_hash + rate) % capacity_;
        }
        return res;
    }

    size_t FindPosition(const KeyType& key) const {
        size_t hash = hash_(key), shift_hash = ComputeShiftHash(hash);
        size_t index = hash % capacity_;
        size_t first_zero = capacity_;
        while (used_[index] == 2 || (used_[index] == 1 && (pairs_[index].first != key))) {
            if (first_zero == capacity_ && used_[index] == 2) {
                first_zero = index;
            }
            index = (index + shift_hash) % capacity_;
        }
        if (used_[index] == 1 || pairs_[index].first == key) {
            return index;
        }
        if (first_zero != capacity_) {
            return first_zero;
        }
        return index;
    }

    void CreatePair(size_t index, const KeyType& key, const ValueType& value = ValueType()) {
        pairs_[index].first = key;
        pairs_[index].second = value;
        size_++;
        used_[index] = 1;
    }

    void DeletePair(size_t index) {
        pairs_[index] = {KeyType{}, ValueType{}};
        size_--;
        used_[index] = 2;
    }

    bool CheckOverload() {
        if (kMaxPercent * (size_ + 1) > kTopLoadFactor * capacity_) {
            Rebuild(capacity_ * 2);
            return true;
        }
        return false;
    }

    void CheckInsufficientLoad() {
        if (size_ >= 2 * kInitialSize && kMaxPercent * size_ < kBottomLoadFactor * capacity_) {
            Rebuild(capacity_ / 2);
        }
    }

    void Rebuild(size_t new_capacity) {
        size_t new_size = 0;
        uint8_t* new_used = new uint8_t[new_capacity];
        for (size_t i = 0; i < new_capacity; i++) {
            new_used[i] = 0;
        }
        std::pair<KeyType, ValueType>* new_pairs;
        try {
            new_pairs = new std::pair<KeyType, ValueType>[new_capacity];
        } catch (...) {
            delete[] new_used;
            throw;
        }

        std::swap(new_capacity, capacity_);
        std::swap(new_size, size_);
        std::swap(new_used, used_);
        std::swap(new_pairs, pairs_);
        try {
            for (size_t i = 0; i < new_capacity; i++) {
                if (new_used[i] == 1) {
                    size_t index = FindPosition(new_pairs[i].first);
                    CreatePair(index, new_pairs[i].first, new_pairs[i].second);
                }
            }
        } catch (...) {
            std::swap(new_capacity, capacity_);
            std::swap(new_size, size_);
            std::swap(new_used, used_);
            std::swap(new_pairs, pairs_);
            delete[] new_used;
            delete[] new_pairs;
            throw;
        }
        delete[] new_used;
        delete[] new_pairs;
    }
};
