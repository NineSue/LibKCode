/*
 * final_bench.cpp - LibKCode vs C++ STL 性能对标测试
 *
 * 编译: g++ -O3 final_bench.cpp -I../../lib/include -L../../lib -lkcode -lpthread -Wl,-rpath,../../lib -o final_bench
 * 运行: ./final_bench [节点数量，默认100万]
 */
#include <iostream>
#include <map>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <random>
#include <cstdlib>

extern "C" {
#include <kcode.h>
}

using namespace std;
using namespace std::chrono;

// 计时辅助
template<typename Func>
double measure_ms(Func f) {
    auto start = high_resolution_clock::now();
    f();
    auto end = high_resolution_clock::now();
    return duration_cast<microseconds>(end - start).count() / 1000.0;
}

void run_benchmark(size_t N) {
    cout << "\n========================================" << endl;
    cout << "  Benchmark: " << N << " nodes" << endl;
    cout << "========================================\n" << endl;

    // 准备随机数据
    vector<uint64_t> keys(N);
    iota(keys.begin(), keys.end(), 0);

    random_device rd;
    mt19937 gen(rd());
    shuffle(keys.begin(), keys.end(), gen);

    double stl_insert = 0, stl_find = 0, stl_erase = 0;
    double kcode_insert = 0, kcode_find = 0, kcode_erase = 0;

    // === C++ STL std::map ===
    cout << "[C++ std::map] (O3 optimized)" << endl;
    {
        map<uint64_t, uint64_t> tree;

        stl_insert = measure_ms([&]() {
            for (auto k : keys) tree[k] = k;
        });
        cout << "  Insert: " << stl_insert << " ms ("
             << (size_t)(N / stl_insert * 1000) << " ops/sec)" << endl;

        shuffle(keys.begin(), keys.end(), gen);

        stl_find = measure_ms([&]() {
            volatile uint64_t sum = 0;
            for (auto k : keys) sum += tree[k];
        });
        cout << "  Find:   " << stl_find << " ms ("
             << (size_t)(N / stl_find * 1000) << " ops/sec)" << endl;

        shuffle(keys.begin(), keys.end(), gen);

        stl_erase = measure_ms([&]() {
            for (auto k : keys) tree.erase(k);
        });
        cout << "  Erase:  " << stl_erase << " ms ("
             << (size_t)(N / stl_erase * 1000) << " ops/sec)" << endl;
    }

    cout << endl;

    // 重新打乱
    shuffle(keys.begin(), keys.end(), gen);

    // === LibKCode ===
    cout << "[LibKCode] (Kernel binary transplant)" << endl;
    {
        kcode_rbtree_t* tree = kcode_rbtree_new();

        kcode_insert = measure_ms([&]() {
            for (auto k : keys) kcode_rbtree_insert(tree, k, k);
        });
        cout << "  Insert: " << kcode_insert << " ms ("
             << (size_t)(N / kcode_insert * 1000) << " ops/sec)" << endl;

        shuffle(keys.begin(), keys.end(), gen);

        kcode_find = measure_ms([&]() {
            volatile uint64_t sum = 0;
            for (auto k : keys) sum += kcode_rbtree_find(tree, k);
        });
        cout << "  Find:   " << kcode_find << " ms ("
             << (size_t)(N / kcode_find * 1000) << " ops/sec)" << endl;

        shuffle(keys.begin(), keys.end(), gen);

        kcode_erase = measure_ms([&]() {
            for (auto k : keys) kcode_rbtree_remove(tree, k);
        });
        cout << "  Erase:  " << kcode_erase << " ms ("
             << (size_t)(N / kcode_erase * 1000) << " ops/sec)" << endl;

        kcode_rbtree_free(tree);
    }

    // === 对比结果 ===
    cout << "\n--- Comparison ---" << endl;
    cout << "  Insert: LibKCode is " << (stl_insert / kcode_insert) << "x vs STL" << endl;
    cout << "  Find:   LibKCode is " << (stl_find / kcode_find) << "x vs STL" << endl;
    cout << "  Erase:  LibKCode is " << (stl_erase / kcode_erase) << "x vs STL" << endl;
}

int main(int argc, char** argv) {
    size_t n = (argc > 1) ? atol(argv[1]) : 1000000;

    if (kcode_init() != 0) {
        cerr << "kcode_init failed" << endl;
        return 1;
    }

    run_benchmark(n);

    kcode_cleanup();
    return 0;
}
