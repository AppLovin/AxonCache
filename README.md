# AxonCache

This library provides a rich key value store backed by files, that are memory mapped.

1. Rich means that values can be string, int, bool and float, but also list of floats and list of strings, all of them being stored in memory in little endian, so that they can be accessed as is without conversion from string to a pod type, typically done with atoi, strtod etc...
2. By being memory mapped, the data is available rather instantly without a parsing step, that say would scan over all the keys to store them in a C++ unordered_map, or a golang map, which offer a form of speed upper bound. AxonCache lookups are slower than a pure map lookup by 4.4x in our example for Go. However multiple pods or process can share that memory, without RSS memory going up, and as the dataset grows (ours are beyond 20G), the parsing step becomes slower and will take multiple minutes. Lookups are still sub microsecond, measured to 230 ns in one benchmark, which is many orders of magnitude faster than a typical network call (typically 1ms on an already opened TCP connection).
3. This library was benched (through the golang/cgo binding, where going through that bridge incur a slowdown) against other well known Go libraries, and is consistently faster than them at generation and lookup time. An internal bench also showed the library being 3x faster than Meta CacheLib. The CDB library (Constant database), re-implemented in native Golang with mmap is actually faster than AxonCache. However the library has an internal 32 bits limitation, and cannot hold more than 4G of data.
4. The library is written in C++, with a C interface. That interface is used to make a Go, a Java and a Python module.
5. There are minimum dependencies, which are bundled, xxhash and fast_float, fast_float could arguably be removed. Everything builds on a mac in seconds, without any required installed library. Only cmake is required for the C++ build. The library is built on Linux and macOS
6. The library is well unit tested and also tested in production code, it has been used as is for multiple years, and more than 10 years for its previous incarnation.
7. There is an optional value deduplication technique, to avoid storing multiple copies of repeated values. As dataset goes wild this allowed us to reduce the in memory working set at little cost.
8. All the code and modules is seriously exercised and tested in CI with github actions.

An internal system not open-sourced yet is used to generate cache files (populate) from various databases content, and ship it on remote servers (datamover). That system will be open-sourced in the future, but is built on this library.

There is no namespace concept, just a flat space. In practice our applications use a resource id followed by a dot and then they key name, which is typical in key value stores.

The library contains no mutex. In Go and Java, a new atomic pointer is used for each lookup to simply implement concurrency, so that a new cache can be swapped from the previous one transparently. In our C++ servers a similar technique is used through shared pointers.

## Benchmark

```
# Golang
go run cmd/{benchmark.go,kv_scanner.go,main.go,progress.go} benchmark
```

```
# C++
./build.sh -x -b Release
./build/main/axoncache_cli --bench
```

```
# Python ; create + activate a virtualenv, install a few modules
sh axoncache/build_python_module.sh
python3 axoncache/bench.py 
```

```
# Java
./build.sh -J -b Release
sh java/run_benchmark.sh
```

```
# Running all benchmarks
python3 benchmark/run_all_cli_benchmarks.py 
```

The benchmark is inserting 1,000,000 small keys (key_%i, val_%i), then randomly looking them up.

### Apple M4 Max laptop (macOS)

| Implementation                                                               | Runtime | Lookups Lat (ns ± sd)        | Lookups QPS (avg ± sd)       | Inserts Lat (ns ± sd)        | Inserts QPS (avg ± sd)       |
|------------------------------------------------------------------------------|---------|------------------------------|------------------------------|------------------------------|------------------------------|
| [Abseil flat_map](https://abseil.io/docs/cpp/guides/container)               | C++     | 30.3 ns ± 0.1 ns             | 32,980,574.3 ± 85,392.4      | 25.9 ns ± 0.2 ns             | 38,595,871.3 ± 317,950.2     |
| [AxonCache](https://github.com/AppLovin/AxonCache) C++ api                   | C++     | 35.0 ns ± 0.5 ns             | 28,550,738.0 ± 378,753.4     | 71.0 ns ± 3.9 ns             | 14,105,436.3 ± 773,987.7     |
| [HashMap](https://www.baeldung.com/java-hashmap) Java HashMap                | Java    | 35.5 ns ± 0.3 ns             | 28,156,638.0 ± 211,436.8     | 31.1 ns ± 0.7 ns             | 32,216,312.3 ± 746,667.3     |
| [Go Map](https://pkg.go.dev/builtin#map)                                     | Golang  | 54.8 ns ± 0.4 ns             | 18,263,583.7 ± 143,258.5     | 132.0 ns ± 2.0 ns            | 7,574,165.3 ± 115,968.0      |
| [AxonCache](https://github.com/AppLovin/AxonCache) C api                     | C++     | 64.4 ns ± 0.6 ns             | 15,531,983.0 ± 153,817.7     | 74.5 ns ± 7.0 ns             | 13,503,343.3 ± 1,276,015.8   |
| [CDB](https://cr.yp.to/cdb.html) Pure Go Version with mmap support           | Golang  | 74.4 ns ± 4.4 ns             | 13,470,030.0 ± 764,286.8     | 72.8 ns ± 6.1 ns             | 13,791,817.7 ± 1,167,396.0   |
| [unordered_map](https://cppreference.net/cpp/container/unordered_map.html)   | C++     | 87.3 ns ± 3.9 ns             | 11,466,906.3 ± 499,163.2     | 45.4 ns ± 1.6 ns             | 22,048,264.3 ± 778,087.3     |
| [AxonCache](https://github.com/AppLovin/AxonCache) Golang                    | Golang  | 202.7 ns ± 2.6 ns            | 4,934,997.0 ± 62,211.2       | 114.2 ns ± 1.6 ns            | 8,761,327.7 ± 121,883.2      |
| [AxonCache](https://github.com/AppLovin/AxonCache) Python                    | Python  | 238.1 ns ± 3.7 ns            | 4,200,177.0 ± 64,997.6       | 195.5 ns ± 5.9 ns            | 5,117,427.7 ± 151,715.8      |
| [LMDB](https://symas.com/lmdb/)                                              | Golang  | 423.7 ns ± 7.1 ns            | 2,360,526.3 ± 39,496.7       | 438.2 ns ± 7.9 ns            | 2,282,780.3 ± 41,408.5       |
| [AxonCache](https://github.com/AppLovin/AxonCache) Java                      | Java    | 439.7 ns ± 1.3 ns            | 2,274,098.3 ± 6,872.6        | 349.9 ns ± 10.9 ns           | 2,859,482.3 ± 88,095.7       |
| [LMDB](https://github.com/jnwatson/py-lmdb/) Python module                   | Python  | 472.8 ns ± 12.9 ns           | 2,116,127.7 ± 58,284.0       | 481.9 ns ± 4.0 ns            | 2,075,374.0 ± 17,348.5       |
| [CDB](https://github.com/bbayles/python-pure-cdb) Pure Python module         | Python  | 1,012.1 ns ± 4.9 ns          | 988,054.7 ± 4,789.5          | 1,046.0 ns ± 7.4 ns          | 956,043.0 ± 6,785.1          |
| [LevelDB](https://github.com/syndtr/goleveldb) Pure Go version               | Golang  | 2,343.0 ns ± 44.2 ns         | 426,908.3 ± 8,121.8          | 794.9 ns ± 18.8 ns           | 1,258,499.7 ± 29,485.8       |

### Google Arm-based Axion CPUs, 4th generation, 2024 (Linux)

| Implementation                                                               | Runtime | Lookups Lat (ns ± sd)        | Lookups QPS (avg ± sd)       | Inserts Lat (ns ± sd)        | Inserts QPS (avg ± sd)       |
|------------------------------------------------------------------------------|---------|------------------------------|------------------------------|------------------------------|------------------------------|
| [Abseil flat_map](https://abseil.io/docs/cpp/guides/container)               | C++     | 88.8 ns ± 0.7 ns             | 11,262,235.0 ± 90,139.0      | 71.4 ns ± 4.4 ns             | 14,033,405.3 ± 838,997.2     |
| [HashMap](https://www.baeldung.com/java-hashmap) Java HashMap                | Java    | 90.1 ns ± 0.9 ns             | 11,105,157.7 ± 115,620.9     | 61.6 ns ± 0.9 ns             | 16,227,351.0 ± 232,993.8     |
| [Go Map](https://pkg.go.dev/builtin#map)                                     | Golang  | 97.8 ns ± 2.5 ns             | 10,229,643.3 ± 260,877.9     | 365.0 ns ± 7.3 ns            | 2,740,230.7 ± 54,024.9       |
| [AxonCache](https://github.com/AppLovin/AxonCache) C++ api                   | C++     | 111.2 ns ± 2.7 ns            | 8,993,234.3 ± 220,938.7      | 185.1 ns ± 3.1 ns            | 5,402,357.0 ± 92,212.4       |
| [unordered_map](https://cppreference.net/cpp/container/unordered_map.html)   | C++     | 171.8 ns ± 2.2 ns            | 5,819,927.7 ± 73,649.8       | 174.3 ns ± 6.8 ns            | 5,743,239.7 ± 226,170.8      |
| [CDB](https://cr.yp.to/cdb.html) Pure Go Version with mmap support           | Golang  | 211.5 ns ± 2.0 ns            | 4,728,739.0 ± 45,302.7       | 228.3 ns ± 8.5 ns            | 4,384,270.0 ± 166,907.6      |
| [AxonCache](https://github.com/AppLovin/AxonCache) C api                     | C++     | 243.6 ns ± 4.2 ns            | 4,106,477.3 ± 71,473.6       | 166.1 ns ± 3.6 ns            | 6,021,801.3 ± 131,474.9      |
| [AxonCache](https://github.com/AppLovin/AxonCache) Python                    | Python  | 628.6 ns ± 6.2 ns            | 1,590,894.0 ± 15,674.6       | 443.5 ns ± 1.2 ns            | 2,254,731.3 ± 6,092.1        |
| [AxonCache](https://github.com/AppLovin/AxonCache) Golang                    | Golang  | 675.5 ns ± 6.1 ns            | 1,480,400.3 ± 13,436.3       | 364.2 ns ± 4.3 ns            | 2,746,083.0 ± 32,364.3       |
| [AxonCache](https://github.com/AppLovin/AxonCache) Java                      | Java    | 701.7 ns ± 5.5 ns            | 1,425,126.7 ± 11,084.8       | 441.1 ns ± 4.2 ns            | 2,267,183.0 ± 21,487.5       |
| [LMDB](https://github.com/jnwatson/py-lmdb/) Python module                   | Python  | 713.7 ns ± 3.4 ns            | 1,401,160.7 ± 6,648.9        | 809.9 ns ± 13.9 ns           | 1,235,033.3 ± 21,348.8       |
| [LMDB](https://symas.com/lmdb/)                                              | Golang  | 724.9 ns ± 5.8 ns            | 1,379,644.3 ± 11,033.7       | 781.8 ns ± 2.9 ns            | 1,279,161.3 ± 4,766.9        |
| [CDB](https://github.com/bbayles/python-pure-cdb) Pure Python module         | Python  | 2,301.6 ns ± 18.6 ns         | 434,495.3 ± 3,502.5          | 2,493.8 ns ± 3.9 ns          | 400,992.0 ± 631.5            |
| [LevelDB](https://github.com/syndtr/goleveldb) Pure Go version               | Golang  | 4,714.8 ns ± 12.1 ns         | 212,099.0 ± 544.6            | 2,197.5 ns ± 3.1 ns          | 455,068.3 ± 651.6            |

### AMD EPYC 7B12 (Linux)

| Implementation                                                               | Runtime | Lookups Lat (ns ± sd)        | Lookups QPS (avg ± sd)       | Inserts Lat (ns ± sd)        | Inserts QPS (avg ± sd)       |
|------------------------------------------------------------------------------|---------|------------------------------|------------------------------|------------------------------|------------------------------|
| [Abseil flat_map](https://abseil.io/docs/cpp/guides/container)               | C++     | 195.9 ns ± 31.6 ns           | 5,191,176.7 ± 796,719.2      | 422.1 ns ± 4.7 ns            | 2,369,069.0 ± 26,272.2       |
| [HashMap](https://www.baeldung.com/java-hashmap) Java HashMap                | Java    | 285.9 ns ± 32.8 ns           | 3,526,868.0 ± 379,702.2      | 106.9 ns ± 19.6 ns           | 9,551,814.0 ± 1,593,234.4    |
| [Go Map](https://pkg.go.dev/builtin#map)                                     | Golang  | 313.7 ns ± 45.8 ns           | 3,237,071.3 ± 502,829.8      | 897.8 ns ± 133.5 ns          | 1,131,997.0 ± 183,154.9      |
| [unordered_map](https://cppreference.net/cpp/container/unordered_map.html)   | C++     | 377.2 ns ± 101.4 ns          | 2,785,655.7 ± 762,162.5      | 380.2 ns ± 90.3 ns           | 2,723,037.7 ± 588,602.1      |
| [AxonCache](https://github.com/AppLovin/AxonCache) C++ api                   | C++     | 436.4 ns ± 92.7 ns           | 2,368,694.3 ± 548,643.7      | 355.8 ns ± 18.9 ns           | 2,816,137.3 ± 152,714.2      |
| [CDB](https://cr.yp.to/cdb.html) Pure Go Version with mmap support           | Golang  | 500.8 ns ± 70.4 ns           | 2,025,216.7 ± 305,559.7      | 431.2 ns ± 46.2 ns           | 2,337,488.7 ± 255,485.5      |
| [AxonCache](https://github.com/AppLovin/AxonCache) C api                     | C++     | 506.8 ns ± 64.3 ns           | 1,995,967.7 ± 268,917.3      | 336.7 ns ± 52.4 ns           | 3,015,066.7 ± 431,642.0      |
| [AxonCache](https://github.com/AppLovin/AxonCache) Golang                    | Golang  | 998.7 ns ± 64.7 ns           | 1,004,177.0 ± 65,697.4       | 668.2 ns ± 97.7 ns           | 1,517,844.3 ± 218,351.4      |
| [AxonCache](https://github.com/AppLovin/AxonCache) Python                    | Python  | 1,180.1 ns ± 49.8 ns         | 848,426.7 ± 35,990.4         | 868.9 ns ± 7.2 ns            | 1,150,883.3 ± 9,583.5        |
| [AxonCache](https://github.com/AppLovin/AxonCache) Java                      | Java    | 1,350.1 ns ± 47.7 ns         | 741,316.3 ± 25,833.3         | 838.2 ns ± 39.7 ns           | 1,194,932.3 ± 58,222.9       |
| [LMDB](https://symas.com/lmdb/)                                              | Golang  | 1,494.1 ns ± 75.5 ns         | 670,473.0 ± 34,505.4         | 1,919.0 ns ± 20.1 ns         | 521,149.0 ± 5,430.3          |
| [LMDB](https://github.com/jnwatson/py-lmdb/) Python module                   | Python  | 1,681.1 ns ± 63.9 ns         | 595,435.0 ± 22,991.9         | 2,113.8 ns ± 74.0 ns         | 473,466.7 ± 16,406.0         |
| [CDB](https://github.com/bbayles/python-pure-cdb) Pure Python module         | Python  | 5,934.1 ns ± 186.8 ns        | 168,628.0 ± 5,247.5          | 5,841.0 ns ± 178.1 ns        | 171,308.7 ± 5,190.6          |
| [LevelDB](https://github.com/syndtr/goleveldb) Pure Go version               | Golang  | 9,451.7 ns ± 477.6 ns        | 105,986.0 ± 5,496.5          | 5,023.9 ns ± 190.6 ns        | 199,238.0 ± 7,504.6          |

Few notes.

1. Go maps are quite fast for random lookups. They have been re-implemented in Go 1.24 to use a [Swiss Table](https://go.dev/blog/swisstable), which is also the base for abseil flat_map.
2. std::unordered_map are fast for insertion but not as good for lookup. It is a known issue that they are not the fastest, Abseil flat_map is much faster.
3. Pure implementation of the CDB algorithm does not perform well in Python, even if using mmap, showing the overhead of this language.
4. Linux performance are not great, but it can be linked to the glibc malloc which is lagging behind other allocators, and also this AMD processor is much older than the recent Apple Max processor. Insertion speed in abseil flat_map is 7x slower.

### C Api Axon Cache only

| Processor    | Lookups                      | QPS                          |  Speed ratio versus N2D processor type  |
|--------------|------------------------------|------------------------------|-----------------------------------------|
| Apple Max    | 35.0 ns ± 8.9 ns             | 28,550,738.0 ± 378,753.4     |  12.4x                                  |
| Google Axion | 111.2 ns ± 2.7 ns            | 8,993,234.3 ± 220,938.7      |  3.92x                                  |
| N2D          | 436.4 ns ± 92.7 ns           | 2,368,694.3 ± 548,643.7      |  1                                      |

## C++ build steps.

The Github actions are the best places to see how each bindings and CLI are built.

Use the following commands from the project root directory to run the test suite.

```
./build.sh -t
```

## Bindings build steps

```
sudo apt install openjdk-17-jdk
sudo apt install maven
```

### Sanitizers

Sanitizers can be enabled by configuring CMake with `-DUSE_SANITIZER=<Address | Memory | MemoryWithOrigins | Undefined | Thread | Leak | 'Address;Undefined'>`.

### Docker build

```
docker build -t alcache ./
docker run --rm --network=host alcache
```

### Limitations

The largest possible alcache file is about 274GB, reference up to 274,877,906,944 bytes, i.e. 274GB
