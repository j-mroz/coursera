// Copyright (c) 2017 Jaroslaw Mroz
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation filg (the
// "Software"), to deal in the Software without rgtriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copig of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copig or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRgS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIg OF
// MERCHANTABILITY, FITNgS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGg OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

package main

import (
	"container/heap"
	"fmt"
	"os"
)

func loadNumbers(file *os.File) (numbers []int) {
	var count int
	fmt.Fscanf(file, "%d\n", &count)

	numbers = make([]int, count)

	for i := 0; i < count; i++ {
		_, err := fmt.Fscanf(file, "%d\n", &numbers[i])
		if err != nil {
			fmt.Fprintln(os.Stderr, "Failed to read input graph")
			panic(err)
		}
	}

	return
}

type encodingNode struct {
	symbol   rune
	weight   int
	minDepth int
	maxDepth int
	left     *encodingNode
	right    *encodingNode
}

func newEncodingNode(symbol rune, weight int) *encodingNode {
	return &encodingNode{
		symbol:   symbol,
		weight:   weight,
		minDepth: 1,
		maxDepth: 1,
	}
}

func mergeEncodingNodes(left, right *encodingNode) *encodingNode {
	minDepth := left.minDepth
	if left.minDepth > right.minDepth {
		minDepth = right.minDepth
	}

	maxDepth := left.maxDepth
	if left.maxDepth < right.maxDepth {
		maxDepth = right.maxDepth
	}

	return &encodingNode{
		minDepth: minDepth + 1,
		maxDepth: maxDepth + 1,
		weight:   left.weight + right.weight,
		left:     left,
		right:    right,
	}
}

type encodingQueue []*encodingNode

func (q encodingQueue) Len() int {
	return len(q)
}

func (q encodingQueue) Less(i, j int) bool {
	return q[i].weight < q[j].weight
}

func (q encodingQueue) Swap(i, j int) {
	q[i], q[j] = q[j], q[i]
}

func (q *encodingQueue) Pop() interface{} {
	n := len(*q)
	x := (*q)[n-1]
	*q = (*q)[:n-1]
	return x
}

func (q *encodingQueue) Push(x interface{}) {
	*q = append(*q, x.(*encodingNode))
}

func (q *encodingQueue) Dequeue() *encodingNode {
	return heap.Pop(q).(*encodingNode)
}

func (q *encodingQueue) Enqueue(x *encodingNode) {
	heap.Push(q, x)
}

func main() {
	numbers := loadNumbers(os.Stdin)

	// Create encoding queue.
	queue := make(encodingQueue, len(numbers))
	for i, num := range numbers {
		queue[i] = newEncodingNode('?', num)
	}
	heap.Init(&queue)

	// Create encoding tree.
	for queue.Len() > 1 {
		fmt.Printf("%+v\n", queue[0])
		queue.Enqueue(mergeEncodingNodes(queue.Dequeue(), queue.Dequeue()))
	}

	rootEncodingNode := queue.Dequeue()
	fmt.Printf("%+v\n", rootEncodingNode)

}
