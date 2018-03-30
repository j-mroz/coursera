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
	"math"
	"os"

	"../../../algo/graph"
)

func loadGraph(file *os.File) (g *graph.Graph) {
	g = graph.New()

	vertexCount := 0
	fmt.Fscanf(file, "%d %d\n", &vertexCount)

	for {
		src, dst, weight := -1, -1, 0
		_, err := fmt.Fscanf(file, "%d %d %d\n", &src, &dst, &weight)
		if err == io.EOF {
			break
		} else if err != nil {
			fmt.Fprintln(os.Stderr, "Failed to read input grap")
			panic(err)
		}

		g.ConnectWeighted(src, dst, weight)
		g.ConnectWeighted(dst, src, weight)
	}

	return
}

func main() {
	g := loadGraph(os.Stdin)
	c := g.GetMaxSpacingClusters(4)

	minDist := math.MaxInt64
	for _, edge := range c.Edges {
		if c.VerticesGroups.Find(edge.Src) != c.VerticesGroups.Find(edge.Dst) {
			if edge.Weight < minDist {
				minDist = edge.Weight
			}
		}
	}

	fmt.Println(minDist)
}
