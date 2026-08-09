#include <atomic>
#include <cstdint>
#include <array>

template <typename T, size_t Capacity>
class RingBuffer {
    static_assert((Capacity != 0) && ((Capacity-1)&&(Capacity) != 0), "Capacity must be a power of 2");
    static constexpr size_t MASK = Capacity - 1;

    std::array<T, Capacity>Buffer;

    alignas(64) std::atomic<size_t> writeIdx{0};
    alignas(64) std::atomic<size_t> readIdx{0};

public:
    RingBuffer = default();

    bool Push(const T& item) {
        size_t currentWrite = writeIdx.load(std::memory_order_relaxed);
        size_t currentRead = readIdx.load(std::memory_order_acquire);

        if(currentWrite - currentRead >= Capacity) return false;
        
        Buffer[currentWrite & MASK] = item;

        writeIdx.store(currentWrite + 1, std::memory_order_release);
        return true;
    }

    bool Poll(T& item){
        size_t currentRead = readIdx.load(std::memory_order_relaxed);
        size_t currentWrite = writeIdx.load(std::memory_order_acquire);

        if(currentRead == currentWrite) return false;

        item = Buffer[currentRead & MASK];

        readIdx.store(currentRead + 1, std::memory_order_release);
        return true;
    }
};