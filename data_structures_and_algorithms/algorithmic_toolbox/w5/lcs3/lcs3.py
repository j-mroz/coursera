#Uses python3

import sys

def lcs3(a, b, c):
    M = [[[-1 for _ in range(len(c))] for _ in range(len(b))] for _ in range(len(a))]
    return lcs3_impl(a, b, c, len(a), len(b), len(c), M)

def lcs3_impl(a, b, c, ai, bi, ci, M):
    if ai == 0 or bi == 0 or ci == 0:
        return 0
    elif M[ai-1][bi-1][ci-1] != -1:
        return M[ai-1][bi-1][ci-1]
    elif a[ai - 1] == b[bi - 1] == c[ci - 1]:
        M[ai-1][bi-1][ci-1] = 1 + lcs3_impl(a, b, c, ai-1, bi-1, ci-1, M)
        return M[ai-1][bi-1][ci-1]
    else:
        M[ai-1][bi-1][ci-1] = max(
                lcs3_impl(a, b, c, ai-1, bi, ci, M),
                lcs3_impl(a, b, c, ai, bi-1, ci, M),
                lcs3_impl(a, b, c, ai, bi, ci-1, M))
        return M[ai-1][bi-1][ci-1]


if __name__ == '__main__':
    input = sys.stdin.read()
    data = list(map(int, input.split()))
    an = data[0]
    data = data[1:]
    a = data[:an]
    data = data[an:]
    bn = data[0]
    data = data[1:]
    b = data[:bn]
    data = data[bn:]
    cn = data[0]
    data = data[1:]
    c = data[:cn]
    print(lcs3(a, b, c))
