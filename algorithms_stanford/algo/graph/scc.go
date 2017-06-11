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

// GetSccs is Tarjan algorithm of computign SCC:
// https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm
func (g *Graph) GetSccs() [][]int {
	scc := sccContext{
		graph:     g,
		vertices:  make([]sccVertex, len(g.adj)),
		vistCount: 0,
	}
	for vertex := range g.adj {
		scc.visit(vertex)
	}

	return scc.groups
}

type sccVertex struct {
	index    int  // Preorder index, assined when vertex is visisted by dfs.
	lowIndex int  // Lowest index reachable from vertex.
	onStack  bool // True if vetrex is currently on visit stack.
}

type sccContext struct {
	graph     *Graph      // Pointer to original graph.
	vertices  []sccVertex // Data assosiated with vertices.
	vistCount int         // Number of nodes visited by DFS.
	stack     []int       // Vertices visit stack.
	groups    [][]int     // Strongly connected components groups.
}

func (scc *sccContext) visit(src int) {
	if scc.visited(src) {
		return
	}

	scc.beginVisit(src)

	// Visit all connected vertices and update the lowest reachable vertex.
	for _, dst := range scc.graph.adj[src] {
		if !scc.visited(dst) {
			// If the destination vertex was not visited do it now and check
			// what is the lowest vetrext it was able to reach.
			scc.visit(dst)
			scc.maybeUpdateLowIndex(src, scc.vertices[dst].lowIndex)
		} else if scc.onVisitStack(dst) {
			// If the destination vertex was already visited and is on
			// the visit stack we have a strongly connected component.
			scc.maybeUpdateLowIndex(src, scc.vertices[dst].index)
		}
	}

	scc.endVisit(src)
}

func (scc *sccContext) beginVisit(vertex int) {
	scc.vistCount++
	scc.vertices[vertex].index = scc.vistCount
	scc.vertices[vertex].lowIndex = scc.vistCount
	scc.pushVisitStack(vertex)
}

func (scc *sccContext) endVisit(vertex int) {
	// At the begining every node low index was set to same value as index.
	// If it has changed to other value it means that this nodes is not the
	// scc group root.
	if scc.vertices[vertex].index != scc.vertices[vertex].lowIndex {
		return
	}

	// Because vertex is root of scc group, collect all vertices from
	// visit stack as they represent the strongly connected component.
	var sccGroup []int
	for {
		groupMember := scc.popVisitStack()
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

func (scc *sccContext) pushVisitStack(vertex int) {
	scc.stack = append(scc.stack, vertex)
	scc.vertices[vertex].onStack = true
}

func (scc *sccContext) popVisitStack() int {
	end := len(scc.stack)
	top := scc.stack[end-1]
	scc.stack = scc.stack[:end-1]
	scc.vertices[top].onStack = false
	return top
}

func (scc *sccContext) onVisitStack(vertex int) bool {
	return scc.vertices[vertex].onStack
}

func (scc *sccContext) maybeUpdateLowIndex(vertex, index int) {
	if index < scc.vertices[vertex].lowIndex {
		scc.vertices[vertex].lowIndex = index
	}
}
