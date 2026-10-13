#include <cstddef>
#include <new>
#include <cstring>
#include "sead/heap/seadHeap.h"

namespace btt {
    extern sead::Heap* sHeap;
}

// 512 KB static fallback arena for very early allocations before sHeap is ready
alignas(16) static unsigned char sStaticArena[512 * 1024];
static size_t sStaticOffset = 0;

static void* static_alloc(size_t size, size_t alignment = 8) {
    size_t cur = reinterpret_cast<size_t>(&sStaticArena[sStaticOffset]);
    size_t aligned = (cur + alignment - 1) & ~(alignment - 1);
    size_t offset = aligned - reinterpret_cast<size_t>(sStaticArena);
    if (offset + size <= sizeof(sStaticArena)) {
        sStaticOffset = offset + size;
        return &sStaticArena[offset];
    }
    return nullptr;
}

static inline bool is_static_ptr(const void* ptr) {
    return ptr >= sStaticArena && ptr < (sStaticArena + sizeof(sStaticArena));
}

void* operator new(std::size_t size) {
    if (btt::sHeap) {
        void* p = btt::sHeap->tryAlloc(size, 8);
        if (p) return p;
    }
    return static_alloc(size, 8);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    return operator new(size);
}

void* operator new[](std::size_t size) {
    return operator new(size);
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    return operator new(size);
}

void operator delete(void* ptr) noexcept {
    if (!ptr || is_static_ptr(ptr)) return;
    if (btt::sHeap && btt::sHeap->isInclude(ptr)) {
        btt::sHeap->free(ptr);
    }
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept {
    operator delete(ptr);
}

void operator delete(void* ptr, std::size_t size) noexcept {
    operator delete(ptr);
}

void operator delete[](void* ptr) noexcept {
    operator delete(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept {
    operator delete(ptr);
}

void operator delete[](void* ptr, std::size_t size) noexcept {
    operator delete(ptr);
}

extern "C" {
void* malloc(std::size_t size) {
    return operator new(size);
}

void free(void* ptr) {
    operator delete(ptr);
}

void* calloc(std::size_t num, std::size_t size) {
    size_t total = num * size;
    void* p = operator new(total);
    if (p) memset(p, 0, total);
    return p;
}

void* realloc(void* ptr, std::size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) {
        free(ptr);
        return nullptr;
    }
    if (is_static_ptr(ptr)) {
        void* newp = malloc(size);
        if (newp) {
            memcpy(newp, ptr, size);
        }
        return newp;
    }
    if (btt::sHeap && btt::sHeap->isInclude(ptr)) {
        void* re = btt::sHeap->tryRealloc(ptr, size, 8);
        if (re) return re;
        void* newp = btt::sHeap->tryAlloc(size, 8);
        if (newp) {
            memcpy(newp, ptr, size);
            btt::sHeap->free(ptr);
            return newp;
        }
    }
    return nullptr;
}

void* aligned_alloc(std::size_t alignment, std::size_t size) {
    if (btt::sHeap) {
        void* p = btt::sHeap->tryAlloc(size, (s32)alignment);
        if (p) return p;
    }
    return static_alloc(size, alignment);
}
}
