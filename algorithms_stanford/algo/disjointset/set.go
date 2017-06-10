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

package disjointset

// Set represent FindUnion disjoint set.
// Usefull for quickly uniting two sets  or checking to which set value belongs.
type Set struct {
	id      []int
	weights []int
	count   int
	begin   int
	end     int
}

// New creates new Set for vales in range <begin, end>
func New(begin, end int) *Set {
	s := new(Set)
	s.begin = begin
	s.end = end
	size := end - begin + 1
	s.id = make([]int, size)
	s.weights = make([]int, size)
	s.count = size

	for i := range s.id {
		s.id[i] = i
	}

	return s
}

// Count returns number of unique items in set.
func (s *Set) Count() int {
	return s.count
}

// Find returns the set identifier that idx belong to.
func (s *Set) Find(idx int) int {
	return s.find(idx-s.begin) + s.begin
}

func (s *Set) find(idx int) int {
	// Find root element of idx.
	root := idx
	for parent := s.id[root]; root != parent; parent = s.id[root] {
		root = parent
	}

	// Compress path.
	for element := idx; element != root; {
		parent := s.id[element]
		s.id[element] = root
		element = parent
	}

	return root
}

// Unite merges two sets that contains a and b.
func (s *Set) Unite(a, b int) {
	s.unite(a-s.begin, b-s.begin)
}

func (s *Set) unite(a, b int) {
	a, b = s.find(a), s.find(b)
	if a != b {
		if s.weights[a] > s.weights[b] {
			// b will point to a
			s.id[b] = s.id[a]
			s.count--
		} else {
			// a will point to b
			s.id[a] = s.id[b]
			s.count--
			if s.weights[a] == s.weights[b] {
				s.weights[b]++
			}
		}
	}
}

// AreUnion checks if two values belong to same set.
func (s *Set) AreUnion(a, b int) bool {
	return s.Find(a) == s.Find(b)
}
