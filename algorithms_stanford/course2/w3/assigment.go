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
	low  IntHeap // low contains median and lower elements.
	high IntHeap // high contains element higher than median.
}

// NewMedianHeap creates new MedianHeap.
func NewMedianHeap() *MedianHeap {
	return &MedianHeap{
		low:  *newIntMaxHeap(),
		high: *newIntMinHeap(),
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

// IntHeap is a heap of ints.
type IntHeap struct {
	elements []int
	cmp      func(a, b int) bool
}

func newIntMinHeap() *IntHeap {
	return &IntHeap{
		cmp: func(a, b int) bool { return a < b },
	}
}

func newIntMaxHeap() *IntHeap {
	return &IntHeap{
		cmp: func(a, b int) bool { return a > b },
	}
}

// Len is heap.Interface method.
func (h IntHeap) Len() int {
	return len(h.elements)
}

// Less is heap.Interface method.
func (h IntHeap) Less(i, j int) bool {
	return h.cmp(h.elements[i], h.elements[j])
}

// Swap is heap.Interface method.
func (h IntHeap) Swap(i, j int) {
	h.elements[i], h.elements[j] = h.elements[j], h.elements[i]
}

// Push is heap.Interface method.
func (h *IntHeap) Push(val interface{}) {
	h.elements = append(h.elements, val.(int))
}

// Pop is heap.Interface method.
func (h *IntHeap) Pop() interface{} {
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
			log.Fatal("Wrong input:", err)
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
