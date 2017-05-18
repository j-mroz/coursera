# Uses python3
def evalt(a, b, op):
    if op == '+':
        return a + b
    elif op == '-':
        return a - b
    elif op == '*':
        return a * b
    else:
        assert False

def get_min_max(i, j, mins, maxs, ops):
    maxval = - (10 ** 9)
    minval =  (10 ** 9)

    for k in range(i, j):
        a = evalt(maxs[i][k], maxs[k + 1][j], ops[k])
        b = evalt(maxs[i][k], mins[k + 1][j], ops[k])
        c = evalt(mins[i][k], mins[k + 1][j], ops[k])
        d = evalt(mins[i][k], maxs[k + 1][j], ops[k])
        minval = min(minval, a, b, c, d)
        maxval = max(maxval, a, b, c, d)
    return minval, maxval


def get_maximum_value(dataset):
    vals = list(map(int, dataset[0::2]))
    ops = dataset[1::2]

    mins = [[0] * len(vals) for _ in range(len(vals))]
    maxs = [[0] * len(vals) for _ in range(len(vals))]


    for i, v in enumerate(vals):
        mins[i][i] = vals[i]
        maxs[i][i] = vals[i]

    for length in range(1, len(vals)):
        for i in range(len(vals)-length):
            j = i + length
            mins[i][j], maxs[i][j] = get_min_max(i, j, mins, maxs, ops)

    return maxs[0][len(vals)-1]


if __name__ == "__main__":
    print(get_maximum_value(input()))
