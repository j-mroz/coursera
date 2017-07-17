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
	"bufio"
	"os"
	"strconv"
	"strings"
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
			testDijkstraShortestPath(inPath, outPath, t)
		}
	}
}

func loadExpectedPathsLength(file *os.File) (dist []uint, err error) {
	reader := bufio.NewReader(file)
	line, _ := reader.ReadString('\n')
	tokens := strings.FieldsFunc(line, func(r rune) bool {
		return r == ',' || r == '\n'
	})
	for _, token := range tokens {
		num, err := strconv.Atoi(token)
		if err != nil {
			break
		}
		dist = append(dist, uint(num))
	}
	return
}

func testDijkstraShortestPath(inPath, outPath string, t *testing.T) {
	inFile, err := os.Open(inPath)
	if err != nil {
		t.Error(err)
	}
	g, err := loadGraph(inFile)
	if err != nil {
		t.Error(err)
	}

	outFile, err := os.Open(outPath)
	if err != nil {
		t.Error(err)
	}
	expectedDist, err := loadExpectedPathsLength(outFile)
	if err != nil {
		t.Error(err)
	}

	srcVertex := 1
	dist := g.DijkstraShortestPath(srcVertex)

	dstVertices := []int{7, 37, 59, 82, 99, 115, 133, 165, 188, 197}
	for i, dst := range dstVertices {
		if dist[dst] != expectedDist[i] {
			t.Error(inPath, outPath, dist[dst], "!=", expectedDist[i])
		}
	}

}
