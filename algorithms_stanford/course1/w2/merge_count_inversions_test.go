package main

import (
	"testing"
)

type TestData struct {
	arr        []int
	inversions int
}

var dataset = []TestData{
	{[]int{1, 2, 3}, 0},
	{[]int{1, 1, 1}, 0},
	{[]int{1, 2, 1}, 1},
	{[]int{1, 2, 0}, 2},
	{[]int{3, 2, 1}, 3},
}

func TestMergeCountInversions(t *testing.T) {
	for _, ds := range dataset {
		inversions := mergeCountInversions(ds.arr)
		if ds.inversions != inversions {
			t.Errorf("Wrong number of inversions, expected %d, got %d for %v",
				ds.inversions, inversions, ds.arr)
		}
	}
}
