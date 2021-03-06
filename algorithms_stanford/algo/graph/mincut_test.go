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
	"testing"
)

func TestEdgeListPush(t *testing.T) {
	lst := EdgeList{}
	lst.Push(Edge{Src: 0, Dst: 1}, Edge{Src: 1, Dst: 2}, Edge{Src: 2, Dst: 0})

	if len(lst) != 3 {
		t.Error("Failed to add edges")
	}
}

func TestEdgeListRemove(t *testing.T) {
	lst := EdgeList{}
	lst.Push(Edge{Src: 0, Dst: 0}, Edge{Src: 0, Dst: 1}, Edge{Src: 1, Dst: 2}, Edge{Src: 2, Dst: 0})

	lst.Remove(func(e Edge) bool {
		return e.Src == e.Dst
	})

	if len(lst) != 3 {
		t.Error("Failed to remove edge")
	}
}

func TestCollectUndirectedEdgges(t *testing.T) {
	g := New()
	g.Connect(0, 1, 2, 3)
	g.Connect(1, 0)
	g.Connect(2, 0)
	g.Connect(3, 0)

	edges := g.collectUndirectedEdges()
	if len(edges) != 3 {
		t.Error("Failed to collect edges")
	}
}

func TestMinCut(t *testing.T) {
	g := New()
	g.Connect(0, 1, 2, 3)
	g.Connect(1, 0)
	g.Connect(2, 0)
	g.Connect(3, 0)

	g.MinCut()
}
