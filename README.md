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
| [Abseil flat_map](https://abseil.io/docs/cpp/guides/container)               | C++     | 85.3 ns ± 5.2 ns             | 11,746,981.0 ± 734,149.8     | 65.9 ns ± 1.2 ns             | 15,183,062.7 ± 287,986.5     |
| [HashMap](https://www.baeldung.com/java-hashmap) Java HashMap                | Java    | 89.8 ns ± 3.7 ns             | 11,150,418.3 ± 453,103.8     | 62.9 ns ± 1.2 ns             | 15,899,045.3 ± 313,619.3     |
| [Go Map](https://pkg.go.dev/builtin#map)                                     | Golang  | 91.1 ns ± 2.2 ns             | 10,979,438.3 ± 266,862.7     | 351.2 ns ± 5.1 ns            | 2,847,619.7 ± 41,180.2       |
| [unordered_map](https://cppreference.net/cpp/container/unordered_map.html)   | C++     | 167.6 ns ± 18.0 ns           | 6,016,462.0 ± 677,027.8      | 167.2 ns ± 18.9 ns           | 6,032,597.3 ± 698,454.0      |
| [CDB](https://cr.yp.to/cdb.html) Pure Go Version with mmap support           | Golang  | 195.9 ns ± 2.9 ns            | 5,106,015.0 ± 75,648.4       | 216.4 ns ± 4.9 ns            | 4,622,979.7 ± 104,559.9      |
| [AxonCache](https://github.com/AppLovin/AxonCache) C api                     | C++     | 232.8 ns ± 28.6 ns           | 4,342,591.7 ± 573,033.4      | 183.4 ns ± 15.6 ns           | 5,479,643.0 ± 465,834.3      |
| [AxonCache](https://github.com/AppLovin/AxonCache) Python                    | Python  | 576.6 ns ± 3.9 ns            | 1,734,503.0 ± 11,756.9       | 420.5 ns ± 1.0 ns            | 2,377,983.0 ± 5,603.8        |
| [AxonCache](https://github.com/AppLovin/AxonCache) Golang                    | Golang  | 629.2 ns ± 4.4 ns            | 1,589,466.3 ± 11,088.6       | 332.4 ns ± 6.2 ns            | 3,009,274.0 ± 56,410.9       |
| [LMDB](https://github.com/jnwatson/py-lmdb/) Python module                   | Python  | 696.9 ns ± 38.7 ns           | 1,437,862.3 ± 78,008.4       | 787.0 ns ± 17.1 ns           | 1,271,118.0 ± 27,861.4       |
| [LMDB](https://symas.com/lmdb/)                                              | Golang  | 697.7 ns ± 5.4 ns            | 1,433,321.3 ± 11,105.0       | 771.5 ns ± 25.1 ns           | 1,297,036.0 ± 41,643.1       |
| [AxonCache](https://github.com/AppLovin/AxonCache) Java                      | Java    | 771.3 ns ± 36.3 ns           | 1,298,387.3 ± 59,678.6       | 438.4 ns ± 18.4 ns           | 2,283,598.3 ± 93,412.0       |
| [CDB](https://github.com/bbayles/python-pure-cdb) Pure Python module         | Python  | 2,265.2 ns ± 24.1 ns         | 441,493.0 ± 4,669.0          | 2,467.4 ns ± 12.9 ns         | 405,285.7 ± 2,118.3          |
| [LevelDB](https://github.com/syndtr/goleveldb) Pure Go version               | Golang  | 4,655.2 ns ± 33.3 ns         | 214,820.3 ± 1,540.2          | 2,116.2 ns ± 179.7 ns        | 474,951.7 ± 42,401.8         |

### AMD EPYC 9B14, 2023 (Linux) 

| Implementation                                                               | Runtime | Lookups Lat (ns ± sd)        | Lookups QPS (avg ± sd)       | Inserts Lat (ns ± sd)        | Inserts QPS (avg ± sd)       |
|------------------------------------------------------------------------------|---------|------------------------------|------------------------------|------------------------------|------------------------------|
| [HashMap](https://www.baeldung.com/java-hashmap) Java HashMap                | Java    | 108.4 ns ± 3.3 ns            | 9,234,725.0 ± 289,826.6      | 81.7 ns ± 3.5 ns             | 12,261,440.0 ± 517,465.1     |
| [Abseil flat_map](https://abseil.io/docs/cpp/guides/container)               | C++     | 109.3 ns ± 4.8 ns            | 9,160,685.7 ± 394,750.1      | 180.1 ns ± 3.8 ns            | 5,555,499.0 ± 116,995.3      |
| [Go Map](https://pkg.go.dev/builtin#map)                                     | Golang  | 121.8 ns ± 4.4 ns            | 8,214,028.7 ± 295,543.2      | 438.4 ns ± 23.4 ns           | 2,285,261.3 ± 119,988.0      |
| [unordered_map](https://cppreference.net/cpp/container/unordered_map.html)   | C++     | 195.2 ns ± 5.7 ns            | 5,124,893.0 ± 147,590.7      | 210.2 ns ± 7.5 ns            | 4,762,472.7 ± 173,526.7      |
| [CDB](https://cr.yp.to/cdb.html) Pure Go Version with mmap support           | Golang  | 232.0 ns ± 4.7 ns            | 4,311,994.3 ± 86,746.6       | 248.3 ns ± 7.4 ns            | 4,029,494.3 ± 120,875.4      |
| [AxonCache](https://github.com/AppLovin/AxonCache) C api                     | C++     | 249.9 ns ± 23.2 ns           | 4,025,945.7 ± 382,301.7      | 309.4 ns ± 104.3 ns          | 3,567,297.0 ± 1,493,332.5    |
| [AxonCache](https://github.com/AppLovin/AxonCache) Golang                    | Golang  | 572.7 ns ± 13.8 ns           | 1,746,638.0 ± 41,517.5       | 439.9 ns ± 135.3 ns          | 2,401,900.3 ± 627,479.0      |
| [AxonCache](https://github.com/AppLovin/AxonCache) Python                    | Python  | 647.8 ns ± 11.8 ns           | 1,544,017.0 ± 28,155.0       | 475.6 ns ± 13.7 ns           | 2,103,907.3 ± 61,123.4       |
| [LMDB](https://symas.com/lmdb/)                                              | Golang  | 760.2 ns ± 20.5 ns           | 1,316,093.7 ± 35,843.7       | 1,233.2 ns ± 245.2 ns        | 835,068.3 ± 182,968.1        |
| [LMDB](https://github.com/jnwatson/py-lmdb/) Python module                   | Python  | 770.9 ns ± 30.0 ns           | 1,298,435.7 ± 49,833.3       | 1,320.8 ns ± 453.6 ns        | 813,748.0 ± 250,161.7        |
| [AxonCache](https://github.com/AppLovin/AxonCache) Java                      | Java    | 788.7 ns ± 24.3 ns           | 1,268,719.3 ± 38,968.4       | 442.9 ns ± 10.3 ns           | 2,258,602.0 ± 53,368.5       |
| [CDB](https://github.com/bbayles/python-pure-cdb) Pure Python module         | Python  | 2,361.5 ns ± 43.4 ns         | 423,550.3 ± 7,840.4          | 3,274.9 ns ± 7.1 ns          | 305,351.3 ± 660.9            |
| [LevelDB](https://github.com/syndtr/goleveldb) Pure Go version               | Golang  | 6,033.6 ns ± 664.0 ns        | 167,006.3 ± 17,280.9         | 2,724.0 ns ± 620.5 ns        | 378,696.0 ± 76,347.0         |

### AMD EPYC 7B12 (Linux)

| Implementation                                                               | Runtime | Lookups Lat (ns ± sd)        | Lookups QPS (avg ± sd)       | Inserts Lat (ns ± sd)        | Inserts QPS (avg ± sd)       |
|------------------------------------------------------------------------------|---------|------------------------------|------------------------------|------------------------------|------------------------------|
| [Abseil flat_map](https://abseil.io/docs/cpp/guides/container)               | C++     | 191.1 ns ± 19.5 ns           | 5,269,891.0 ± 563,347.5      | 384.3 ns ± 62.8 ns           | 2,652,893.7 ± 464,454.4      |
| [HashMap](https://www.baeldung.com/java-hashmap) Java HashMap                | Java    | 296.9 ns ± 16.2 ns           | 3,374,606.3 ± 182,203.7      | 101.7 ns ± 6.8 ns            | 9,866,789.0 ± 673,832.8      |
| [Go Map](https://pkg.go.dev/builtin#map)                                     | Golang  | 346.3 ns ± 65.0 ns           | 2,965,305.7 ± 619,114.5      | 870.7 ns ± 104.2 ns          | 1,159,671.0 ± 139,894.5      |
| [unordered_map](https://cppreference.net/cpp/container/unordered_map.html)   | C++     | 429.2 ns ± 71.8 ns           | 2,378,179.0 ± 434,652.3      | 396.7 ns ± 63.9 ns           | 2,563,279.7 ± 399,810.5      |
| [AxonCache](https://github.com/AppLovin/AxonCache) C api                     | C++     | 510.1 ns ± 56.5 ns           | 1,976,509.7 ± 217,779.8      | 380.9 ns ± 26.8 ns           | 2,633,678.7 ± 179,925.7      |
| [CDB](https://cr.yp.to/cdb.html) Pure Go Version with mmap support           | Golang  | 519.2 ns ± 109.2 ns          | 1,980,709.7 ± 393,351.7      | 407.9 ns ± 13.6 ns           | 2,453,656.7 ± 83,388.7       |
| [AxonCache](https://github.com/AppLovin/AxonCache) Golang                    | Golang  | 956.6 ns ± 111.9 ns          | 1,055,312.0 ± 127,558.8      | 649.8 ns ± 85.5 ns           | 1,558,353.0 ± 220,768.4      |
| [AxonCache](https://github.com/AppLovin/AxonCache) Python                    | Python  | 1,198.4 ns ± 33.6 ns         | 834,901.7 ± 23,820.6         | 834.7 ns ± 57.5 ns           | 1,201,836.3 ± 82,781.7       |
| [AxonCache](https://github.com/AppLovin/AxonCache) Java                      | Java    | 1,488.0 ns ± 110.5 ns        | 674,624.7 ± 52,278.2         | 789.7 ns ± 54.8 ns           | 1,270,321.7 ± 85,915.3       |
| [LMDB](https://github.com/jnwatson/py-lmdb/) Python module                   | Python  | 1,559.3 ns ± 92.1 ns         | 642,807.0 ± 38,486.1         | 2,108.3 ns ± 14.0 ns         | 474,318.7 ± 3,151.3          |
| [LMDB](https://symas.com/lmdb/)                                              | Golang  | 1,570.8 ns ± 108.4 ns        | 638,557.7 ± 42,477.8         | 1,836.1 ns ± 56.1 ns         | 544,963.0 ± 16,470.8         |
| [CDB](https://github.com/bbayles/python-pure-cdb) Pure Python module         | Python  | 5,986.3 ns ± 232.8 ns        | 167,219.0 ± 6,644.1          | 5,726.1 ns ± 207.8 ns        | 174,789.0 ± 6,218.2          |
| [LevelDB](https://github.com/syndtr/goleveldb) Pure Go version               | Golang  | 9,339.8 ns ± 489.9 ns        | 107,263.0 ± 5,558.0          | 4,754.1 ns ± 406.4 ns        | 211,362.0 ± 17,871.9         |

Few notes.

1. Go maps are quite fast for random lookups. They have been re-implemented in Go 1.24 to use a [Swiss Table](https://go.dev/blog/swisstable), which is also the base for abseil flat_map.
2. std::unordered_map are fast for insertion but not as good for lookup. It is a known issue that they are not the fastest, Abseil flat_map is much faster.
3. Pure implementation of the CDB algorithm does not perform well in Python, even if using mmap, showing the overhead of this language.
4. Linux performance are not great, but it can be linked to the glibc malloc which is lagging behind other allocators, and also this AMD processor is much older than the recent Apple Max processor. Insertion speed in abseil flat_map is 7x slower.

### C Api Axon Cache only

| Processor    | Lookups                      | QPS                          |  Speed ratio versus N2D processor type  |
|--------------|------------------------------|------------------------------|-----------------------------------------|
| Apple Max    | 71.7 ns ± 8.9 ns             | 14,088,554.7 ± 1,638,238.6   |  5.89x                                  |
| Google Axion | 217.6 ns ± 7.1 ns            | 4,599,040.3 ± 151,279.1      |  1.94x                                  |
| C3D          | 262.9 ns ± 15.7 ns           | 3,812,584.3 ± 227,097.6      |  1.6x                                   |
| N2D          | 422.8 ns ± 15.5 ns           | 2,367,359.3 ± 86,098.0       |  1                                      |

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
docker build -t alcache --network=host ./
docker run --rm --network=host alcache
```

### Limitations

The largest possible alcache file is about 274GB, reference up to 274,877,906,944 bytes, i.e. 274GB
