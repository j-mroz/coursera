# Uses python3
def calc_fib(n):
    if n == 0:
        return 0

    i = 1
    f_prev, f_curr = 0, 1
    while i < n:
        f_curr, f_prev = f_curr + f_prev, f_curr
        i += 1

    return f_curr

n = int(input())
print(calc_fib(n))
