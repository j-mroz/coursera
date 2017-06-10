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

package graph

import "math"

type adjHashList map[int][]int

// Connect adds dst edge list to src vertex.
func (adj adjHashList) Connect(src int, dst ...int) {
	if _, found := adj[src]; !found {
		adj[src] = []int{}
	}

	adj[src] = append(adj[src], dst...)
}

type adjList [][]int

// Connect adds dst edge list to src vertex.
func (adj *adjList) Connect(src int, dst ...int) {
	if !(src < len(*adj)) {
		tail := make([][]int, src-len(*adj)+1)
		*adj = append(*adj, tail...)
	}

	(*adj)[src] = append((*adj)[src], dst...)
}

// Graph is top level abstraction for the graph.
type Graph struct {
	adj       adjList
	minVertex int
	maxVertex int
}

// New creates a Graph struct
func New() *Graph {
	return &Graph{
		adj:       make(adjList, 0),
		minVertex: math.MaxInt64,
		maxVertex: 0,
	}
}

// Connect adds src vertex to adj list and its outgoing edges.
func (g *Graph) Connect(src int, dst ...int) {
	if src <= g.minVertex {
		g.minVertex = src
	}
	if src >= g.maxVertex {
		g.maxVertex = src
	}
	g.adj.Connect(src, dst...)
}
