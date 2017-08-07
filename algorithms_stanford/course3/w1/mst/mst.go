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

	"../../../algo/graph"
)

func loadGraph(file *os.File) (g *graph.Graph) {
	g = graph.New()

	vertexCount, edgeCount := 0, 0
	fmt.Fscanf(file, "%d %d\n", &vertexCount, &edgeCount)

	for edgeIdx := 0; edgeIdx < edgeCount; edgeIdx++ {
		src, dst, weight := -1, -1, 0
		fmt.Fscanf(file, "%d %d %d\n", &src, &dst, &weight)
		g.ConnectWeighted(src, dst, weight)
		g.ConnectWeighted(dst, src, weight)
	}

	return
}

func main() {
	g := loadGraph(os.Stdin)

	// Compute spanning three starting from vertex 1.
	// Save the weights of edges from MST.
	srcVertex := 1
	mst := g.PrimMinimumSpanningTree(srcVertex)

	// Print the cumulated weights of MST.
	distSum := 0
	for _, dist := range mst.DistanceMap {
		distSum += dist
	}
	fmt.Println(distSum)
}
