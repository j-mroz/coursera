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

package main

import (
	"bufio"
	"fmt"
	"io"
	"log"
	"os"
	"strconv"
	"strings"

	"../../algo/graph"
)

func loadGraph(file *os.File) (g *graph.Graph, err error) {
	g = graph.New()

	reader := bufio.NewReader(file)
	for {
		line, err := reader.ReadString('\n')
		if err == io.EOF {
			break
		}
		if err != nil {
			return g, err
		}

		tokens := strings.FieldsFunc(line, func(r rune) bool {
			return r == ',' || r == '\t'
		})
		if tokens[len(tokens)-1] == "\r\n" || tokens[len(tokens)-1] == "\n" {
			tokens = tokens[:len(tokens)-1]
		}

		var vertices []int
		for _, token := range tokens {
			vertex, err := strconv.Atoi(token)
			if err != nil {
				return g, err
			}
			vertices = append(vertices, vertex)
		}

		g.ConnectWeighted(vertices[0], vertices[1:]...)
	}

	return g, nil
}

func main() {
	g, err := loadGraph(os.Stdin)
	if err != nil {
		log.Fatal(err)
		os.Exit(1)
	}

	srcVertex := 1
	dist := g.DijkstraShortestPath(srcVertex)

	dstVertices := []int{7, 37, 59, 82, 99, 115, 133, 165, 188, 197}
	for _, dst := range dstVertices {
		fmt.Print(dist[dst], ",")
	}
	fmt.Println()
}
