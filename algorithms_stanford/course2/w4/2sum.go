package main

import (
	"fmt"
	"io"
	"log"
	"sort"
)

func main() {
	// Read and sort the input numbers.
	numbers := make([]int, 0, 1000000)
	for {
		var num int
		_, err := fmt.Scanf("%d\n", &num)
		if err == io.EOF {
			break
		} else if err != nil {
			log.Fatal("Wrong input")
		}
		numbers = append(numbers, num)
	}
	sort.Ints(numbers)

	// Gather all possible target sums where |x + y| <- [0, 10000], x != y
	targets := make(map[int]int)
	for _, x := range numbers {
		lo := sort.SearchInts(numbers, -10000-x)
		hi := sort.SearchInts(numbers, 10000-x)
		for _, y := range numbers[lo:hi] {
			if x != y {
				targets[x+y] = 1
			}
		}
	}
	fmt.Println(len(targets))
}
