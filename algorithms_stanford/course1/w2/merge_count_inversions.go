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
	"fmt"
	"io"
	"os"
)

func mergeCountInversions(arr []int) int {
	buf := make([]int, len(arr))
	return mergeCountInversionsImpl(arr, buf)
}

func mergeCountInversionsImpl(arr []int, buf []int) int {
	mid := len(arr) / 2
	leftArr, rightArr := arr[:mid], arr[mid:]
	leftBuf, rightBuf := buf[:mid], buf[mid:]

	// Split to smaller problems.
	inversions := 0
	if len(leftArr) > 1 {
		inversions += mergeCountInversionsImpl(leftArr, leftBuf)
	}
	if len(rightArr) > 1 {
		inversions += mergeCountInversionsImpl(rightArr, rightBuf)
	}

	// Merge slices and count inversions.
	lIdx, rIdx, bIdx := 0, 0, 0
	lEnd, rEnd, bufEnd := len(leftArr), len(rightArr), len(buf)
	for bIdx < bufEnd {
		if (rIdx == rEnd) || (lIdx < lEnd && leftArr[lIdx] <= rightArr[rIdx]) {
			// Either there is no more elements in right array, either leftArray
			// contains smaller element.
			buf[bIdx] = leftArr[lIdx]
			bIdx++
			lIdx++
		} else {
			// In this case right array contains smaller element.
			buf[bIdx] = rightArr[rIdx]
			bIdx++
			rIdx++
			inversions += lEnd - lIdx
		}
	}

	// Copy from buffer to original array.
	copy(arr, buf)

	return inversions
}

func main() {

	// Read input numbers.
	arr := make([]int, 0, 100000)
	for {
		var number int

		_, err := fmt.Scanf("%d\n", &number)
		if err == io.EOF {
			break
		}
		if err != nil {
			fmt.Println(err)
			os.Exit(-1)
		}
		arr = append(arr, number)
	}

	// Count the number of inversion needed to get sorted array.
	inversions := mergeCountInversions(arr)
	fmt.Println(inversions)
}
