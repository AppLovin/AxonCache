// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

package core

type ExtraWriter interface {
	Write(p []byte) (int, error)
	GetTotalRows() int
}

type DevNullWriter struct {
}

func (d *DevNullWriter) Write(p []byte) (int, error) {
	n := len(p)
	return n, nil
}

func (d *DevNullWriter) GetTotalRows() int {
	return 0
}
