// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

package main

import (
	"context"
	"sync"
	"sync/atomic"
	"time"

	"github.com/dustin/go-humanize"

	log "github.com/sirupsen/logrus"
)

// WriteCounter counts the number of bytes written to it. It implements to the io.Writer interface
// and we can pass this into io.TeeReader() which will report progress on each write cycle.
type WriteCounter struct {
	Label         string
	Total         uint64
	ContentLength int64
	Name          string
	Start         time.Time
	LastTotal     uint64
}

func (wc *WriteCounter) Write(p []byte) (int, error) {
	n := len(p)
	atomic.AddUint64(&wc.Total, uint64(n))
	return n, nil
}

func handleWriterProgress(ctx context.Context, wc *WriteCounter) {
	go func() {
		for range time.Tick(time.Second) {
			select {
			case <-ctx.Done():
				return
			default:
				total := atomic.LoadUint64(&wc.Total)
				lastTotal := atomic.LoadUint64(&wc.LastTotal)

				log.Debugf("%s %q: %s/%s, ⬇️  %v/s %s",
					wc.Label,
					wc.Name,
					humanize.Bytes(total),
					humanize.Bytes(uint64(wc.ContentLength)),
					humanize.Bytes(uint64(total-lastTotal)),
					time.Since(wc.Start),
				)

				atomic.StoreUint64(&wc.LastTotal, total)
			}
		}
	}()
}

type InsertCounter struct {
	Label     string
	Start     time.Time
	Total     int64
	LastTotal int64
	Mutex     sync.Mutex
}

func (ic *InsertCounter) Incr() {
	atomic.AddInt64(&ic.Total, 1)
}

func (ic *InsertCounter) SetLabel(label string) {
	ic.Mutex.Lock()
	ic.Label = label
	ic.Mutex.Unlock()
}

func handleInsertProgress(ctx context.Context, ic *InsertCounter) {
	go func() {
		for range time.Tick(time.Second) {
			select {
			case <-ctx.Done():
				return
			default:
				total := atomic.LoadInt64(&ic.Total)
				lastTotal := atomic.LoadInt64(&ic.LastTotal)

				ic.Mutex.Lock()
				label := ic.Label
				ic.Mutex.Unlock()

				log.Debugf("%s : total keys processed %s 🚴 %v/s %s",
					label,
					humanize.Comma(total),
					humanize.Comma(total-lastTotal),
					time.Since(ic.Start),
				)

				atomic.StoreInt64(&ic.LastTotal, total)
			}
		}
	}()
}
