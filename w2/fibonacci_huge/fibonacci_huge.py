# Uses python3
import sys

def fib(n):
    if n == 0:
        return 0

    f_prev, f_curr = 0, 1
    for i in range(1, n):
        f_curr, f_prev = f_curr + f_prev, f_curr

    return f_curr

def calc_cycle(n, m):
    a, b = 1, 1

    for i in range(3, m**2):
        a, b = b, (a + b) % m
        if a == 1 and b == 0:
            return i
    return -1

def get_fibonaccihuge(n, m):
    c = calc_cycle(n, m)
    return fib(n % c) % m


if __name__ == '__main__':
    input = sys.stdin.read();
    n, m = map(int, input.split())
    print(get_fibonaccihuge(n, m))
