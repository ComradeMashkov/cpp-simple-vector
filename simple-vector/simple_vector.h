#pragma once

#include <cassert>
#include <initializer_list>
#include "array_ptr.h"

class ReserveProxyObj {
public:
    explicit ReserveProxyObj (size_t capacity_to_reserve)
        : capacity_(capacity_to_reserve)
    {}
    
    size_t GetCapacity() {
        return capacity_;
    }
    
private:
    size_t capacity_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)
        : vector_(size)
        , size_(size)
        , capacity_(size)
        {}

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value)
        : vector_(size)
        , size_(size)
        , capacity_(size)
    {
        std::fill(begin(), end(), value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init)
        : vector_(init.size())
        , size_(init.size())
        , capacity_(init.size())
    {
        std::copy(init.begin(), init.end(), begin());
    }
    
    SimpleVector(const SimpleVector& other)
        : vector_(other.size_)
        , size_(other.size_)
        , capacity_(other.capacity_)
    {
        ArrayPtr<Type> tmp(other.size_);
        std::copy(other.begin(), other.end(), &tmp[0]);
        vector_.swap(tmp);
    }
    
    SimpleVector(SimpleVector&& other)
        : vector_(other.size_)
        , size_(std::move(other.size_))
        , capacity_(std::move(other.capacity_))
    {
        vector_.swap(other.vector_);
        other.Clear();
    }
    
    SimpleVector(ReserveProxyObj capacity_to_reserve) {
        Reserve(capacity_to_reserve.GetCapacity());
    }
    
    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            SimpleVector tmp(rhs);
            this->swap(tmp);
        }
        return *this;
    }
    
    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> tmp(new_capacity);
            for (size_t i = 0; i < size_; ++i) {
                tmp[i] = vector_[i];
            }
            vector_.swap(tmp);
            capacity_ = new_capacity;
        }
        
        else {
            return;
        }
    }
    
    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (size_ < capacity_) {
            vector_[size_] = item;
        }
        
        else {
            SimpleVector<Type> tmp(capacity_ == 0u ? 1u : capacity_ * 2);
            std::copy(begin(), end(), tmp.begin());
            tmp[size_] = item;
            vector_.swap(tmp.vector_);
            capacity_ = tmp.capacity_;;
        }
        ++size_;
    }
    
    void PushBack(Type&& item) {
        if (size_ < capacity_) {
            vector_[size_] = std::move(item);
        }
        
        else {
            SimpleVector<Type> tmp(capacity_ == 0u ? 1u : capacity_ * 2);
            std::move(begin(), end(), tmp.begin());
            tmp[size_] = std::move(item);
            vector_.swap(tmp.vector_);
            capacity_ = tmp.capacity_;;
        }
        ++size_;
    }
    
    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        auto index = std::distance(cbegin(), pos);

        // Проверка на попадание индекса в адресное пространство вектора
        assert(index >= 0 && index <= size_);

        if (size_ == capacity_) {
            if (size_ != 0) {
                ArrayPtr<Type> tmp(size_);
                std::copy(begin(), end(), &tmp[0]);
                vector_.swap(tmp);
                capacity_ = size_ * 2;
            }
            else {
                ArrayPtr<Type> tmp(1u);
                std::copy(begin(), end(), &tmp[0]);
                vector_.swap(tmp);
                capacity_ = 1u;
            }
        }
        
        for (size_t i = size_; i > (size_t)index; --i) {
            vector_[i] = vector_[i - 1];
        }
        
        ++size_;
        vector_[index] = value;
        return const_cast<Iterator>(index + begin());
    }
    
    Iterator Insert(ConstIterator pos, Type&& value) {
        auto index = std::distance(cbegin(), pos);

        assert(index >= 0 && index <= size_);

        if (size_ == capacity_) {
            if (size_ != 0) {
                ArrayPtr<Type> tmp(size_);
                std::move(begin(), end(), &tmp[0]);
                vector_.swap(tmp);
                capacity_ = size_ * 2;
            }
            else {
                ArrayPtr<Type> tmp(1u);
                std::move(begin(), end(), &tmp[0]);
                vector_.swap(tmp);
                capacity_ = 1u;
            }
        }
        
        for (size_t i = size_; i > (size_t)index; --i) {
            vector_[i] = std::move(vector_[i - 1]);
        }
        
        ++size_;
        vector_[index] = std::move(value);
        return const_cast<Iterator>(index + begin());
    }
    
    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(!IsEmpty());
        --size_;
    }
    
    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(!IsEmpty());
        auto index = std::distance(cbegin(), pos);
        std::move(&vector_[index + 1], end(), const_cast<Iterator>(pos));
        --size_;
        return const_cast<Iterator>(pos);
    }
    
    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        vector_.swap(other.vector_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return (size_ == 0 ? true : false);
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return vector_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return vector_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index value is out of range of vector size");
        }
        return vector_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Index value is out of range of vector size");
        }
        return vector_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (size_ >= new_size) {
            size_ = new_size;
        }
        
        else if (size_ < new_size && capacity_ > new_size) {
            for (auto it = begin() + new_size; it != end(); --it) {
                *it = std::move(Type());
            }
            size_ = new_size;
        }
        
        else {
            ArrayPtr<Type> tmp(new_size);
            std::move(begin(), end(), &tmp[0]);
            vector_.swap(tmp);
            size_ = new_size;
            capacity_ = new_size * 2;
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return Iterator(&vector_[0]);
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return Iterator{&vector_[size_]};
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return ConstIterator{&vector_[0]};
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return ConstIterator{&vector_[size_]};
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return ConstIterator{&vector_[0]};
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return ConstIterator{&vector_[size_]};
    }
    
private:
    ArrayPtr<Type> vector_;
    size_t size_ = 0u;
    size_t capacity_ = 0u;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
} 