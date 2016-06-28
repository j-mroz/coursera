# Uses python3
import sys

def optimal_summands(n):
    summands = []

    sumand, remaining = 0, n
    while remaining > 0 and sumand + 1 < remaining - sumand - 1:
        sumand += 1
        remaining -= sumand
        summands.append(sumand)
    summands.append(remaining)

    return summands

if __name__ == '__main__':
    input = sys.stdin.read()
    n = int(input)
    summands = optimal_summands(n)
    print(len(summands))
    for x in summands:
        print(x, end=' ')
