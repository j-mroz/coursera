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
	"os"
	"path/filepath"
	"strings"
	"testing"
)

type Registry map[string]string

var inputs, outputs Registry

func TestMain(m *testing.M) {
	loadTestCases()

	// Tun tests.
	ret := m.Run()
	os.Exit(ret)
}

func loadTestCases() {
	inputs = make(Registry)
	outputs = make(Registry)

	// Setup test cases by reading input and output test files.
	files, _ := filepath.Glob("testcases/*_random_*")
	for _, fpath := range files {
		fname := strings.Replace(fpath, "testcases/", "", 1)
		tokens := strings.Split(fname, "_random_")

		ftype := tokens[0]
		testid := strings.Replace(tokens[1], ".txt", "", 1)

		switch ftype {
		case "input":
			inputs[testid] = fpath
		case "output":
			outputs[testid] = fpath
		}
	}
}

func TestAll(t *testing.T) {
	for testId, inPath := range inputs {
		outPath, found := outputs[testId]
		if found {
			testMinCut(inPath, outPath, t)
		}
	}
}

func testMinCut(inPath, outPath string, t *testing.T) {
	file, err := os.Open(inPath)
	if err != nil {
		t.Error(err)
	}
	defer file.Close()

	g, err := loadGraph(file)
	if err != nil {
		t.Error(err)
	}

	expectedMinCut, err := loadExpectedOutput(outPath)
	if err != nil {
		t.Error(err)
	}

	minCutEdges := g.MinCut()
	minCut := minCutEdges.Len()

	if minCut != expectedMinCut {
		t.Errorf("%s: MinCut: got %d, expected %d.\n", inPath, minCut, expectedMinCut)
	}
}

func loadExpectedOutput(fpath string) (expected int, err error) {
	fd, err := os.Open(fpath)
	if err != nil {
		return 0, err
	}
	defer fd.Close()

	// Read output.
	fmt.Fscanf(fd, "%d", &expected)

	return expected, nil
}
