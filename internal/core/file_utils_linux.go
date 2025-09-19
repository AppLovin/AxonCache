// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

//go:build linux

package core

import (
	"os"
	"syscall"

	"golang.org/x/sys/unix"
)

func MMapOptions() int {
	return syscall.MAP_SHARED | syscall.MAP_POPULATE
}

func (m *MMapInfo) Prefetch(fd int) error {
	offset := int64(0)
	len_of_range := int64(len(m.ByteRange))

	FADV_WILLNEED := 3

	err := unix.Fadvise(fd, offset, len_of_range, FADV_WILLNEED)
	return err
}

func Fallocate(fd int, size int64) error {
	err := syscall.Fallocate(fd, 0, 0, size)
	return err
}

func Evict(filePath string) (err error) {
	fd, err := os.Open(filePath)
	if err != nil {
		return
	}
	defer fd.Close()
	stat, err := fd.Stat()
	if err != nil {
		return
	}
	if err = unix.Fadvise(int(fd.Fd()), 0, stat.Size(), unix.FADV_DONTNEED); err != nil {
		return
	}
	return
}
