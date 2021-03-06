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

// DijkstraShortestPath calculates shortest path from source vertex to all vertices.
func (g *Graph) DijkstraShortestPath(sourceVertex int) []int {
	// Initialize distances from sourceVertex
	dist := make([]int, g.VertexCount())
	prev := make([]int, g.VertexCount())
	for vertex := range dist {
		dist[vertex] = infinity
	}
	dist[sourceVertex] = 0

	// Add all vertices to the unvisited heap/set.
	heap := newVertexHeap()

	for vertex := range g.adjList {
		heap.PushVertex(vertex, dist[vertex])
	}

	// Pop the nearest vertex from the heap untill all are visited.
	for heap.Len() > 0 {
		vertex := heap.PopVertex()
		for _, edge := range g.adjList[vertex] {
			weight := g.edgesWeights[edge.ID]
			// if src is infinity away than no route is know to src from start
			if dist[edge.Src] == infinity {
				continue
			}
			if dist[edge.Src]+weight < dist[edge.Dst] {
				dist[edge.Dst] = dist[edge.Src] + weight
				heap.MaybeUpdateVertex(edge.Dst, dist[edge.Dst])
				prev[edge.Dst] = edge.Src
			}
		}
	}

	return dist
}
