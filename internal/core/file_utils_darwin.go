// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

//go:build darwin

package core

import "syscall"

func MMapOptions() int {
	return syscall.MAP_PRIVATE
}

func (m *MMapInfo) Prefetch(fd int) error {
	return nil
}

func Fallocate(fd int, size int64) error {
	return nil
}

func Evict(filePath string) (err error) {
	return
}
