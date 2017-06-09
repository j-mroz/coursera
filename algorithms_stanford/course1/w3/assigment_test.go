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
	"path/filepath"
	"sort"
	"strings"
	"testing"

	"./qsort"
)

type Registry map[string]string

var inputs, outputs Registry

func TestMain(m *testing.M) {
	inputs = make(Registry)
	outputs = make(Registry)

	// Setup test cases by reading input and output test files.
	files, _ := filepath.Glob("testcases/*_dgrcode_*")
	for _, fpath := range files {
		fname := strings.Replace(fpath, "testcases/", "", 1)
		tokens := strings.Split(fname, "_dgrcode_")

		ftype := tokens[0]
		testid := strings.Replace(tokens[1], ".txt", "", 1)

		switch ftype {
		case "input":
			inputs[testid] = fpath
		case "output":
			outputs[testid] = fpath
		}
	}

	// Tun tests.
	ret := m.Run()
	os.Exit(ret)
}

func TestAll(t *testing.T) {
	for id, inPath := range inputs {
		outPath, found := outputs[id]
		if !found {
			continue
		}
		testCountSwaps(inPath, outPath, t)
	}
}

type CountSwapsResults struct {
	byFirst  int
	byLast   int
	byMedian int
}

type CountSwapsTest struct {
	input         []int
	arr           []int
	actualSwaps   CountSwapsResults
	expectedSwaps CountSwapsResults
}

func (test *CountSwapsTest) readInput(fpath string) (err error) {
	test.input = test.input[:0]
	fd, err := os.Open(fpath)
	if err != nil {
		return
	}
	defer fd.Close()

	// Read input.
	for {
		var number int

		_, err = fmt.Fscanf(fd, "%d\n", &number)
		if err == io.EOF {
			err = nil
			break
		}
		if err != nil {
			fmt.Println(err)
			break
		}
		test.input = append(test.input, number)
	}

	return
}

func (test *CountSwapsTest) readOutput(fpath string) (err error) {
	fd, err := os.Open(fpath)
	if err != nil {
		return err
	}
	defer fd.Close()

	// Read output.
	fmt.Fscanf(fd, "%d\n", &test.expectedSwaps.byFirst)
	fmt.Fscanf(fd, "%d\n", &test.expectedSwaps.byLast)
	fmt.Fscanf(fd, "%d\n", &test.expectedSwaps.byMedian)

	return
}

func (test *CountSwapsTest) setup() {
	test.arr = test.arr[:0]
	test.arr = append(test.arr, test.input...)
}

//v Resue this variable to avoid arr allocations.
var test CountSwapsTest

func testCountSwaps(inPath, outPath string, t *testing.T) {
	if test.readInput(inPath) != nil || test.readOutput(outPath) != nil {
		t.Error("Files", inPath, outPath, "failed to be read.")
		return
	}

	test.setup()
	test.actualSwaps.byFirst = qsort.CountSwapsByFirst(sort.IntSlice(test.arr))

	test.setup()
	test.actualSwaps.byLast = qsort.CountSwapsByLast(sort.IntSlice(test.arr))

	test.setup()
	test.actualSwaps.byMedian = qsort.CountSwapsByMedian(sort.IntSlice(test.arr))

	// Utilizes struct compare feature.
	if test.actualSwaps != test.expectedSwaps {
		t.Errorf("%s:\t swaps mismatch, actual:%+v, expected:%+v", inPath, test.actualSwaps, test.expectedSwaps)
	}
}
