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

// Graph is top level abstraction for the graph.
type Graph struct {
	adjList      AdjList
	minVertex    int
	maxVertex    int
	edgesWeights map[Edge]int
}

// New creates a Graph struct
func New() *Graph {
	return &Graph{
		adjList:      make(AdjList, 0, 32),
		minVertex:    math.MaxInt64,
		maxVertex:    0,
		edgesWeights: make(map[Edge]int),
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
	g.adjList.Connect(src, dst...)
}

// ConnectWeighted adds (src, dst, weight) pairs to adj lsit.
func (g *Graph) ConnectWeighted(src int, dstWeightPairs ...int) {
	for i := 0; i < len(dstWeightPairs); i += 2 {
		dst, weight := dstWeightPairs[i], dstWeightPairs[i+1]
		g.Connect(src, dstWeightPairs[i])
		g.edgesWeights[Edge{src, dst}] = weight
	}
}

// VertexCount returns number of vertices.
func (g *Graph) VertexCount() int {
	return len(g.adjList)
}

// AdjList implement adjacency list representation of a graph.
type AdjList []EdgeList

// Connect adds dst edge list to src vertex.
func (adj *AdjList) Connect(src int, dstList ...int) {
	if !(src < len(*adj)) {
		tail := make([]EdgeList, src-len(*adj)+1)
		*adj = append(*adj, tail...)
	}
	dstEdges := make(EdgeList, len(dstList))
	for i, dst := range dstList {
		dstEdges[i] = Edge{src, dst}
	}
	(*adj)[src] = append((*adj)[src], dstEdges...)
}
