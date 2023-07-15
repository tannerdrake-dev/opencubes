// #define DBG 1

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

#include "results.hpp"
#include "structs.hpp"
#include "rotations.hpp"
#include "cache.hpp"

bool USE_CACHE = 1;

void expand(const Cube &c, Hashy &hashes)
{
    XYZSet candidates;
    candidates.reserve(c.size() * 6);
    for (const auto &p : c)
    {
        candidates.emplace(XYZ{p.x + 1, p.y, p.z});
        candidates.emplace(XYZ{p.x - 1, p.y, p.z});
        candidates.emplace(XYZ{p.x, p.y + 1, p.z});
        candidates.emplace(XYZ{p.x, p.y - 1, p.z});
        candidates.emplace(XYZ{p.x, p.y, p.z + 1});
        candidates.emplace(XYZ{p.x, p.y, p.z - 1});
    }
    for (const auto &p : c)
    {
        candidates.erase(p);
    }
#ifdef DBG
    printf("candidates: %lu\n\r", candidates.size());
#endif
    for (const auto &p : candidates)
    {
#ifdef DBG
        printf("(%2d %2d %2d)\n\r", p.x, p.y, p.z);
#endif
        int ax = (p.x < 0) ? 1 : 0;
        int ay = (p.y < 0) ? 1 : 0;
        int az = (p.z < 0) ? 1 : 0;
        Cube newCube;
        newCube.reserve(c.size() + 1);
        newCube.emplace_back(XYZ{p.x + ax, p.y + ay, p.z + az});
        std::array<int, 3> shape{p.x + ax, p.y + ay, p.z + az};
        for (const auto &np : c)
        {
            auto nx = np.x + ax;
            auto ny = np.y + ay;
            auto nz = np.z + az;
            if (nx > shape[0])
                shape[0] = nx;
            if (ny > shape[1])
                shape[1] = ny;
            if (nz > shape[2])
                shape[2] = nz;
            newCube.emplace_back(XYZ{nx, ny, nz});
        }
        // printf("shape %2d %2d %2d\n\r", shape[0], shape[1], shape[2]);
        // newCube.print();

        // check rotations
        Cube lowestHashCube;
        XYZ lowestShape;
        bool none_set = true;
        for (int i = 0; i < 24; ++i)
        {
            auto res = Rotations::rotate(i, shape, newCube);
            if (res.second.size() == 0)
                continue; // rotation generated violating shape
            Cube rotatedCube{std::move(res.second)};
            std::sort(rotatedCube.begin(), rotatedCube.end());

            if (none_set || lowestHashCube < rotatedCube)
            {
                none_set = false;
                // printf("shape %2d %2d %2d\n\r", res.first.x, res.first.y, res.first.z);
                lowestHashCube = std::move(rotatedCube);
                lowestShape = res.first;
            }
        }
        hashes.insert(std::move(lowestHashCube), lowestShape);
#ifdef DBG
        std::printf("=====\n\r");
        // lowestHashCube.print();
        std::printf("inserted! (num %2lu)\n\n\r", hashes.size());
#endif
    }
#ifdef DBG
    std::printf("new hashes: %lu\n\r", hashes.size());
#endif
}

void expandPart(std::vector<Cube> &base, Hashy &hashes, size_t start, size_t end)
{
    printf("  start from %lu to %lu\n\r", start, end);
    auto t_start = std::chrono::steady_clock::now();

    for (auto i = start; i < end; ++i)
    {
        expand(base[i], hashes);
        auto count = i - start;
        if (start == 0 && (count % 100 == 99))
        {
            auto t_end = std::chrono::steady_clock::now();
            auto dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
            auto perc = 100 * count / (end - start);
            auto its = 1000.f * count / dt_ms;
            auto remaining = (end - i) / its;
            std::printf(" %3lu%% %5.0f baseCubes/s, remaining: %.0fs\033[0K\r", perc, its, remaining);
            std::flush(std::cout);
        }
    }
    auto t_end = std::chrono::steady_clock::now();
    auto dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
    std::printf("  done from %lu to %lu: found %lu\n\r", start, end, hashes.size());
    std::printf("  took %.2f s\033[0K\n\r", dt_ms / 1000.f);
}

Hashy gen(int n, int threads = 1)
{
    Hashy hashes;
    if (n < 1)
        return {};
    else if (n == 1)
    {
        hashes.insert(Cube{{XYZ{0, 0, 0}}}, XYZ{0, 0, 0});
        std::printf("%ld elements for %d\n\r", hashes.size(), n);
        return hashes;
    }
    else if (n == 2)
    {
        hashes.insert(Cube{{XYZ{0, 0, 0}, XYZ{0, 0, 1}}}, XYZ{0, 0, 1});
        std::printf("%ld elements for %d\n\r", hashes.size(), n);
        return hashes;
    }

    if (USE_CACHE)
    {
        hashes = load("cubes_" + std::to_string(n) + ".bin");

        if (hashes.size() != 0)
            return hashes;
    }

    auto base = gen(n - 1, threads);
    std::printf("N = %d || generating new cubes from %lu base cubes.\n\r", n, base.size());
    hashes.init(n);
    int count = 0;
    if (threads == 1 || base.size() < 100)
    {
        auto start = std::chrono::steady_clock::now();
        int total = base.size();

        for (const auto &s : base.byshape)
        {
            // printf("shapes %d %d %d\n\r", s.first.x, s.first.y, s.first.z);
            for (const auto &b : s.second.set)
            {
                expand(b, hashes);
                count++;
                if (count % 100 == 99)
                {
                    auto end = std::chrono::steady_clock::now();
                    auto dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                    auto perc = 100 * count / total;
                    auto its = 1000.f * count / dt_ms;
                    auto remaining = (total - count) / its;
                    printf(" %3d%% %5.0f baseCubes/s, remaining: %.0fs\033[0K\r", perc, its, remaining);
                    std::flush(std::cout);
                }
            }
        }
        auto end = std::chrono::steady_clock::now();
        auto dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::printf("  took %.2f s\033[0K\n\r", dt_ms / 1000.f);
    }
    else
    {
        std::vector<Cube> baseCubes;
        std::printf("converting to vector\n\r");
        for (auto &s : base.byshape)
        {
            baseCubes.insert(baseCubes.end(), s.second.set.begin(), s.second.set.end());
            s.second.set.clear();
            s.second.set.reserve(1);
        }
        std::printf("starting %d threads\n\r", threads);
        std::vector<std::thread> ts;
        ts.reserve(threads);
        for (int i = 0; i < threads; ++i)
        {
            auto start = baseCubes.size() * i / threads;
            auto end = baseCubes.size() * (i + 1) / threads;

            ts.emplace_back(expandPart, std::ref(baseCubes), std::ref(hashes), start, end);
        }
        for (int i = 0; i < threads; ++i)
        {
            ts[i].join();
        }
    }
    std::printf("  num cubes: %lu\n\r", hashes.size());
    save("cubes_" + std::to_string(n) + ".bin", hashes, n);
    if (sizeof(results) / sizeof(results[0]) > (n - 1) && n > 1)
    {
        if (results[n - 1] != hashes.size())
        {
            std::printf("ERROR: result does not equal resultstable (%lu)!\n\r", results[n - 1]);
            std::exit(-1);
        }
    }
    return hashes;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::printf("usage: %s N [NUM_THREADS]\n\r", argv[0]);
        std::exit(-1);
    }
    int n = atoi(argv[1]);

    int threads = 1;
    if (argc > 2)
        threads = atoi(argv[2]);

    if (const char *p = getenv("USE_CACHE"))
        USE_CACHE = atoi(p);
    gen(n, threads);
    return 0;
}
