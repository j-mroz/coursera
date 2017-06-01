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
	"sort"

	"./qsort"
)

func main() {
	var input, arr []int

	// Read input.
	input = make([]int, 0, 100000)
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
		input = append(input, number)
	}

	// Sort and count the inversions sing differen strategies.
	arr = make([]int, len(input))

	copy(arr, input)
	swapsByFirst := qsort.CountSwapsByFirst(sort.IntSlice(arr))
	fmt.Println("swaps when partitioning by first: ", swapsByFirst)

	copy(arr, input)
	swapsByLast := qsort.CountSwapsByLast(sort.IntSlice(arr))
	fmt.Println("swaps when partitioning by last: ", swapsByLast)

	copy(arr, input)
	swapsByMedian := qsort.CountSwapsByMedian(sort.IntSlice(arr))
	fmt.Println("swaps when partitioning by median: ", swapsByMedian)

}
