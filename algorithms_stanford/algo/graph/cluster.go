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
	"sort"

	"../disjointset"
)

type WeightedEdge struct {
	Src, Dst, Weight int
}

type byEdgeWeight []WeightedEdge

// Len implements sort.Interface
func (x byEdgeWeight) Len() int {
	return len(x)
}

// Swap implements sort.Interface
func (x byEdgeWeight) Swap(i, j int) {
	x[i], x[j] = x[j], x[i]
}

// Less implements sort.Interface
func (x byEdgeWeight) Less(i, j int) bool {
	return x[i].Weight < x[j].Weight
}

// Clusters describes clusters subgraphs in the graph.
type Clusters struct {
	VerticesGroups *disjointset.Set
	Edges          []WeightedEdge
}

// GetMaxSpacingClusters - compute clusters using Kruskal minimum spanning tree
// algorithm with k limit.
func (g *Graph) GetMaxSpacingClusters(k int) (spanningTrees []SpanningTree) {
	return GetMaxSpacingClusters(g, k)
}

// GetMaxSpacingClusters - compute clusters using Kruskal minimum spanning tree
// algorithm with k limit.
func GetMaxSpacingClusters(wev WeightedEdgesView, k int) (spanningTrees []SpanningTree) {
	verticesGroups := disjointset.New(wev.GetMinVertex(), wev.GetMaxVertex())
	edges := wev.GetWeightedEdges()
	sort.Sort(byEdgeWeight(edges))

	for _, edge := range edges {
		if verticesGroups.Count() <= k {
			break
		}
		verticesGroups.Unite(edge.Src, edge.Dst)
	}

	return
}
