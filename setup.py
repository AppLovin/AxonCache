# SPDX-License-Identifier: MIT
# Copyright (c) 2025 AppLovin. All rights reserved.

# Available at setup time due to pyproject.toml
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup
import sys

__version__ = "0.0.1"

# The main interface is through Pybind11Extension.
# * You can add cxx_std=11/14/17, and then build_ext can be removed.
# * You can set include_pybind11=false to add the include directory yourself,
#   say from a submodule.
#
# Note:
#   Sort input source files if you glob sources to ensure bit-for-bit
#   reproducible builds (https://github.com/pybind/python_example/pull/53)

extra_cflags = ["-std=c++20"]
extra_ldflags = []
if sys.platform == "darwin":
    extra_cflags += ["-mmacosx-version-min=10.15"]
    extra_ldflags += ["-mmacosx-version-min=10.15"]

ext_modules = [
    Pybind11Extension(
        "axoncache",
        ["axoncache/axoncache.cpp"],
        include_dirs=["include", ".."],
        extra_compile_args=extra_cflags,
        extra_link_args=extra_ldflags,
        cxx_std=20,
    ),
]

setup(
    name="axoncache",
    version=__version__,
    author="Benjamin Sergeant",
    url="https://github.com/AppLovin/AxonCache",
    description="A test project using pybind11",
    long_description="",
    ext_modules=ext_modules,
    extras_require={"test": "pytest"},
    # Currently, build_ext only provides an optional "highest supported C++
    # level" feature, but in the future it may provide more features.
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.9",
)
