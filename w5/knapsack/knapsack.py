# Uses python3
import sys

def optimal_weight(W, weights):
    #knapsacks[items][knapsak_weight]
    knapsacks = [[0] * (W+1) for _ in range(len(weights)+1)]
    
    for items in range(1, len(weights) + 1):
        for knapsak_weight in range(0, W+1):
            if weights[items-1] > knapsak_weight:
                knapsacks[items][knapsak_weight] = knapsacks[items-1][knapsak_weight]
            else:
                item = items - 1

                knapsacks[items][knapsak_weight] = max(
                    knapsacks[items-1][knapsak_weight],
                    knapsacks[items-1][knapsak_weight - weights[item]] + weights[item])
    return knapsacks[len(weights)][W]
    # return knapsacks

if __name__ == '__main__':
    input = sys.stdin.read()
    W, n, *w = list(map(int, input.split()))
    print(optimal_weight(W, w))
