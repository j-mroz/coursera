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

// GetScc is Tarjan algorithm of computign SCC:
// https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm
func (g *Graph) GetSccs() [][]int {
	scc := sccContext{
		graph:    g,
		vertices: make([]sccVertex, len(g.adj)),
		index:    0,
	}
	for vertex := range g.adj {
		scc.visit(vertex)
	}

	return scc.groups
}

type sccVertex struct {
	index    int
	lowIndex int
	onStack  bool
}

type sccContext struct {
	graph    *Graph
	vertices []sccVertex
	index    int
	stack    []int
	groups   [][]int
}

func (scc *sccContext) visit(src int) {
	if scc.visited(src) {
		return
	}

	scc.beginVisit(src)
	for _, dst := range scc.graph.adj[src] {
		if !scc.visited(dst) {
			scc.visit(dst)
			scc.vertices[src].lowIndex = min(scc.vertices[src].lowIndex,
				scc.vertices[dst].lowIndex)
		} else if scc.onVisitStack(dst) {
			scc.vertices[src].lowIndex = min(scc.vertices[src].lowIndex,
				scc.vertices[dst].index)
		}
	}
	scc.endVisit(src)
}

func (scc *sccContext) beginVisit(vertex int) {
	scc.index++
	scc.vertices[vertex].index = scc.index
	scc.vertices[vertex].lowIndex = scc.index
	scc.push(vertex)
}

func (scc *sccContext) endVisit(vertex int) {
	// Check if is scc group root.
	if scc.vertices[vertex].index != scc.vertices[vertex].lowIndex {
		return
	}

	var sccGroup []int
	for {
		groupMember := scc.pop()
		sccGroup = append(sccGroup, groupMember)
		if vertex == groupMember {
			break
		}
	}
	scc.groups = append(scc.groups, sccGroup)
}

func (scc *sccContext) visited(vertex int) bool {
	return scc.vertices[vertex].index != 0
}

func (scc *sccContext) push(vertex int) {
	scc.stack = append(scc.stack, vertex)
	scc.vertices[vertex].onStack = true
}

func (scc *sccContext) pop() int {
	end := len(scc.stack)
	top := scc.stack[end-1]
	scc.stack = scc.stack[:end-1]
	scc.vertices[top].onStack = false
	return top
}

func (scc *sccContext) onVisitStack(vertex int) bool {
	return scc.vertices[vertex].onStack
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}
