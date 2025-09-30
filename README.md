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
It is run on an Apple M4 Max laptop.

| Library                                                                  | Insertion (keys/s) | Lookup (keys/s) | Runtime |
| --------------------------------------------------                       | ------------------ | ----------------| --------|
| [C++ unordered_map](https://github.com/AppLovin/AxonCache)               | 17,910,969         | 10,115,937      | C++     |
| [Abseil flat_map](https://abseil.io/docs/cpp/guides/container)           | 36,746,862         | 30,718,899      | C++     |
| [AxonCache](https://github.com/AppLovin/AxonCache) C api                 | 13,451,761         | 13,837,095      | C++     |
| [Go Map](https://pkg.go.dev/builtin#map)                                 | 7,792,827          | 19,493,731      | Golang  |
| [AxonCache](https://github.com/AppLovin/AxonCache) Golang                | 8,903,104          | 4,600,516       | Golang  |
| [LMDB](https://symas.com/lmdb/)                                          | 2,263,189          | 2,340,060       | Golang  |
| [LevelDB](https://github.com/syndtr/goleveldb) Pure Go version           | 1,165,081          | 417,783         | Golang  |
| [CDB](https://cr.yp.to/cdb.html) Pure Go Version with mmap support       | 13,872,808         | 14,094,333      | Golang  |
| [AxonCache](https://github.com/AppLovin/AxonCache) Python                | 4,945,699          | 4,281,129       | Python  |
| [LMDB](https://github.com/jnwatson/py-lmdb/) Python module               | 1,933,083          | 2,099,467       | Python  |
| [CDB](https://github.com/bbayles/python-pure-cdb) Pure Python module     | 935,780            | 980,383         | Python  |


Few notes.

1. Go maps are quite fast for random lookups. They have been re-implemented in Go 1.24 to use a [Swiss Table](https://go.dev/blog/swisstable), which is also the base for abseil flat_map.
2. std::unordered_map are fast for insertion but not as good for lookup. It is a known issue that they are not the fastest, Abseil flat_map is much faster.
3. Pure implementation of the CDB algorithm does not perform well in Python, even if using mmap, showing the overhead of this language.

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
