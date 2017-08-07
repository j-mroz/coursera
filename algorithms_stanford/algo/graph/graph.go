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

import (
	"math"
)

const infinity = math.MaxInt64

// Graph is top level abstraction for the graph.
type Graph struct {
	adjList      AdjList
	minVertex    int
	maxVertex    int
	vertices     map[int]bool
	edgesWeights map[int]int
	edgeCount    uint
}

// WeightedEdgesView is an interface fo accessing edges list in some graph algorithms.
type WeightedEdgesView interface {
	GetWeightedEdges() []WeightedEdge
	GetMinVertex() int
	GetMaxVertex() int
}

// New creates a Graph struct
func New() *Graph {
	return &Graph{
		adjList:      make(AdjList, 0, 32),
		minVertex:    math.MaxInt64,
		maxVertex:    0,
		vertices:     make(map[int]bool),
		edgesWeights: make(map[int]int),
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
	g.vertices[src] = true
	for _, vertex := range dst {
		g.vertices[vertex] = true
	}
	g.adjList.Connect(src, dst...)
}

// ConnectWeighted adds (src, dst, weight) pairs to adj lsit.
func (g *Graph) ConnectWeighted(src int, dstWeightPairs ...int) {
	for i := 0; i < len(dstWeightPairs); i += 2 {
		dst, weight := dstWeightPairs[i], dstWeightPairs[i+1]

		g.Connect(src, dst)

		vertexID := len(g.edgesWeights)
		g.adjList[src][len(g.adjList[src])-1].ID = vertexID
		g.edgesWeights[vertexID] = weight
	}
}

// VertexCount returns number of vertices.
func (g *Graph) VertexCount() int {
	return len(g.adjList)
}

// Contains returns true if vertex exists in graph.
func (g *Graph) Contains(vertex int) bool {
	_, contains := g.vertices[vertex]
	return contains
}

func (g *Graph) GetMinVertex() int {
	return g.minVertex
}

func (g *Graph) GetMaxVertex() int {
	return g.maxVertex
}

func (g *Graph) GetWeightedEdges() (ret []WeightedEdge) {
	// Count all directed edges.
	uniqueEdges := make(map[WeightedEdge]int)
	for _, edges := range g.adjList {
		for _, edge := range edges {
			if edge.Dst < edge.Src {
				edge.Src, edge.Dst = edge.Dst, edge.Src
			}
			wEdge := WeightedEdge{edge.Src, edge.Dst, g.edgesWeights[edge.ID]}
			uniqueEdges[wEdge]++
		}
	}

	// Add unique, undirected edges to list. Possible duplicates.
	for wEdge, count := range uniqueEdges {
		for i := 0; i < count/2; i++ {
			ret = append(ret, wEdge)
		}
	}

	return
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
		dstEdges[i] = Edge{Src: src, Dst: dst}
	}
	(*adj)[src] = append((*adj)[src], dstEdges...)
}
