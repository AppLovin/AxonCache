## Python bindings

FIXME: Make sure we re-open LMDB in read-only mode. This made lmdb faster in the golang bench.

```
Using AxonCache
Inserted 5,000,000 keys in 1.200s (4,168,045 keys/sec)
Looked up 5,000,000 keys in 1.557s (3,212,234 keys/sec)

Using LMDB
Inserted 5,000,000 keys in 3.752s (1,332,479 keys/sec)
Looked up 5,000,000 keys in 3.592s (1,391,929 keys/sec)
```
