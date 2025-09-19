#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 AppLovin. All rights reserved.

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
import unittest

import axoncache


class TestCache(unittest.TestCase):

    def test_insert_and_lookups(self):
        num = 10_000
        keys = [f"key_{i}".encode("ascii") for i in range(num)]
        values = [f"value_{i}".encode("ascii") for i in range(num)]

        number_of_key_slots = 2 * len(keys)

        properties_path = "bench_test_cache.properties"
        with open(properties_path, "w") as f:
            f.write("ccache.destination_folder=.")

        w = axoncache.Writer("bench_test_cache", properties_path, number_of_key_slots)
        for key, value in zip(keys, values):
            w.insert_key(key, value, 0)

        timestamp = w.finish_cache_creation()
        w.close()

        r = axoncache.Reader()
        r.update("bench_test_cache", ".", timestamp)

        for key, expected_val in zip(keys, values):
            val = r.get_key(key)
            assert val == expected_val

        with self.assertRaises(axoncache.NotFoundError):
            r.get_key(b"foobarbaz")

        r.close()


if __name__ == "__main__":
    unittest.main()
