# Uses python3
import sys

def bisearch_greater_than(a, left, right, x):
    length = right - left
    while length > 0:
        half = length // 2
        middle = left + half
        if a[middle] <= x:
            left = middle + 1
            length -= half + 1
        else:
            length = half
    return left

def get_majority_element(a, left, right):
    a.sort()
    length = right - left
    while left < right:
        first_greater = bisearch_greater_than(a, left, right, a[left])
        if first_greater - left > length // 2:
            return left
        left = first_greater
    return -1

if __name__ == '__main__':
    input = sys.stdin.read()
    n, *a = list(map(int, input.split()))
    if get_majority_element(a, 0, n) != -1:
        print(1)
    else:
        print(0)
