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

| Implementation                         | Runtime | Lookups Lat (ns ± sd)        | Lookups QPS (avg ± sd)       | Inserts Lat (ns ± sd)        | Inserts QPS (avg ± sd)       |
|----------------------------------------|---------|------------------------------|------------------------------|------------------------------|------------------------------|
| Abseil flat map                        | C++     | 29.9 ns ± 0.6 ns             | 33,489,471.0 ± 657,374.6     | 25.0 ns ± 0.4 ns             | 40,069,664.7 ± 693,702.1     |
| AxonCache                              | C++     | 59.6 ns ± 0.7 ns             | 16,780,129.3 ± 210,661.9     | 67.6 ns ± 1.9 ns             | 14,808,894.0 ± 411,348.6     |
| Unordered map                          | C++     | 82.7 ns ± 2.9 ns             | 12,096,495.0 ± 421,069.3     | 41.4 ns ± 0.6 ns             | 24,186,031.7 ± 368,718.7     |
| Native go maps                         | Golang  | 54.0 ns ± 0.2 ns             | 18,527,879.0 ± 67,390.0      | 127.2 ns ± 5.6 ns            | 7,869,449.0 ± 340,481.3      |
| CDB                                    | Golang  | 69.0 ns ± 0.8 ns             | 14,488,158.7 ± 169,160.6     | 69.5 ns ± 1.6 ns             | 14,392,223.0 ± 323,034.5     |
| AxonCache                              | Golang  | 189.6 ns ± 3.3 ns            | 5,275,777.0 ± 91,053.4       | 109.8 ns ± 1.9 ns            | 9,108,831.7 ± 161,727.5      |
| LMDB                                   | Golang  | 413.7 ns ± 13.3 ns           | 2,418,565.3 ± 76,624.9       | 442.2 ns ± 8.4 ns            | 2,262,180.0 ± 43,538.2       |
| LevelDB                                | Golang  | 2,366.2 ns ± 16.0 ns         | 422,635.7 ± 2,852.0          | 850.3 ns ± 115.2 ns          | 1,189,503.3 ± 149,606.1      |
| AxonCache                              | Python  | 226.6 ns ± 1.5 ns            | 4,414,157.7 ± 29,872.4       | 207.2 ns ± 16.5 ns           | 4,844,875.3 ± 370,012.5      |
| LMDB                                   | Python  | 457.4 ns ± 18.0 ns           | 2,188,279.3 ± 85,005.5       | 525.0 ns ± 16.6 ns           | 1,905,863.3 ± 59,365.3       |
| Python CDB                             | Python  | 1,015.0 ns ± 10.2 ns         | 985,328.0 ± 9,869.8          | 1,145.9 ns ± 167.8 ns        | 884,331.0 ± 119,376.5        |

## AMD EPYC 9B14 (Linux)

| Library                                                                  | Insertion (keys/s) | Lookup (keys/s) | Runtime |
| --------------------------------------------------                       | ------------------ | ----------------| --------|
| [C++ unordered_map](https://github.com/AppLovin/AxonCache)               | 4,541,219          | 5,195,295       | C++     |
| [Abseil flat_map](https://abseil.io/docs/cpp/guides/container)           | 5,558,335          | 9,476,591       | C++     |
| [AxonCache](https://github.com/AppLovin/AxonCache) C api                 | 5,350,336          | 4,151,917       | C++     |
| [Go Map](https://pkg.go.dev/builtin#map)                                 | 2,240,472          | 7,695,549       | Golang  |
| [AxonCache](https://github.com/AppLovin/AxonCache) Golang                | 2,475,044          | 1,738,299       | Golang  |
| [LMDB](https://symas.com/lmdb/)                                          | 814,048            | 1,257,732       | Golang  |
| [LevelDB](https://github.com/syndtr/goleveldb) Pure Go version           | 297,856            | 155,247         | Golang  |
| [CDB](https://cr.yp.to/cdb.html) Pure Go Version with mmap support       | 3,864,621          | 4,281,072       | Golang  |
| [AxonCache](https://github.com/AppLovin/AxonCache) Python                | 2,100,425          | 1,648,403       | Python  |
| [LMDB](https://github.com/jnwatson/py-lmdb/) Python module               | 896,988            | 1,383,006       | Python  |
| [CDB](https://github.com/bbayles/python-pure-cdb) Pure Python module     | 288,039            | 427,231         | Python  |

## AMD EPYC 7B12 (Linux)

| Library                                                                  | Insertion (keys/s) | Lookup (keys/s) | Runtime |
| --------------------------------------------------                       | ------------------ | ----------------| --------|
| [C++ unordered_map](https://github.com/AppLovin/AxonCache)               | 3,002,962          | 3,075,348       | C++     |
| [Abseil flat_map](https://abseil.io/docs/cpp/guides/container)           | 3,245,658          | 5,690,564       | C++     |
| [AxonCache](https://github.com/AppLovin/AxonCache) C api                 | 3,310,459          | 2,264,290       | C++     |
| [Go Map](https://pkg.go.dev/builtin#map)                                 | 1,406,407          | 4,258,986       | Golang  |
| [AxonCache](https://github.com/AppLovin/AxonCache) Golang                | 1,951,137          | 1,236,280       | Golang  |
| [LMDB](https://symas.com/lmdb/)                                          | 647,308            | 826,295         | Golang  |
| [LevelDB](https://github.com/syndtr/goleveldb) Pure Go version           | 269,288            | 119,708         | Golang  |
| [CDB](https://cr.yp.to/cdb.html) Pure Go Version with mmap support       | 2,839,036          | 2,267,299       | Golang  |
| [AxonCache](https://github.com/AppLovin/AxonCache) Python                | 1,288,613          | 960,134         | Python  |
| [LMDB](https://github.com/jnwatson/py-lmdb/) Python module               | 568,787            | 754,872         | Python  |
| [CDB](https://github.com/bbayles/python-pure-cdb) Pure Python module     | 178,604            | 176,594         | Python  |

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
