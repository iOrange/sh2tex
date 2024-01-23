#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <filesystem>
#include <memory>
#include <numeric>
#include <algorithm>
#include <functional>
#include <cassert>
#include <cuchar>
#include <random>
#include <cctype>   // std::tolower
#include <cwctype>  // std::towlower


#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#define rcast reinterpret_cast
#define scast static_cast

#define STRINGIFY_UTIL_(s) #s
#define STRINGIFY(s) STRINGIFY_UTIL_(s)

namespace fs = std::filesystem;

template <typename T>
using MyArray = std::vector<T>;
template <typename K, typename T>
using MyDict = std::unordered_map<K, T>;
template <typename T>
using MyDeque = std::deque<T>;
using CharString = std::string;
using StringView = std::string_view;
using WideString = std::wstring;
using WStringView = std::wstring_view;
using StringArray = MyArray<CharString>;
using WStringArray = MyArray<WideString>;
using BytesArray = MyArray<uint8_t>;

template <typename T>
using StrongPtr = std::unique_ptr<T>;

template <typename T, typename... Args>
constexpr StrongPtr<T> MakeStrongPtr(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T>
using RefPtr = std::shared_ptr<T>;

template <typename T, typename... Args>
constexpr RefPtr<T> MakeRefPtr(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}
template <typename T, typename U>
constexpr RefPtr<T> SCastRefPtr(const RefPtr<U>& p) noexcept {
    return std::static_pointer_cast<T>(p);
}

inline bool StrEqualsCaseInsensitive(const CharString& str1, const CharString& str2) {
    return str1.size() == str2.size() &&
        std::equal(str1.begin(), str1.end(), str2.begin(), [](const char ch1, const char ch2) -> bool {
            return std::tolower(scast<int>(ch1)) == std::tolower(scast<int>(ch2));
        });
}

inline bool WStrEqualsCaseInsensitive(const WideString& str1, const WideString& str2) {
    return str1.size() == str2.size() &&
        std::equal(str1.begin(), str1.end(), str2.begin(), [](const wchar_t ch1, const wchar_t ch2) -> bool {
            return std::towlower(scast<wint_t>(ch1)) == std::towlower(scast<wint_t>(ch2));
        });
}

class MemStream {
    using OwnedPtrType = std::shared_ptr<uint8_t>;

public:
    MemStream()
        : data(nullptr)
        , length(0)
        , cursor(0) {
    }

    MemStream(const void* _data, const size_t _size, const bool _ownMem = false)
        : data(rcast<const uint8_t*>(_data))
        , length(_size)
        , cursor(0)
    {
        //#NOTE_SK: dirty hack to own a pointer
        if (_ownMem) {
            ownedPtr = OwnedPtrType(const_cast<uint8_t*>(data), free);
        }
    }
    MemStream(const MemStream& other)
        : data(other.data)
        , length(other.length)
        , cursor(other.cursor)
        , ownedPtr(other.ownedPtr)
        , name(other.name) {
    }
    MemStream(MemStream&& other) noexcept
        : data(other.data)
        , length(other.length)
        , cursor(other.cursor)
        , ownedPtr(other.ownedPtr)
        , name(other.name) {
    }
    ~MemStream() {
    }

    inline MemStream& operator =(const MemStream& other) {
        this->data = other.data;
        this->length = other.length;
        this->cursor = other.cursor;
        this->ownedPtr = other.ownedPtr;
        this->name = other.name;
        return *this;
    }

    inline MemStream& operator =(MemStream&& other) noexcept {
        this->data = other.data;
        this->length = other.length;
        this->cursor = other.cursor;
        this->ownedPtr.swap(other.ownedPtr);
        this->name.swap(other.name);
        return *this;
    }

    inline void SetWindow(const size_t wndOffset, const size_t wndLength) {
        this->cursor = wndOffset;
        this->length = wndOffset + wndLength;
    }

    inline operator bool() const {
        return this->Good();
    }

    inline bool Good() const {
        return (this->data != nullptr && this->length > 0 && !this->Ended());
    }

    inline bool Ended() const {
        return this->cursor >= this->length;
    }

    inline size_t Length() const {
        return this->length;
    }

    inline size_t Remains() const {
        return this->length - this->cursor;
    }

    inline const uint8_t* Data() const {
        return this->data;
    }

    inline const CharString& Name() const {
        return this->name;
    }

    inline void SetName(const CharString& n) {
        this->name = n;
    }

    bool ReadToBuffer(void* buffer, const size_t bytesToRead) {
        if (this->cursor + bytesToRead <= this->length) {
            std::memcpy(buffer, this->data + this->cursor, bytesToRead);
            this->cursor += bytesToRead;
            return true;
        } else {
            return false;
        }
    }

    void SkipBytes(const size_t bytesToSkip) {
        if (this->cursor + bytesToSkip <= this->length) {
            this->cursor += bytesToSkip;
        } else {
            this->cursor = this->length;
        }
    }

    void RewindBytes(const size_t bytesToRewind) {
        if (bytesToRewind >= this->cursor) {
            this->cursor = 0;
        } else {
            this->cursor -= bytesToRewind;
        }
    }

    template <typename T>
    T ReadTyped() {
        T result = T(0);
        if (this->cursor + sizeof(T) <= this->length) {
            result = *rcast<const T*>(this->data + this->cursor);
            this->cursor += sizeof(T);
        }
        return result;
    }

    template <typename T>
    void ReadStruct(T& s) {
        this->ReadToBuffer(&s, sizeof(T));
    }

#define _IMPL_READ_FOR_TYPE(type, name) \
    inline type Read##name() {          \
        return this->ReadTyped<type>(); \
    }

    _IMPL_READ_FOR_TYPE(int8_t, I8);
    _IMPL_READ_FOR_TYPE(uint8_t, U8);
    _IMPL_READ_FOR_TYPE(int16_t, I16);
    _IMPL_READ_FOR_TYPE(uint16_t, U16);
    _IMPL_READ_FOR_TYPE(int32_t, I32);
    _IMPL_READ_FOR_TYPE(uint32_t, U32);
    _IMPL_READ_FOR_TYPE(int64_t, I64);
    _IMPL_READ_FOR_TYPE(uint64_t, U64);
    _IMPL_READ_FOR_TYPE(bool, Bool);
    _IMPL_READ_FOR_TYPE(float, F32);
    _IMPL_READ_FOR_TYPE(double, F64);

#undef _IMPL_READ_FOR_TYPE


    inline size_t GetCursor() const {
        return this->cursor;
    }

    void SetCursor(const size_t pos) {
        this->cursor = std::min<size_t>(pos, this->length);
    }

    inline const uint8_t* GetDataAtCursor() const {
        return this->data + this->cursor;
    }

    MemStream Substream(const size_t subStreamLength) const {
        const size_t allowedLength = ((this->cursor + subStreamLength) > this->Length()) ? (this->Length() - this->cursor) : subStreamLength;
        return MemStream(this->GetDataAtCursor(), allowedLength);
    }

    MemStream Substream(const size_t subStreamOffset, const size_t subStreamLength) const {
        const size_t allowedOffset = (subStreamOffset > this->Length()) ? this->Length() : subStreamOffset;
        const size_t allowedLength = ((allowedOffset + subStreamLength) > this->Length()) ? (this->Length() - allowedOffset) : subStreamLength;
        return MemStream(this->data + allowedOffset, allowedLength);
    }

    MemStream Clone() const {
        if (this->ownedPtr) {
            return *this;
        } else {
            void* dataCopy = malloc(this->Length());
            assert(dataCopy != nullptr);
            memcpy(dataCopy, this->data, this->Length());
            return MemStream(dataCopy, this->Length(), true);
        }
    }

private:
    const uint8_t*  data;
    size_t          length;
    size_t          cursor;
    OwnedPtrType    ownedPtr;
    CharString      name;
};


class MemWriteStream {
public:
    MemWriteStream(const size_t startupSize = 4096) { mBuffer.reserve(startupSize); }
    ~MemWriteStream() {}

    void Swap(MemWriteStream& other) {
        mBuffer.swap(other.mBuffer);
    }

    void SwapBuffer(BytesArray& buffer) {
        mBuffer.swap(buffer);
    }

    void Write(const void* data, const size_t length) {
        const size_t cursor = this->GetWrittenBytesCount();
        mBuffer.resize(mBuffer.size() + length);
        memcpy(mBuffer.data() + cursor, data, length);
    }

    void WriteDupByte(const uint8_t value, const size_t numBytes) {
        const size_t cursor = this->GetWrittenBytesCount();
        mBuffer.resize(mBuffer.size() + numBytes);
        memset(mBuffer.data() + cursor, scast<int>(value), numBytes);
    }

    template <typename T>
    void Write(const T& v) {
        this->Write(&v, sizeof(T));
    }

#define _IMPL_WRITE_FOR_TYPE(type, name)        \
    inline void Write##name(const type& v) {    \
        this->Write<type>(v);                   \
    }

    _IMPL_WRITE_FOR_TYPE(int8_t, I8);
    _IMPL_WRITE_FOR_TYPE(uint8_t, U8);
    _IMPL_WRITE_FOR_TYPE(int16_t, I16);
    _IMPL_WRITE_FOR_TYPE(uint16_t, U16);
    _IMPL_WRITE_FOR_TYPE(int32_t, I32);
    _IMPL_WRITE_FOR_TYPE(uint32_t, U32);
    _IMPL_WRITE_FOR_TYPE(int64_t, I64);
    _IMPL_WRITE_FOR_TYPE(uint64_t, U64);
    _IMPL_WRITE_FOR_TYPE(bool, Bool);
    _IMPL_WRITE_FOR_TYPE(float, F32);
    _IMPL_WRITE_FOR_TYPE(double, F64);

#undef _IMPL_WRITE_FOR_TYPE

    inline void Append(const MemStream& stream) {
        this->Write(stream.Data(), stream.Length());
    }

    inline void Append(const MemWriteStream& stream) {
        this->Write(stream.Data(), stream.GetWrittenBytesCount());
    }

    size_t GetWrittenBytesCount() const {
        return mBuffer.size();
    }

    void* Data() {
        return mBuffer.data();
    }

    const void* Data() const {
        return mBuffer.data();
    }

    inline void SwapToBytesArray(BytesArray& dst) {
        mBuffer.swap(dst);
    }

private:
    BytesArray  mBuffer;
};
