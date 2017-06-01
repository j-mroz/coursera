package qsort

import (
	"fmt"
	"math/rand"
	"sort"
	"testing"
	"time"
)

func genIntArray(size, min, max int) []int {
	rand.Seed(time.Now().UTC().UnixNano())
	arr := make([]int, size)
	for i := range arr {
		val := min + rand.Intn(max-min)
		arr[i] = val
	}

	return arr
}

func testquickSortt(arr sort.Interface, t *testing.T) {
	Sort(arr)
	original := fmt.Sprint(arr)
	if !sort.IsSorted(arr) {
		t.Error("quickSort failed\nBefore:", original, "\n After:", arr)
	}
}

type PartitionFunction func(arr sort.Interface, start, pivot, end int) int

func testPartition(partition PartitionFunction, arr sort.Interface, pivot int, t *testing.T) {
	original := fmt.Sprint(arr)
	split := partition(arr, 0, pivot, arr.Len())
	if !isPartitioned(arr, split) {
		t.Error("partition failed\n Pivot:", pivot, "\n Split:", split, "\nBefore:", original, "\n After:", arr)
	}
}

func isPartitioned(arr sort.Interface, pivot int) bool {
	if arr.Len() == pivot {
		return true
	}
	for i := 0; i < pivot; i++ {
		if arr.Less(pivot, i) {
			return false
		}
	}
	for i := pivot + 1; i < arr.Len(); i++ {
		// Could use stronger condition here the one commented out, but not when
		// using partition routine as sugested by course
		// if !arr.Less(pivot, i) {
		if arr.Less(i, pivot) {
			fmt.Println(arr, i, pivot)
			return false
		}
	}
	return true
}

func TestQuickSortRandomized(t *testing.T) {
	const iterations = 10
	const maxSize = 10

	for i := 0; i < iterations; i++ {
		n := rand.Intn(maxSize)
		arr := genIntArray(n, 0, 10)
		testquickSortt(sort.IntSlice(arr), t)
	}
}

func TestPartitionTwoSideRandomized(t *testing.T) {
	const iterations = 50
	const maxSize = 10

	for i := 0; i < iterations; i++ {
		n := rand.Intn(maxSize)
		arr := genIntArray(n, 0, 10)
		pivot := 0
		testPartition(partitionTwoSide, sort.IntSlice(arr), pivot, t)
	}
}

func TestPartitionLeftToRightRandomized(t *testing.T) {
	const iterations = 50
	const maxSize = 10

	for i := 0; i < iterations; i++ {
		n := rand.Intn(maxSize)
		arr := genIntArray(n, 0, 10)
		pivot := 0
		testPartition(partitionLeftToRight, sort.IntSlice(arr), pivot, t)
	}
}

func TestMedian(t *testing.T) {
	arr := []int{8, 2, 4, 5, 7, 1}
	partitionByMedian(sort.IntSlice(arr), 0, len(arr))
}
