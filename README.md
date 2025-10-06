# AxonCache

This library provides a rich key value store backed by files, that are memory mapped.

1. Rich means that values can be string, int, bool and float, but also list of floats and list of strings, all of them being stored in memory in little endian, so that they can be accessed as is without conversion from string to a pod type, typically done with atoi, strtod etc...
2. By being memory mapped, the data is available rather instantly without a parsing step, that say would scan over all the keys to store them in a C++ unordered_map, or a golang map, which offer a form of speed upper bound. AxonCache lookups are slower than a pure map lookup by 4.4x in our example for Go. However multiple pods or process can share that memory, without RSS memory going up, and as the dataset grows (ours are beyond 20G), the parsing step becomes slower and will take multiple minutes. Lookups are still sub microsecond, measured to 230 ns in one benchmark, which is many orders of magnitude faster than a typical network call (typically 1ms on an already opened TCP connection).
3. This library was benched (through the golang/cgo binding, where going through that bridge incur a slowdown) against other well known Go libraries, and is consistently faster than them at generation and lookup time. An internal bench also showed the library being 3x faster than Meta CacheLib. The CDB library (Constant database), re-implemented in native Golang with mmap is actually faster than AxonCache. However the library has an internal 32 bits limitation, and cannot hold more than 4G of data.
4. The library is written in C++, with a C interface. That interface is used to make a Go and a Python module. A Java module exists as well, but was not open-sourced yet.
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

The benchmark is inserting 1,000,000 small keys (key_%i, val_%i), then randomly looking them up.

## Apple M4 Max laptop (macOS)

| Implementation                                                               | Runtime | Lookups Lat (ns ± sd)        | Lookups QPS (avg ± sd)       | Inserts Lat (ns ± sd)        | Inserts QPS (avg ± sd)       |
|------------------------------------------------------------------------------|---------|------------------------------|------------------------------|------------------------------|------------------------------|
| [Abseil flat_map](https://abseil.io/docs/cpp/guides/container)               | C++     | 30.9 ns ± 1.0 ns             | 32,348,257.7 ± 1,076,943.4   | 26.1 ns ± 1.0 ns             | 38,417,719.3 ± 1,490,519.6   |
| [Go Map](https://pkg.go.dev/builtin#map)                                     | Golang  | 54.2 ns ± 0.2 ns             | 18,443,446.3 ± 77,788.6      | 131.9 ns ± 3.8 ns            | 7,584,756.0 ± 220,305.8      |
| [AxonCache](https://github.com/AppLovin/AxonCache) C api                     | C++     | 71.7 ns ± 8.9 ns             | 14,088,554.7 ± 1,638,238.6   | 162.1 ns ± 109.8 ns          | 8,493,465.3 ± 5,560,106.2    |
| [CDB](https://cr.yp.to/cdb.html) Pure Go Version with mmap support           | Golang  | 73.1 ns ± 1.0 ns             | 13,682,756.0 ± 192,789.3     | 70.4 ns ± 1.7 ns             | 14,213,223.7 ± 336,262.0     |
| [C++ unordered_map](https://cppreference.net/cpp/container/unordered_map.html) | C++     | 93.8 ns ± 4.3 ns             | 10,678,605.3 ± 500,010.9     | 44.6 ns ± 2.3 ns             | 22,476,520.7 ± 1,149,837.4   |
| [AxonCache](https://github.com/AppLovin/AxonCache) Golang                    | Golang  | 206.5 ns ± 3.8 ns            | 4,842,853.0 ± 90,163.3       | 114.3 ns ± 2.5 ns            | 8,753,291.3 ± 195,006.9      |
| [AxonCache](https://github.com/AppLovin/AxonCache) Python                    | Python  | 241.5 ns ± 2.0 ns            | 4,141,026.7 ± 33,790.5       | 251.7 ns ± 79.8 ns           | 4,211,820.7 ± 1,128,887.5    |
| [LMDB](https://symas.com/lmdb/)                                              | Golang  | 443.2 ns ± 21.7 ns           | 2,259,633.3 ± 108,300.6      | 448.2 ns ± 7.3 ns            | 2,231,597.7 ± 36,134.4       |
| [LMDB](https://github.com/jnwatson/py-lmdb/) Python module                   | Python  | 479.1 ns ± 2.2 ns            | 2,087,361.3 ± 9,473.4        | 527.3 ns ± 4.9 ns            | 1,896,732.3 ± 17,557.2       |
| [CDB](https://github.com/bbayles/python-pure-cdb) Pure Python module         | Python  | 1,033.6 ns ± 19.2 ns         | 967,702.7 ± 17,931.6         | 1,135.5 ns ± 55.4 ns         | 882,103.3 ± 43,045.7         |
| [LevelDB](https://github.com/syndtr/goleveldb) Pure Go version               | Golang  | 2,500.2 ns ± 131.1 ns        | 400,692.7 ± 20,518.9         | 888.7 ns ± 124.0 ns          | 1,139,202.7 ± 150,820.3      |

## AMD EPYC 9B14, 2023 (Linux) 

| Implementation                                                               | Runtime | Lookups Lat (ns ± sd)        | Lookups QPS (avg ± sd)       | Inserts Lat (ns ± sd)        | Inserts QPS (avg ± sd)       |
|------------------------------------------------------------------------------|---------|------------------------------|------------------------------|------------------------------|------------------------------|
| [Abseil flat_map](https://abseil.io/docs/cpp/guides/container)               | C++     | 105.7 ns ± 0.9 ns            | 9,462,990.3 ± 83,292.5       | 177.3 ns ± 1.7 ns            | 5,641,735.0 ± 54,399.3       |
| [Go Map](https://pkg.go.dev/builtin#map)                                     | Golang  | 123.8 ns ± 0.8 ns            | 8,075,053.0 ± 53,400.8       | 435.6 ns ± 1.9 ns            | 2,295,975.7 ± 10,257.8       |
| [C++ unordered_map](https://cppreference.net/cpp/container/unordered_map.html) | C++     | 207.0 ns ± 18.7 ns           | 4,855,381.0 ± 425,417.9      | 225.4 ns ± 19.3 ns           | 4,458,307.7 ± 383,697.8      |
| [CDB](https://cr.yp.to/cdb.html) Pure Go Version with mmap support           | Golang  | 230.4 ns ± 5.8 ns            | 4,342,944.7 ± 107,433.7      | 251.4 ns ± 14.7 ns           | 3,986,389.7 ± 231,634.6      |
| [AxonCache](https://github.com/AppLovin/AxonCache) C api                     | C++     | 262.9 ns ± 15.7 ns           | 3,812,584.3 ± 227,097.6      | 296.8 ns ± 105.0 ns          | 3,763,679.3 ± 1,670,087.3    |
| [AxonCache](https://github.com/AppLovin/AxonCache) Golang                    | Golang  | 567.0 ns ± 16.6 ns           | 1,764,625.0 ± 52,377.5       | 458.4 ns ± 65.1 ns           | 2,213,451.3 ± 336,630.5      |
| [AxonCache](https://github.com/AppLovin/AxonCache) Python                    | Python  | 604.4 ns ± 5.2 ns            | 1,654,657.7 ± 14,214.7       | 469.2 ns ± 13.0 ns           | 2,132,586.0 ± 58,291.4       |
| [LMDB](https://github.com/jnwatson/py-lmdb/) Python module                   | Python  | 739.8 ns ± 13.5 ns           | 1,352,040.0 ± 24,739.3       | 1,094.8 ns ± 181.1 ns        | 932,224.3 ± 170,514.1        |
| [LMDB](https://symas.com/lmdb/)                                              | Golang  | 743.3 ns ± 49.0 ns           | 1,349,462.3 ± 91,933.8       | 1,127.5 ns ± 201.1 ns        | 908,331.3 ± 180,186.3        |
| [CDB](https://github.com/bbayles/python-pure-cdb) Pure Python module         | Python  | 2,334.4 ns ± 55.5 ns         | 428,544.7 ± 10,329.8         | 3,354.3 ns ± 109.9 ns        | 298,341.0 ± 9,966.3          |
| [LevelDB](https://github.com/syndtr/goleveldb) Pure Go version               | Golang  | 5,910.8 ns ± 520.9 ns        | 170,020.0 ± 14,258.3         | 2,627.0 ns ± 497.5 ns        | 389,164.7 ± 67,401.6         |

## AMD EPYC 7B12 (Linux)

| Implementation                                                               | Runtime | Lookups Lat (ns ± sd)        | Lookups QPS (avg ± sd)       | Inserts Lat (ns ± sd)        | Inserts QPS (avg ± sd)       |
|------------------------------------------------------------------------------|---------|------------------------------|------------------------------|------------------------------|------------------------------|
| [Abseil flat_map](https://abseil.io/docs/cpp/guides/container)               | C++     | 157.9 ns ± 22.4 ns           | 6,421,629.3 ± 955,968.2      | 317.4 ns ± 13.4 ns           | 3,154,553.0 ± 129,945.6      |
| [Go Map](https://pkg.go.dev/builtin#map)                                     | Golang  | 266.4 ns ± 13.2 ns           | 3,760,096.3 ± 191,691.7      | 740.5 ns ± 8.5 ns            | 1,350,525.7 ± 15,579.8       |
| [C++ unordered_map](https://cppreference.net/cpp/container/unordered_map.html) | C++     | 354.0 ns ± 10.1 ns           | 2,826,214.0 ± 79,682.6       | 351.8 ns ± 11.9 ns           | 2,844,836.7 ± 94,264.7       |
| [AxonCache](https://github.com/AppLovin/AxonCache) C api                     | C++     | 422.8 ns ± 15.5 ns           | 2,367,359.3 ± 86,098.0       | 299.7 ns ± 4.0 ns            | 3,337,264.7 ± 43,856.3       |
| [CDB](https://cr.yp.to/cdb.html) Pure Go Version with mmap support           | Golang  | 430.2 ns ± 8.3 ns            | 2,325,189.0 ± 44,347.7       | 347.2 ns ± 7.9 ns            | 2,880,887.0 ± 65,614.9       |
| [AxonCache](https://github.com/AppLovin/AxonCache) Golang                    | Golang  | 842.4 ns ± 26.7 ns           | 1,187,877.7 ± 38,246.2       | 527.9 ns ± 13.1 ns           | 1,894,937.7 ± 46,223.8       |
| [AxonCache](https://github.com/AppLovin/AxonCache) Python                    | Python  | 981.8 ns ± 25.0 ns           | 1,018,934.3 ± 25,567.6       | 769.4 ns ± 32.2 ns           | 1,301,174.3 ± 53,264.2       |
| [LMDB](https://symas.com/lmdb/)                                              | Golang  | 1,250.7 ns ± 13.0 ns         | 799,590.3 ± 8,331.2          | 1,530.1 ns ± 12.5 ns         | 653,567.3 ± 5,349.2          |
| [LMDB](https://github.com/jnwatson/py-lmdb/) Python module                   | Python  | 1,325.3 ns ± 37.9 ns         | 754,952.7 ± 21,956.5         | 1,735.7 ns ± 10.8 ns         | 576,167.7 ± 3,578.8          |
| [CDB](https://github.com/bbayles/python-pure-cdb) Pure Python module         | Python  | 5,795.4 ns ± 112.7 ns        | 172,594.0 ± 3,350.9          | 5,667.4 ns ± 117.6 ns        | 176,499.3 ± 3,658.9          |
| [LevelDB](https://github.com/syndtr/goleveldb) Pure Go version               | Golang  | 8,280.9 ns ± 230.5 ns        | 120,822.0 ± 3,376.6          | 3,814.2 ns ± 101.4 ns        | 262,302.7 ± 7,075.5          |

Few notes.

1. Go maps are quite fast for random lookups. They have been re-implemented in Go 1.24 to use a [Swiss Table](https://go.dev/blog/swisstable), which is also the base for abseil flat_map.
2. std::unordered_map are fast for insertion but not as good for lookup. It is a known issue that they are not the fastest, Abseil flat_map is much faster.
3. Pure implementation of the CDB algorithm does not perform well in Python, even if using mmap, showing the overhead of this language.
4. Linux performance are not great, but it can be linked to the glibc malloc which is lagging behind other allocators, and also this AMD processor is much older than the recent Apple Max processor. Insertion speed in abseil flat_map is 7x slower.

## C++ build steps.

The Github actions are the best places to see how each bindings and CLI are built.

Use the following commands from the project root directory to run the test suite.

```
./build.sh -t
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
