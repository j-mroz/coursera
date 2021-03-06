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
package utils

import (
	"path/filepath"
	"strings"
)

type TestCasesFilesMap map[string]string

// LoadTestCases loads testcases from testcases subdir.
func LoadTestCases(pattern string) (inputs, outputs TestCasesFilesMap) {
	inputs = make(TestCasesFilesMap)
	outputs = make(TestCasesFilesMap)

	// Setup test cases by reading input and output test files.
	files, _ := filepath.Glob("testcases/*" + pattern + "*")
	for _, fpath := range files {
		fname := strings.Replace(fpath, "testcases/", "", 1)
		tokens := strings.Split(fname, pattern)

		ftype := tokens[0]
		testid := strings.Replace(tokens[1], ".txt", "", 1)

		switch ftype {
		case "input":
			inputs[testid] = fpath
		case "output":
			outputs[testid] = fpath
		}
	}
	return
}
