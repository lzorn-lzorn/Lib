#include "Array.h"
#include <vector>
#include <iostream>
#include <chrono>
#include <string>
#include <cassert>

using namespace std::chrono;

struct Trackable {
    int value{0};
    inline static std::size_t constructions = 0;
    inline static std::size_t destructions = 0;
    inline static std::size_t moves = 0;
    inline static std::size_t copies = 0;

    Trackable() noexcept { ++constructions; }
    explicit Trackable(int v) noexcept : value(v) { ++constructions; }
    Trackable(const Trackable& other) noexcept : value(other.value) { ++copies; ++constructions; }
    Trackable(Trackable&& other) noexcept : value(other.value) { ++moves; ++constructions; }
    Trackable& operator=(const Trackable& other) noexcept { value = other.value; ++copies; return *this; }
    Trackable& operator=(Trackable&& other) noexcept { value = other.value; ++moves; return *this; }
    ~Trackable() noexcept { ++destructions; }
};

static void ResetTrackableCounters() {
    Trackable::constructions = 0;
    Trackable::destructions = 0;
    Trackable::moves = 0;
    Trackable::copies = 0;
}

void BasicLogicTest() {
    std::cout << "=== Basic Logic Test ===\n";
    ResetTrackableCounters();
    {
        Potato::Array<Trackable> arr;
        // Append temporaries and lvalues
        arr.Append(Trackable(1));
        Trackable t(2);
        arr.Append(t);
        arr.Append(Trackable(3));
        std::cout << "constructions=" << Trackable::constructions
                  << " copies=" << Trackable::copies
                  << " moves=" << Trackable::moves
                  << " destructions=" << Trackable::destructions << "\n";
    }
    // after scope destructed, destructions should match constructions
    std::cout << "after scope destructions=" << Trackable::destructions << " constructions=" << Trackable::constructions << "\n";
}

void MemoryPressureTest(std::size_t count) {
    std::cout << "=== Memory Pressure Test (count=" << count << ") ===\n";
    Potato::Array<int> arr;
    auto t0 = steady_clock::now();
    for (std::size_t i = 0; i < count; ++i) {
        arr.Append(static_cast<int>(i));
    }
    auto t1 = steady_clock::now();
    std::cout << "Appended " << arr.Size() << " ints in " << duration_cast<milliseconds>(t1 - t0).count() << " ms\n";
    std::cout << "Size reported by array: " << arr.Size() << "\n";
    if (!arr.IsEmpty()) {
        auto it = arr.begin();
        std::cout << "first element (via iterator): " << *it << "\n";
    }
    // simple validation using iterator (operator[] not implemented in Array)
    bool ok = true;
    std::size_t idx = 0;
    for (auto it = arr.begin(); it != arr.end() && idx < 10; ++it, ++idx) {
        if (*it != static_cast<int>(idx)) { ok = false; break; }
    }
    std::cout << "validation: " << (ok ? "PASS" : "FAIL") << "\n";
}

void PerformanceTest(std::size_t count) {
    std::cout << "=== Performance Test (count=" << count << ") ===\n";
    Potato::Array<int> arr;
    auto t0 = high_resolution_clock::now();
    for (std::size_t i = 0; i < count; ++i) arr.Append(static_cast<int>(i));
    auto t1 = high_resolution_clock::now();
    auto append_ms = duration_cast<milliseconds>(t1 - t0).count();

    // iterate
    volatile std::uint64_t sum = 0;
    auto t2 = high_resolution_clock::now();
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        sum += *it;
    }
    auto t3 = high_resolution_clock::now();
    auto iter_ms = duration_cast<milliseconds>(t3 - t2).count();

    std::cout << "append time: " << append_ms << " ms, iterate time: " << iter_ms << " ms, checksum=" << sum << "\n";
}

int main() {
    try {
        BasicLogicTest();
        // Choose counts conservative for CI; you can increase locally
        MemoryPressureTest(10000); // 10k (reduced for debugging)
        PerformanceTest(50000); // 50k (reduced for debugging)
    } catch (const std::exception& ex) {
        std::cerr << "Unhandled exception: " << ex.what() << '\n';
        return 2;
    } catch (...) {
        std::cerr << "Unhandled unknown exception\n";
        return 3;
    }
    std::cout << "All tests completed.\n";
    return 0;
}