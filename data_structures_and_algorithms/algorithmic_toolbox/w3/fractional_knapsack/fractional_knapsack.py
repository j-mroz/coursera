# Uses python3
import sys

def get_optimal_value(capacity, weights, values):
    result = 0.
    items = [(1.0*v/w, w, v) for (w, v) in zip(weights, values)]
    items = reversed(sorted(items))
    remaining = capacity
    for _, weight, value in items:
        if remaining <= 0:
            break;
        if weight <= remaining:
            result += value
            remaining -= weight
        else:
            result += value * (1.0 * remaining / weight)
            remaining = 0

    return result


if __name__ == "__main__":
    data = list(map(int, sys.stdin.read().split()))
    n, capacity = data[0:2]
    values = data[2:(2 * n + 2):2]
    weights = data[3:(2 * n + 2):2]
    opt_value = get_optimal_value(capacity, weights, values)
    print("{:.10f}".format(opt_value))
