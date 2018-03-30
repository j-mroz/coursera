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

//SpanningTree representst spanning tree as a maps of incomming edges.
type SpanningTree struct {
	PredecessorMap map[int]int
	DistanceMap    map[int]int
}

// PrimMinimumSpanningTree calculates MST from source vertex.
func (g *Graph) PrimMinimumSpanningTree(sourceVertex int) (mst SpanningTree) {
	const infDist = math.MaxInt64

	mst.PredecessorMap = make(map[int]int)
	mst.DistanceMap = make(map[int]int)

	// Initialize distances from sourceVertex
	distArr := make([]int, g.VertexCount())
	for vertex := range distArr {
		distArr[vertex] = infDist
	}
	distArr[sourceVertex] = 0

	// Add all vertices to the unvisited heap/set.
	unvisited := newVertexHeap()
	for vertex := range g.adjList {
		unvisited.PushVertex(vertex, distArr[vertex])
	}

	// Pop the nearest vertex from the heap untill all are visited.
	for unvisited.Len() > 0 {
		vertex := unvisited.PopVertex()
		for _, edge := range g.adjList[vertex] {
			edgeWeight := g.edgesWeights[edge.ID]

			// If src is infinity away than no route is know to src from start.
			// Should not happen in connected graph, vut might happen
			// in disconnecte one.
			if distArr[edge.Src] != infDist && edgeWeight < distArr[edge.Dst] &&
				unvisited.Contains(edge.Dst) {
				distArr[edge.Dst] = edgeWeight
				unvisited.UpdateVertex(edge.Dst, edgeWeight)
				mst.PredecessorMap[edge.Dst] = edge.Src
				mst.DistanceMap[edge.Dst] = edgeWeight
			}
		}
	}

	return
}
