#Uses python3

import sys

def min_dot_product(a, b):
    result = 0
    a.sort()
    b.sort()
    for av, bv in zip(a, reversed(b)):
        result += av * bv
    return result

if __name__ == '__main__':
    input = sys.stdin.read()
    data = list(map(int, input.split()))
    n = data[0]
    a = data[1:(n + 1)]
    b = data[(n + 1):]
    print(min_dot_product(a, b))
