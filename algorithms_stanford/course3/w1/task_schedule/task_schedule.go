package main

import (
	"fmt"
	"io"
	"log"
	"sort"
)

// Task struct gathers params related to task.
type task struct {
	weight int
	length int
}

// Helper functions to calculate params used for task ordering.
func (t task) diff() int      { return t.weight - t.length }
func (t task) ratio() float64 { return float64(t.weight) / float64(t.length) }

// For orderding by decreasing difference between weight and length.
type byDiffDesc []task

// Len implements sort.Interface function.
func (t byDiffDesc) Len() int {
	return len(t)
}

// Swap implements sort.Interface function.
func (t byDiffDesc) Swap(i, j int) {
	t[i], t[j] = t[j], t[i]
}

// Less implements sort.Interface function.
func (t byDiffDesc) Less(i, j int) bool {
	return t[i].diff() > t[j].diff() ||
		(t[i].diff() == t[j].diff() && t[i].weight > t[j].weight)
}

// For orderding by decreasing ratio between weight and length.
type byRatioDesc []task

// Len implements sort.Interface function.
func (t byRatioDesc) Len() int {
	return len(t)
}

// Swap implements sort.Interface function.
func (t byRatioDesc) Swap(i, j int) {
	t[i], t[j] = t[j], t[i]
}

// Less implements sort.Interface function.
func (t byRatioDesc) Less(i, j int) bool {
	return t[i].ratio() > t[j].ratio() ||
		(t[i].ratio() == t[j].ratio() && t[i].weight > t[j].weight)
}

func main() {
	// Read task from stdin.
	taskCount := 0
	fmt.Scanf("%d\n", &taskCount)
	tasks := make([]task, taskCount)
	for i := range tasks {
		_, err := fmt.Scanf("%d %d\n", &tasks[i].weight, &tasks[i].length)
		if err == io.EOF {
			break
		} else if err != nil {
			log.Fatal("Wrong input")
		}
	}

	sumWeightedCompletionTimes := func(tasks []task) int {
		completionTime := 0
		sum := 0
		for _, task := range tasks {
			completionTime += task.length
			sum += completionTime * task.weight
		}
		return sum
	}

	// Check weighted completion time
	// when sheduluing tasks by difference of weight and length.
	sort.Sort(byDiffDesc(tasks))
	fmt.Println(sumWeightedCompletionTimes(tasks))

	// Check weighted completion time
	// when sheduluing tasks by difference ratio of weight and length.
	sort.Sort(byRatioDesc(tasks))
	fmt.Println(sumWeightedCompletionTimes(tasks))
}
