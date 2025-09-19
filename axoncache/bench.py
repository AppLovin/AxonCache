#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 AppLovin. All rights reserved.

import lmdb
import os
import shutil
import tempfile
import time
import random
import random
import time
import os
import shutil
import tempfile
import time
import random

import axoncache
import lmdb
import humanize


def bench_alcache(keys, values):
    print("Using AxonCache")

    n = len(keys)
    number_of_key_slots = 2 * len(keys)

    properties_path = "bench_cache.properties"
    with open(properties_path, "w") as f:
        f.write("ccache.destination_folder=.")

    start = time.time()
    w = axoncache.Writer("bench_cache", properties_path, number_of_key_slots)
    for key, value in zip(keys, values):
        w.insert_key(key, value, 0)

    timestamp = w.finish_cache_creation()
    w.close()

    elapsed = time.time() - start
    qps = float(n) / elapsed
    print(
        "Inserted %s keys in %.3fs (%s keys/sec)"
        % (humanize.intcomma(n), elapsed, humanize.intcomma(int(qps)))
    )

    # if r.contains_key(b"key_123"):
    #     v = r.get_key(b"key_123")      # bytes
    #     t = r.get_key_type(b"key_123") # str
    #     print(v, t)

    random.shuffle(keys)

    r = axoncache.Reader()
    r.update("bench_cache", ".", timestamp)

    start = time.time()
    for key in keys:
        r.get_key(key)

    elapsed = time.time() - start
    qps = float(n) / elapsed
    print(
        "Looked up %s keys in %.3fs (%s keys/sec)"
        % (humanize.intcomma(n), elapsed, humanize.intcomma(int(qps)))
    )

    r.close()


def run_bench(keys, values, map_size=8 * 1024**3, use_tmpdir=True):
    """
    Insert given keys and values into an LMDB database (one per transaction),
    then randomly look them up.
    Args:
        keys   : list of bytes
        values : list of bytes, must match len(keys)
    """
    print("Using LMDB")

    if len(keys) != len(values):
        raise ValueError("keys and values must have the same length")

    n = len(keys)
    DB_PATH = (
        tempfile.mkdtemp(prefix="lmdb-bench-") if use_tmpdir else "./lmdb_bench_db"
    )
    os.makedirs(DB_PATH, exist_ok=True)

    env = lmdb.open(
        DB_PATH,
        map_size=map_size,
        subdir=True,
        max_dbs=1,
        lock=True,
        writemap=True,
        map_async=True,
        max_readers=512,
    )

    # ----- batch inserts, one transaction
    start = time.time()
    with env.begin(write=True) as txn:
        for i in range(n):
            txn.put(keys[i], values[i], overwrite=True)
    env.sync(True)

    elapsed = time.time() - start
    qps = float(n) / elapsed
    print(
        "Inserted %s keys in %.3fs (%s keys/sec)"
        % (humanize.intcomma(n), elapsed, humanize.intcomma(int(qps))),
    )

    # ----- random lookups -----
    shuffled = keys[:]
    random.shuffle(shuffled)

    start = time.time()
    count = 0
    checksum = 0
    with env.begin(write=False) as txn:
        for k in shuffled:
            v = txn.get(k)
            if v is not None:
                checksum ^= v[0]
            count += 1

    elapsed = time.time() - start
    qps = float(n) / elapsed
    print(
        "Looked up %s keys in %.3fs (%s keys/sec)"
        % (humanize.intcomma(n), elapsed, humanize.intcomma(int(qps)))
    )

    if use_tmpdir:
        shutil.rmtree(DB_PATH, ignore_errors=True)
        print("\n(cleaned up temporary DB)")
    else:
        print("\n(DB kept on disk)")


if __name__ == "__main__":
    # Example: small test set (scale up to millions if you want)
    num = 5_000_000
    keys = [f"key_{i}".encode("ascii") for i in range(num)]
    values = [f"value_{i}".encode("ascii") for i in range(num)]

    bench_alcache(keys, values)
    print()
    run_bench(keys, values)
    print()
