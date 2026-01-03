/*
 * Copyright (C) 2026 E Zuan, Liu Jiayou, Xia Yefei (Team Xinjian Wenjianjia)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * 项目名称: 内核代码映射用户态
 * 参赛队伍: 新建文件夹 (Xinjian Wenjianjia)
 * 队员: 鄂祖安 (E Zuan), 刘家佑 (Liu Jiayou), 夏业飞 (Xia Yefei)
 */

/*
 * benchmark.cpp - LibKCode vs C++ STL 性能对标测试
 *
 * 编译: g++ -O3 benchmark.cpp -I../../lib/include -L../../lib -lkcode -lpthread -Wl,-rpath,../../lib -o benchmark
 * 运行: ./benchmark [节点数量，默认100万]
 */
#include <iostream>
#include <map>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <random>
#include <cstdlib>
#include <cstring>

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

// kcode_sort 的比较函数
static int cmp_int(const void* a, const void* b) {
    return *(int*)a - *(int*)b;
}

void run_rbtree_benchmark(size_t N, mt19937& gen) {
    cout << "\n========================================" << endl;
    cout << "  RB-Tree Benchmark: " << N << " nodes" << endl;
    cout << "========================================\n" << endl;

    vector<uint64_t> keys(N);
    iota(keys.begin(), keys.end(), 0);
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
    shuffle(keys.begin(), keys.end(), gen);

    // === LibKCode ===
    cout << "[LibKCode RB-Tree] (Kernel binary)" << endl;
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
    cout << "\n--- RB-Tree Comparison ---" << endl;
    printf("  Insert: LibKCode is %.2fx vs STL\n", stl_insert / kcode_insert);
    printf("  Find:   LibKCode is %.2fx vs STL\n", stl_find / kcode_find);
    printf("  Erase:  LibKCode is %.2fx vs STL\n", stl_erase / kcode_erase);
}

void run_sort_benchmark(size_t N, mt19937& gen) {
    cout << "\n========================================" << endl;
    cout << "  Sort Benchmark: " << N << " elements" << endl;
    cout << "========================================\n" << endl;

    vector<int> data(N);
    for (size_t i = 0; i < N; i++) data[i] = gen();

    // === C++ std::sort ===
    cout << "[C++ std::sort] (Introsort, O3)" << endl;
    {
        vector<int> d = data;
        double t = measure_ms([&]() {
            sort(d.begin(), d.end());
        });
        cout << "  Time: " << t << " ms" << endl;
    }

    // === C qsort ===
    cout << "[C qsort]" << endl;
    {
        vector<int> d = data;
        double t = measure_ms([&]() {
            qsort(d.data(), N, sizeof(int), [](const void* a, const void* b) {
                return *(int*)a - *(int*)b;
            });
        });
        cout << "  Time: " << t << " ms" << endl;
    }

    // === LibKCode (Kernel heapsort) ===
    cout << "[LibKCode sort] (Kernel heapsort)" << endl;
    {
        vector<int> d = data;
        double t = measure_ms([&]() {
            kcode_sort(d.data(), N, sizeof(int), cmp_int, nullptr);
        });
        cout << "  Time: " << t << " ms" << endl;
    }

    cout << "\n--- Sort Characteristics ---" << endl;
    cout << "  std::sort:   Fast (Introsort), O(log n) stack space" << endl;
    cout << "  qsort:       Portable, function pointer overhead" << endl;
    cout << "  kcode_sort:  O(1) space, O(n log n) guaranteed, no recursion" << endl;
}

int main(int argc, char** argv) {
    size_t n = (argc > 1) ? atol(argv[1]) : 1000000;

    if (kcode_init() != 0) {
        cerr << "kcode_init failed" << endl;
        return 1;
    }

    random_device rd;
    mt19937 gen(rd());

    run_rbtree_benchmark(n, gen);
    run_sort_benchmark(n, gen);

    cout << "\n========================================" << endl;
    cout << "  Benchmark Complete" << endl;
    cout << "========================================" << endl;

    kcode_cleanup();
    return 0;
}
