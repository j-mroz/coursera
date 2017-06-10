package main

import (
	"bufio"
	"fmt"
	"io"
	"log"
	"os"
	"sort"
	"strconv"
	"strings"

	"../../algo/graph"
)

func loadGraph(file *os.File) (g graph.Graph, err error) {
	g = *graph.New()

	reader := bufio.NewReader(file)
	for {
		var vertices []int

		line, err := reader.ReadString('\n')
		if err == io.EOF {
			break
		}
		if err != nil {
			return g, err
		}

		tokens := strings.Fields(line)
		for _, token := range tokens {
			vertex, err := strconv.Atoi(token)
			if err != nil {
				return g, err
			}
			vertices = append(vertices, vertex)
		}
		g.Connect(vertices[0], vertices[1:]...)
	}

	return g, nil
}

func main() {
	g, err := loadGraph(os.Stdin)
	if err != nil {
		log.Fatal(err)
		os.Exit(1)
	}

	sccGroup := g.GetSccs()
	sccGroupSizes := make([]int, len(sccGroup))
	for i, scc := range sccGroup {
		sccGroupSizes[i] = len(scc)
	}
	sort.Ints(sccGroupSizes)

	firstIdx := len(sccGroupSizes) - 5
	if firstIdx < 0 {
		firstIdx = 0
	}

	for i := len(sccGroupSizes) - 1; i >= firstIdx; i-- {
		fmt.Print(sccGroupSizes[i])
		if i > firstIdx {
			fmt.Print(",")
		}
	}
	fmt.Println()
}
