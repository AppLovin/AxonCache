## Python bindings

FIXME: Make sure we re-open LMDB in read-only mode. This made lmdb faster in the golang bench.

```
Using AxonCache
Inserted 1,000,000 keys in 0.185s (5,395,311 keys/sec)
Looked up 1,000,000 keys in 0.229s (4,370,646 keys/sec)

Using LMDB
Inserted 1,000,000 keys in 0.513s (1,947,860 keys/sec)
Looked up 1,000,000 keys in 0.445s (2,247,190 keys/sec)

Using Python CDB
Inserted 1,000,000 keys in 1.048s (954,070 keys/sec)
Looked up 1,000,000 keys in 1.003s (996,745 keys/sec)
```
