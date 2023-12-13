#pragma once

#include <initializer_list>
#include <algorithm>
#include <cassert>
#include <memory>
#include <deque>

class Deque {
public:
    const static size_t BlockSizeBytes;
    const static size_t BlockSize;
    const static size_t MinCapacity;

    class Block {
    public:
        explicit Block(size_t size)
                : begin_(0), size_(0), max_size_(size), ptr_(std::make_unique<int[]>(size)) {
        }

        bool IsBackEmpty() const {
            return begin_ + size_ < max_size_;
        }

        bool IsFrontEmpty() const {
            return begin_ > 0 || size_ == 0;
        }

        size_t Size() const {
            return size_;
        }

        void PushBack(int elem) {
            if (size_ == 0) {
                begin_ = 0;
            }
            ptr_[begin_ + size_] = elem;
            ++size_;
        }

        void PushFront(int elem) {
            if (size_ == 0) {
                begin_ = max_size_;
            }
            ptr_[begin_ - 1] = elem;
            ++size_;
            --begin_;
        }

        void PopBack() {
            --size_;
        }

        void PopFront() {
            ++begin_;
            --size_;
        }

        int& Get(size_t ind) {
            return ptr_[begin_ + ind];
        }

        const int& Get(size_t ind) const {
            return ptr_[begin_ + ind];
        }

    private:
        size_t begin_;
        size_t size_;
        size_t max_size_;
        std::unique_ptr<int[]> ptr_;
    };

    Deque() : size_(0), begin_(0), capacity_(0), count_elem_(0) {
        SetCapacity(MinCapacity);
    }
    Deque(const Deque& rhs) : Deque(rhs.Size()) {
        for (size_t i = 0; i < Size(); ++i) {
            this->operator[](i) = rhs[i];
        }
    }
    Deque(Deque&& rhs) = default;
    explicit Deque(size_t size) : size_(0), begin_(0), capacity_(0), count_elem_(0) {
        SetCapacity(std::max(MinCapacity, size / BlockSize + 1));
        for (size_t i = 0; i < size; ++i) {
            PushBack(0);
        }
    }

    Deque(std::initializer_list<int> list) : Deque(list.size()) {
        size_t i = 0;
        auto it = list.begin();
        while (it != list.end()) {
            this->operator[](i) = *it;
            ++i;
            ++it;
        }
    }

    Deque& operator=(Deque rhs) {
        Swap(rhs);
        return *this;
    }

    void Swap(Deque& rhs) {
        std::swap(size_, rhs.size_);
        std::swap(begin_, rhs.begin_);
        std::swap(capacity_, rhs.capacity_);
        std::swap(count_elem_, rhs.count_elem_);
        std::swap(blocks_, rhs.blocks_);
    }

    void PushBack(int value) {
        size_t ind;
        if (size_ == 0) {
            ind = PushBlockBack();
        } else {
            size_t lst = Next(begin_, size_ - 1);
            if (blocks_[lst]->IsBackEmpty()) {
                ind = lst;
            } else {
                ind = PushBlockBack();
            }
        }
        blocks_[ind]->PushBack(value);
        ++count_elem_;
    }

    void PopBack() {
        size_t lst = Next(begin_, size_ - 1);
        blocks_[lst]->PopBack();
        if (blocks_[lst]->Size() == 0) {
            blocks_[lst].reset(nullptr);
            --size_;
        }
        --count_elem_;
    }

    void PushFront(int value) {
        size_t ind;
        if (size_ == 0) {
            ind = PushBlockBack();
        } else {
            size_t fst = begin_;
            if (blocks_[fst]->IsFrontEmpty()) {
                ind = fst;
            } else {
                ind = PushBlockFront();
            }
        }
        blocks_[ind]->PushFront(value);
        ++count_elem_;
    }

    void PopFront() {
        size_t fst = begin_;
        blocks_[fst]->PopFront();
        if (blocks_[fst]->Size() == 0) {
            blocks_[fst].reset(nullptr);
            begin_ = Next(begin_);
            --size_;
        }
        --count_elem_;
    }

    int& operator[](size_t ind) {
        auto [block, pos] = FindLocalIndex(ind);
        return blocks_[block]->Get(pos);
    }

    int operator[](size_t ind) const {
        auto [block, pos] = FindLocalIndex(ind);
        return blocks_[block]->Get(pos);
    }

    size_t Size() const {
        return count_elem_;
    }

    void Clear() {
        while (Size() > 0) {
            PopBack();
        }
    }

private:
    size_t size_;
    size_t begin_;
    size_t capacity_;
    size_t count_elem_;
    std::unique_ptr<std::unique_ptr<Block>[]> blocks_;

    size_t Next(size_t ind, size_t offset = 1) const {
        if (ind + offset < capacity_) {
            return ind + offset;
        }
        return ind + offset - capacity_;
    }

    size_t Prev(size_t ind, size_t offset = 1) const {
        if (ind < offset) {
            return capacity_ + ind - offset;
        }
        return ind - offset;
    }

    void MakeBlock(size_t ind) {
        if (blocks_[ind].get() == nullptr) {
            blocks_[ind].reset(new Block(BlockSize));
        }
    }

    bool AnyBlockPlace() const {
        return size_ < capacity_;
    }

    size_t PushBlockBack() {
        if (!AnyBlockPlace()) {
            DoubleCapacity();
        }
        MakeBlock(Next(begin_, size_));
        ++size_;
        return Next(begin_, size_ - 1);
    }

    size_t PushBlockFront() {
        if (!AnyBlockPlace()) {
            DoubleCapacity();
        }
        begin_ = Prev(begin_);
        ++size_;
        MakeBlock(begin_);
        return begin_;
    }

    std::pair<size_t, size_t> FindLocalIndex(size_t ind) const {
        size_t block = begin_;
        if (ind < blocks_[block]->Size()) {
            return {block, ind};
        }
        ind -= blocks_[block]->Size();
        block = Next(block);
        size_t full_blocks = ind / BlockSize;
        block = Next(block, full_blocks);
        ind -= full_blocks * BlockSize;

        return {block, ind};
    }

    void DoubleCapacity() {
        SetCapacity(capacity_ * 2);
    }

    void SetCapacity(size_t new_capacity) {
        assert(new_capacity >= capacity_);
        std::unique_ptr<std::unique_ptr<Block>[]> new_blocks =
                std::make_unique<std::unique_ptr<Block>[]>(new_capacity);
        size_t current = 0;
        size_t i = begin_;
        while (current < capacity_) {
            new_blocks[current].reset(blocks_[i].release());
            ++current;
            i = Next(i);
        }
        begin_ = 0;
        capacity_ = new_capacity;
        blocks_.reset(new_blocks.release());
    }
};

const size_t Deque::BlockSizeBytes = 512;
const size_t Deque::BlockSize = BlockSizeBytes / sizeof(int);
const size_t Deque::MinCapacity = 1;
