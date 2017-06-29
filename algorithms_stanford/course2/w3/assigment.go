// Copyright (c) 2017 Jaroslaw Mroz
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

package main

import (
	"container/heap"
	"fmt"
	"io"
	"log"
)

// MedianHeap is a type that supports gettign median in O(1) time.
type MedianHeap struct {
	low  IntMinMaxHeap
	high IntMinMaxHeap
}

// NewMedianHeap creates new MedianHeap.
func NewMedianHeap() *MedianHeap {
	return &MedianHeap{
		low:  IntMinMaxHeap{minHeap: false},
		high: IntMinMaxHeap{minHeap: true},
	}
}

// Push adds new value to the heap.
func (h *MedianHeap) Push(val int) {
	if h.low.Len() == 0 || val < h.low.elements[0] {
		heap.Push(&h.low, val)
	} else {
		heap.Push(&h.high, val)
	}
	for h.low.Len() > h.high.Len() {
		heap.Push(&h.high, heap.Pop(&h.low))
	}
	for h.low.Len() < h.high.Len() {
		heap.Push(&h.low, heap.Pop(&h.high))
	}
}

// Median returns current median.
func (h *MedianHeap) Median() int {
	return h.low.elements[0]
}

type IntMinMaxHeap struct {
	elements []int
	minHeap  bool
}

func (h IntMinMaxHeap) Len() int {
	return len(h.elements)
}

func (h IntMinMaxHeap) Less(i, j int) bool {
	if h.minHeap {
		return h.elements[i] < h.elements[j]
	}
	return h.elements[i] > h.elements[j]
}

func (h IntMinMaxHeap) Swap(i, j int) {
	h.elements[i], h.elements[j] = h.elements[j], h.elements[i]
}

func (h *IntMinMaxHeap) Push(val interface{}) {
	h.elements = append(h.elements, val.(int))
}

func (h *IntMinMaxHeap) Pop() interface{} {
	size := len(h.elements)
	top := h.elements[size-1]
	h.elements = h.elements[:size-1]
	return top
}

func main() {
	mheap := NewMedianHeap()

	var numbers []int
	for {
		var num int
		_, err := fmt.Scanf("%d\n", &num)
		if err == io.EOF {
			break
		} else if err != nil {
			log.Fatal("Wrong input")
		}
		numbers = append(numbers, num)
	}

	medianModuloSum := 0
	for _, num := range numbers {
		mheap.Push(num)
		medianModuloSum += mheap.Median() % 10000
		medianModuloSum %= 10000
	}
	fmt.Println(medianModuloSum)
}
