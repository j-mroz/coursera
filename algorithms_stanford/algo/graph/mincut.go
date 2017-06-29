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

func (g *Graph) collectUndirectedEdges() (edges EdgeList) {
	uniqueEdges := make(map[Edge]int)

	for _, edges := range g.adjList {
		for _, edge := range edges {
			if edge.Dst < edge.Src {
				edge.Src, edge.Dst = edge.Dst, edge.Src
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
func (g *Graph) MinCut() (bestCut EdgeList) {
	const parallelJobs = 10
	const trials = 500

	rand.Seed(time.Now().UTC().UnixNano())

	bestCutSize := math.MaxUint32
	edges := g.collectUndirectedEdges()

	// Do the trials in parallel.
	for i := 0; i < trials/parallelJobs; i++ {
		var cuts [parallelJobs]EdgeList
		var wg sync.WaitGroup

		// Run jubs in parallel then wait for all jobs to finish.
		wg.Add(parallelJobs)
		for i := 0; i < len(cuts); i++ {
			go g.asyncMinCut(edges, int64(rand.Int()), &wg, &cuts[i])
		}
		wg.Wait()

		// Find the bestCut, if any, among those returned by parallel jobs.
		for i := 0; i < len(cuts); i++ {
			if len(cuts[i]) < bestCutSize {
				bestCut = cuts[i]
				bestCutSize = len(cuts[i])
			}
		}
	}

	return bestCut
}

func (g *Graph) asyncMinCut(e EdgeList, randSeed int64, wg *sync.WaitGroup, result *EdgeList) {
	defer wg.Done()
	*result = g.minCut(e, randSeed)
}

func (g *Graph) minCut(edges EdgeList, randSeed int64) (cutEdges EdgeList) {
	// Copy and shuffle edges for randomnes.
	cutEdges.Copy(edges)
	cutEdges.shuffle(randSeed)

	// Allocate FindUnion disjoint set. It will represent our graph of graphs.
	hyperGraph := disjointset.New(g.minVertex, g.maxVertex)

	// See: https://en.wikipedia.org/wiki/Karger%27s_algorithm
	for hyperGraph.Count() > 2 {
		edge := cutEdges.Pop()
		srcGraph := hyperGraph.Find(edge.Src)
		dstGraph := hyperGraph.Find(edge.Dst)
		hyperGraph.Unite(srcGraph, dstGraph)
	}

	// Filter out edges that belong to the same connected component -> have
	// common parent in FindUnion structure.
	cutEdges.Remove(func(e Edge) bool {
		return hyperGraph.AreUnion(e.Src, e.Dst)
	})

	return cutEdges
}
