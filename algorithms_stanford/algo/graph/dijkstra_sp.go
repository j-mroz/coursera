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
	"container/heap"
	"math"
)

const infinity = math.MaxUint64

type vertexHeapElement struct {
	vertex int
	weight uint
}

type vertexHeap struct {
	elements []vertexHeapElement
	indices  map[int]int
}

func newVertexHeap(cap int) *vertexHeap {
	vertexHeap := &vertexHeap{
		indices: make(map[int]int),
	}
	heap.Init(vertexHeap)
	return vertexHeap
}

func (h vertexHeap) Len() int {
	return len(h.elements)
}

func (h vertexHeap) Less(i, j int) bool {
	return h.elements[i].weight < h.elements[j].weight
}

func (h *vertexHeap) Swap(i, j int) {
	h.elements[i], h.elements[j] = h.elements[j], h.elements[i]
	h.indices[h.elements[i].vertex], h.indices[h.elements[j].vertex] = i, j
}

func (h *vertexHeap) Push(x interface{}) {
	element := x.(vertexHeapElement)
	h.indices[element.vertex] = h.Len()
	h.elements = append(h.elements, element)
}

func (h *vertexHeap) Pop() interface{} {
	n := h.Len()
	x := h.elements[n-1]
	h.elements = h.elements[:n-1]
	delete(h.indices, x.vertex)
	return x
}

func (h *vertexHeap) PushVertex(vertex int, weight uint) {
	heap.Push(h, vertexHeapElement{vertex, weight})
}

func (h *vertexHeap) PopVertex() int {
	return heap.Pop(h).(vertexHeapElement).vertex
}

func (h *vertexHeap) MaybeUpdateVertex(vertex int, weight uint) {
	index, ok := h.indices[vertex]
	if !ok {
		return
	}
	h.elements[index].weight = weight
	heap.Fix(h, index)
}

// DijkstraShortestPath calculates shortest path from source vertex to all vertices.
func (g *Graph) DijkstraShortestPath(sourceVertex int) []uint {
	// Initialize distances from sourceVertex
	dist := make([]uint, g.VertexCount())
	prev := make([]int, g.VertexCount())
	for vertex := range dist {
		dist[vertex] = infinity
	}
	dist[sourceVertex] = 0

	// Add all vertices to the unvisited heap/set.
	heap := newVertexHeap(g.VertexCount())

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
			if dist[edge.Src]+uint(weight) < dist[edge.Dst] {
				dist[edge.Dst] = dist[edge.Src] + uint(weight)
				heap.MaybeUpdateVertex(edge.Dst, dist[edge.Dst])
				prev[edge.Dst] = edge.Src
			}
		}
	}

	return dist
}
