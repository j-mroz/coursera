# Uses python3
import sys


def gen_fib(n):
    F = [f for f in range(n)]

    i = 1
    f_prev, f_curr = 0, 1
    while i < n-1:
        f_curr, f_prev = f_curr + f_prev, f_curr
        i += 1
        F[i] =  f_curr

    return F


def calc_cycle():
    D = [f%10 for f in gen_fib(200)]

    for c in range(2, 101):
        is_cycle = True;
        for i in range(100):
            if D[i] != D[i+c]:
                is_cycle = False
                break;
        if is_cycle:
            return c
    return -1

def get_fibonacci_last_digit(n):
    # write your code here
    return 0

if __name__ == '__main__':
    input = sys.stdin.read()
    n = int(input)
    F = gen_fib(60);
    print(F[n % 60]%10)
