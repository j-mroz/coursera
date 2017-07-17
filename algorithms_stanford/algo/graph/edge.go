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

import "math/rand"

// Edge consists of Src and Dst vertex indentifiers.
type Edge struct {
	Src, Dst, ID int
}

// EdgeList is just list of edges, defined for convenience.
type EdgeList []Edge

// Copy inserts new edges into a list from src.
func (edges *EdgeList) Copy(src EdgeList) {
	edges.Push(src...)
}

// Push inserts new edges into a list
func (edges *EdgeList) Push(values ...Edge) {
	*edges = append(*edges, values...)
}

// Pop removes last/top element from list anr returns it.
func (edges *EdgeList) Pop() (last Edge) {
	lastIdx := len(*edges) - 1
	last = (*edges)[lastIdx]
	*edges = (*edges)[:lastIdx]
	return
}

// Remove delets edges from the list based on provided predicate function.
func (edges *EdgeList) Remove(pred func(Edge) bool) {
	filtered := (*edges)[:0]
	for _, edge := range *edges {
		if !pred(edge) {
			filtered = append(filtered, edge)
		}
	}
	*edges = filtered
}

func (edges *EdgeList) shuffle(seed int64) {
	randGen := rand.New(rand.NewSource(seed))
	perm := randGen.Perm(len(*edges))

	for i := range perm {
		(*edges)[i], (*edges)[perm[i]] = (*edges)[perm[i]], (*edges)[i]
	}
}
