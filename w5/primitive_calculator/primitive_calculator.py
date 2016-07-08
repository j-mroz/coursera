# Uses python3
import sys

def optimal_sequence(n):
    sequence = []
    while n >= 1:
        sequence.append(n)
        if n % 3 == 0:
            n = n // 3
        elif n % 2 == 0:
            n = n // 2
        else:
            n = n - 1
    return reversed(sequence)

def dynamic_sequence(n):
    Ops = [i-1 for i in range(0, n+1)]
    Prev = [i-1 for i in range(0, n+1)]

    for i in range(1, n+1):
        if Ops[i - 1] + 1 < Ops[i]:
            Ops[i] = Ops[i - 1] + 1
            Prev[i] = i - 1
        if i % 2 == 0 and Ops[i // 2] + 1 < Ops[i]:
            Ops[i] = Ops[i // 2] + 1
            Prev[i] = i // 2
        if i % 3 == 0 and Ops[i // 3] + 1 < Ops[i]:
            Ops[i] = Ops[i // 3] + 1
            Prev[i] = i // 3

    sequence = [n]
    p = Prev[n]
    while p >= 1:
        sequence.append(p)
        p = Prev[p]

    return reversed(sequence)

input = sys.stdin.read()
n = int(input)
sequence = list(dynamic_sequence(n))
print(len(sequence) - 1)
for x in sequence:
    print(x, end=' ')
