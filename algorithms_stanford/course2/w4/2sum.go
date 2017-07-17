package main

import (
	"fmt"
	"io"
	"log"
	"sort"
)

func main() {

	// numbers := make(map[int]int)
	numbers := make([]int, 0, 1000000)
	targets := make(map[int]int)
	for {
		var num int
		_, err := fmt.Scanf("%d\n", &num)
		if err == io.EOF {
			break
		} else if err != nil {
			log.Fatal("Wrong input")
		}
		// numbers[num]++
		numbers = append(numbers, num)
	}

	sort.Ints(numbers)

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
