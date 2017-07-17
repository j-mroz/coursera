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

package qsort

import (
	"sort"
)

// PartitionStrategy determines how the pivot is choosen.
type PartitionStrategy func(arr sort.Interface, start, end int) (pivot int)

func partitionByFirst(arr sort.Interface, start, end int) int {
	return start
}

func partitionByLast(arr sort.Interface, start, end int) int {
	return end - 1
}

func partitionByMedian(arr sort.Interface, start, end int) int {
	median := [3]int{start, start + (end-start-1)/2, end - 1}

	if arr.Less(median[1], median[0]) {
		median[0], median[1] = median[1], median[0]
	}
	if arr.Less(median[2], median[1]) {
		median[2], median[1] = median[1], median[2]

		if arr.Less(median[1], median[0]) {
			median[0], median[1] = median[1], median[0]
		}
	}

	return median[1]
}

// Sort is quick sort implementation.
func Sort(arr sort.Interface) {
	quickSortImpl(arr, 0, arr.Len(), partitionByMedian)
}

// CountSwapsByFirst sorts arr by partitioning arround first element.
func CountSwapsByFirst(arr sort.Interface) (swaps int) {
	swaps = quickSortImpl(arr, 0, arr.Len(), partitionByFirst)
	return
}

// CountSwapsByLast sorts arr by partitioning arround last element.
func CountSwapsByLast(arr sort.Interface) (swaps int) {
	swaps = quickSortImpl(arr, 0, arr.Len(), partitionByLast)
	return
}

// CountSwapsByMedian sorts arr by partitioning arround median of three elements.
func CountSwapsByMedian(arr sort.Interface) (swaps int) {
	swaps = quickSortImpl(arr, 0, arr.Len(), partitionByMedian)
	return
}

func quickSortImpl(arr sort.Interface, start, end int, getPartionPivot PartitionStrategy) (swaps int) {
	if !(start < end) {
		return
	}

	pivot := getPartionPivot(arr, start, end)
	pivot = partitionLeftToRight(arr, start, pivot, end)
	// pivot = partitionTwoSide(arr, start, pivot, end)

	swaps = end - start - 1
	swaps += quickSortImpl(arr, start, pivot, getPartionPivot)
	swaps += quickSortImpl(arr, pivot+1, end, getPartionPivot)

	return
}

func partitionTwoSide(arr sort.Interface, start, pivot, end int) int {
	if !(start < end) {
		return start
	}
	arr.Swap(start, pivot)
	pivot = start

	leftEnd, rightStart := start+1, end-1
	for {
		for leftEnd <= rightStart && !arr.Less(pivot, leftEnd) {
			leftEnd++
		}
		for leftEnd < rightStart && arr.Less(pivot, rightStart) {
			rightStart--
		}
		if leftEnd >= rightStart {
			break
		}
		arr.Swap(leftEnd, rightStart)
	}
	arr.Swap(pivot, leftEnd-1)

	return leftEnd - 1
}

func partitionLeftToRight(arr sort.Interface, start, pivot, end int) int {
	if !(start < end) {
		return start
	}
	arr.Swap(start, pivot)
	pivot = start

	leftEnd := start + 1
	for rightStart := leftEnd; rightStart < end; rightStart++ {
		if arr.Less(rightStart, pivot) {
			arr.Swap(rightStart, leftEnd)
			leftEnd++
		}
	}
	arr.Swap(pivot, leftEnd-1)

	return leftEnd - 1
}
