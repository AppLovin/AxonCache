## Golang bindings

### Memory usage

```
$ go run cmd/*.go benchmark
INFO[0000] Using AxonCache
INFO[0001] Inserted 5,000,000 keys in 0.870s (5,745,252 keys/sec)
INFO[0003] Looked up 5,000,000 keys in 1.55 seconds (3,218,990 keys/sec)
INFO[0003]
INFO[0002] Peak RSS: 1.0 GB
$ 
$ 
$ go run cmd/*.go benchmark
INFO[0000] Using LMDB                                   
INFO[0001] Inserted 5,000,000 keys in 1.326s (3,770,889 keys/sec) 
INFO[0006] Looked up 5,000,000 keys in 4.27 seconds (1,169,607 keys/sec) 
INFO[0006]                                              
INFO[0006] Peak RSS: 1.2 GB                             
$ 
$ 
$ go run cmd/*.go benchmark
INFO[0000] Using BoltDB                                 
INFO[0002] Inserted 5,000,000 keys in 1.670s (2,994,719 keys/sec) 
INFO[0006] Looked up 5,000,000 keys in 4.15 seconds (1,206,020 keys/sec) 
INFO[0006]                                              
INFO[0006] Peak RSS: 2.4 GB                             
$ 
$ 
$ go run cmd/*.go benchmark
INFO[0000] Using LevelDB                                
INFO[0007] Inserted 5,000,000 keys in 7.269s (687,852 keys/sec) 
INFO[0020] Inserted 5,000,000 keys in 12.570s (397,780 keys/sec) 
INFO[0020]                                              
INFO[0020] Peak RSS: 2.1 GB                             
```

## Python bindings

```
Using AxonCache
Inserted 5,000,000 keys in 1.200s (4,168,045 keys/sec)
Looked up 5,000,000 keys in 1.557s (3,212,234 keys/sec)

Using LMDB
Inserted 5,000,000 keys in 3.752s (1,332,479 keys/sec)
Looked up 5,000,000 keys in 3.592s (1,391,929 keys/sec)
```
