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

func TestVertexHeap(t *testing.T) {
	h := newVertexHeap()
	h.PushVertex(105, 5)
	h.PushVertex(108, 8)
	h.PushVertex(103, 3)
	h.PushVertex(101, 1)
	h.PushVertex(104, 4)
	h.PushVertex(107, 7)
	h.PushVertex(102, 2)
	h.PushVertex(109, 9)
	h.PushVertex(106, 6)

	if h.Len() != 9 {
		t.Error("heap size is too small, expected 9, got", h.Len())
	}
	expected := 101
	for h.Len() > 0 {
		top := h.PopVertex()
		if top != expected {
			t.Error("expected", expected, "got", top)
		}
		expected++
	}
}
