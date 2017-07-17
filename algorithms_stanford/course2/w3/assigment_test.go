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
	"log"
	"os"
	"testing"

	"../../utils"
)

var testInputs, testOutputs utils.TestCasesFilesMap

func TestMain(m *testing.M) {
	testInputs, testOutputs = utils.LoadTestCases("_random_")

	// Tun tests.
	ret := m.Run()
	os.Exit(ret)
}

func TestAll(t *testing.T) {
	for testID, inPath := range testInputs {
		if outPath, ok := testOutputs[testID]; ok {
			testMedianHeap(inPath, outPath, t)
		}
	}
}

func testMedianHeap(inPath, outPath string, t *testing.T) {
	numbers := loadInputNumbers(inPath)
	expected := loadExpectedSumOfMedians(outPath)

	sumOfMedianMod10k := 0
	mheap := NewMedianHeap()
	for _, num := range numbers {
		mheap.Push(num)
		sumOfMedianMod10k += mheap.Median() % 10000
		sumOfMedianMod10k %= 10000
	}

	if sumOfMedianMod10k != expected {
		t.Errorf("%s: median is %d, expected %d.\n", inPath, sumOfMedianMod10k, expected)
	}
}

func loadInputNumbers(inPath string) []int {
	file, err := os.Open(inPath)
	if err != nil {
		log.Fatal("loadInputNumbers error:", err)
	}
	defer file.Close()

	var numbers []int
	for {
		var num int
		_, err := fmt.Fscanf(file, "%d\n", &num)
		if err == io.EOF {
			break
		} else if err != nil {
			log.Fatal("loadInputNumbers error:", err)
		}
		numbers = append(numbers, num)
	}
	return numbers
}

func loadExpectedSumOfMedians(fpath string) int {
	fd, err := os.Open(fpath)
	if err != nil {
		log.Fatal("loadExpectedOutput error:", err)
	}
	defer fd.Close()

	// Read output.
	var median int
	fmt.Fscanf(fd, "%d", &median)
	return median
}
