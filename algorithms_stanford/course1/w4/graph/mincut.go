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
	"math/rand"
	"sync"
	"time"

	"../disjointset"
)

// Edge consists of Src and Dst vertex indentifiers.
type Edge struct {
	Src, Dst int
}

// EdgeList is just list of edges, defined for convenience.
type EdgeList struct {
	edges []Edge
}

// Copy inserts new edges into a list from src.
func (l *EdgeList) Copy(src EdgeList) {
	l.Push(src.edges...)
}

// Push inserts new edges into a list
func (l *EdgeList) Push(edges ...Edge) {
	l.edges = append(l.edges, edges...)
}

// Pop removes last/top element from list anr returns it.
func (l *EdgeList) Pop() (last Edge) {
	last, l.edges = l.edges[l.Len()-1], l.edges[:l.Len()-1]
	return
}

// Remove delets edges from the list based on provided predicate function.
func (l *EdgeList) Remove(pred func(Edge) bool) {
	filtered := l.edges[:0]
	for _, edge := range l.edges {
		if !pred(edge) {
			filtered = append(filtered, edge)
		}
	}
	l.edges = filtered
}

// Len returns lenght of connection list
func (l *EdgeList) Len(edges ...Edge) int {
	return len(l.edges)
}

func (l *EdgeList) shuffle(seed int64) {
	randGen := rand.New(rand.NewSource(seed))
	perm := randGen.Perm(l.Len())

	for i := range perm {
		l.edges[i], l.edges[perm[i]] = l.edges[perm[i]], l.edges[i]
	}
}

// AdjHashList implement concept of adjacency list, but uses map as a
// first level of lookup.
type AdjHashList struct {
	list map[int][]int
}

// Connect adds dst edge list to src vertex.
func (adj *AdjHashList) Connect(src int, dst ...int) {
	// Lazy initialization
	if adj.list == nil {
		adj.list = make(map[int][]int)
	}
	if _, found := adj.list[src]; !found {
		adj.list[src] = []int{}
	}

	adj.list[src] = append(adj.list[src], dst...)
}

// Graph is top level abstraction for the graph.
type Graph struct {
	adj AdjHashList
}

// Connect adds src vertex to adj list and its outgoing edges.
func (g *Graph) Connect(src int, dst ...int) {
	g.adj.Connect(src, dst...)
}

func (g *Graph) collectUndirectedEdges() (edges EdgeList) {
	uniqueEdges := make(map[Edge]int)

	for src, srcConnections := range g.adj.list {
		for _, dst := range srcConnections {
			edge := Edge{src, dst}
			if dst < src {
				edge.Src, edge.Dst = dst, src
			}
			uniqueEdges[edge]++
		}
	}

	for edge, count := range uniqueEdges {
		for i := 0; i < count/2; i++ {
			edges.Push(edge)
		}
	}

	return
}

// MinCut is parallel  implementation of Karager minimal graph cut algorithm.
// See: https://en.wikipedia.org/wiki/Karger%27s_algorithm
func (g *Graph) MinCut() (ret EdgeList) {
	const parallelJobs = 10
	const trials = 500

	rand.Seed(time.Now().UTC().UnixNano())

	bestCut, bestCutSize := EdgeList{}, math.MaxUint32

	edges := g.collectUndirectedEdges()

	// Get the minimum and maximum vertex number.
	minVertex, maxVertex := getMinMaxVertex(g)

	// Do the trials in parallel.
	for i := 0; i < trials/parallelJobs; i++ {
		var cuts [parallelJobs]EdgeList
		var wg sync.WaitGroup

		// Run jubs in parallel then wait for wall jobs to finish.
		wg.Add(parallelJobs)
		asyncMinCut := func(idx int, seed int64) {
			defer wg.Done()
			cuts[idx] = minCut(edges, minVertex, maxVertex, seed)
		}
		for i := 0; i < len(cuts); i++ {
			go asyncMinCut(i, int64(rand.Int()))
		}
		wg.Wait()

		// Find the bestCut ,if any, among those returned by parallel jobs.
		for i := 0; i < len(cuts); i++ {
			if cuts[i].Len() < bestCutSize {
				bestCut = cuts[i]
				bestCutSize = cuts[i].Len()
			}
		}
	}

	return bestCut
}

type asyncMinCut func(idx int, seed int64)

func minCut(edges EdgeList, minVertex, maxVertex int, seed int64) (cutEdges EdgeList) {
	// Copy and shuffle edges for randomnes.
	cutEdges.Copy(edges)
	cutEdges.shuffle(seed)

	// Allocate FindUnion disjoint set. It will represent our graph of graphs.
	hyperGraph := disjointset.New(minVertex, maxVertex)

	// See: https://en.wikipedia.org/wiki/Karger%27s_algorithm
	for hyperGraph.Count() > 2 {
		edge := cutEdges.Pop()
		src := hyperGraph.Find(edge.Src)
		dst := hyperGraph.Find(edge.Dst)
		hyperGraph.Unite(src, dst)
	}

	// Filter out edges that belong to the same connected component -> have
	// common parent in FindUnion structure.
	cutEdges.Remove(func(e Edge) bool {
		return hyperGraph.AreUnion(e.Src, e.Dst)
	})

	return cutEdges
}

func getMinMaxVertex(g *Graph) (minVertexNum, maxVertexNum int) {
	minVertexNum = math.MaxInt64

	for src := range g.adj.list {
		if src > maxVertexNum {
			maxVertexNum = src
		}
		if src < minVertexNum {
			minVertexNum = src
		}
	}

	return minVertexNum, maxVertexNum
}
