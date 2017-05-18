# Uses python3
import sys

def binary_search(a, x):
    left, length = 0, len(a)
    while length > 0:
        half = length // 2
        middle = left + half
        if a[middle] < x:
            left = middle + 1
            length -= half + 1
        else:
            length = half
    if left < len(a) and a[left] == x:
        return left
    return -1

if __name__ == '__main__':
    input = sys.stdin.read()
    data = list(map(int, input.split()))
    n = data[0]
    m = data[n + 1]
    a = data[1 : n + 1]
    for x in data[n + 2:]:
        print(binary_search(a, x), end = ' ')
