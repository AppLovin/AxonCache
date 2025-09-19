# Tested on macOS
# brew install rocksdb

CGO_CFLAGS="-I/opt/homebrew/include" \
CGO_LDFLAGS="-L/opt/homebrew/lib -lrocksdb -lstdc++ -lm -lz -lsnappy -llz4 -lzstd" \
  go run cmd/rocksdb_benchmark/main.go
