# Uses python3
import sys
import random

# right is inclusive here ...
def partition3(a, left, right):
    length = right - left + 1
    if not length > 1:
        return

    pivot = a[left]

    while left < right and a[left] < pivot:
        left += 1
    while left < right and pivot < a[right]:
        right -= 1

    # a[lower] >= pivot, a[upper] > pivot after a loop
    pos, lower, upper = left, left, left
    while pos <= right:
        if a[pos] == pivot:
            a[upper], a[pos] = a[pos], a[upper]
            upper += 1
        elif a[pos] < pivot:
            if upper != pos:
                a[lower], a[upper], a[pos] = a[pos], a[lower], a[upper]
            else:
                a[lower], a[pos] = a[pos], a[lower]
            lower += 1
            upper += 1
        pos += 1

    return lower, upper-1

def partition3v2(a, left, right):
    length = right - left + 1
    if not length > 1:
        return left, right

    pivot = a[left]
    pos, lower, upper = left, left, right
    while pos <= upper:
        if a[pos] < pivot:
            a[lower], a[pos] = a[pos], a[lower]
            pos, lower = pos + 1, lower + 1
        elif a[pos] == pivot:
            pos += 1
        else: #a[pos] > pivot:
            a[upper], a[pos] = a[pos], a[upper]
            upper -= 1

    return lower, upper


def randomized_quick_sort(a, l, r):
    if l >= r:
        return
    k = random.randint(l, r)
    a[l], a[k] = a[k], a[l]
    b, e = partition3v2(a, l, r)
    randomized_quick_sort(a, l, b - 1);
    randomized_quick_sort(a, e+1, r);


if __name__ == '__main__':
    input = sys.stdin.read()
    n, *a = list(map(int, input.split()))
    randomized_quick_sort(a, 0, n - 1)
    for x in a:
        print(x, end=' ')
    #
    # for test in range(10):
    #     a = [random.randint(0, 5) for _ in range(10)]
    #     print (a, "->",)
    #     randomized_quick_sort(a, 0, len(a)-1)
    #     print (a, all(a <= b for a,b in zip(a[:-1], a[1:])))
