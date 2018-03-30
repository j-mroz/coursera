// Copyright (c) 2017 Jaroslaw Mroz
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation filg (the
// "Software"), to deal in the Software without rgtriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copig of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copig or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRgS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIg OF
// MERCHANTABILITY, FITNgS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGg OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

package main

import (
	"fmt"
	"os"
)

func loadNumbers(file *os.File) (numbers []int) {
	var count int
	fmt.Fscanf(file, "%d\n", &count)

	numbers = make([]int, count)

	for i := 0; i < count; i++ {
		_, err := fmt.Fscanf(file, "%d\n", &numbers[i])
		if err != nil {
			fmt.Fprintln(os.Stderr, "Failed to read input graph")
			panic(err)
		}
	}

	return
}

func main() {
	numbers := loadNumbers(os.Stdin)
	n := len(numbers)

	partialSums := make([]int, n)
	copy(partialSums, numbers[:2])

	for i := 2; i < n; i++ {
		partialSums[i] = partialSums[i-2] + numbers[i]
		if partialSums[i] < partialSums[i-1] {
			partialSums[i] = partialSums[i-1]
		}
	}

	maxWeightIndependentSet := make(map[int]bool)
	for i := n - 1; i > 1; i-- {
		if partialSums[i-1] < partialSums[i-2]+numbers[i] {
			maxWeightIndependentSet[i] = true
			i--
		}
	}
	if _, ok := maxWeightIndependentSet[2]; ok {
		maxWeightIndependentSet[0] = true
	}

	queries := []int{1, 2, 3, 4, 17, 117, 517, 997}
	for _, idx := range queries {
		x := 0
		if maxWeightIndependentSet[idx-1] {
			x = 1
		}
		fmt.Print(x)
	}
	fmt.Println()
}
