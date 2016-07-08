# Uses python3
def edit_distance(s, t):
    dist = [[0] * (len(s) + 1) for _ in range(len(t) + 1)]

    for i in range(0, len(s) + 1):
        dist[0][i] = i
    for i in range(0, len(t) + 1):
        dist[i][0] = i

    for i in range(1, len(t) + 1):
        for j in range(1, len(s) + 1):
            if s[j - 1] == t[i - 1]:
                dist[i][j] = min(
                    dist[i - 1][j - 1],
                    dist[i - 1][j] + 1,
                    dist[i][j - 1] + 1)
            else:
                dist[i][j] = min(
                    dist[i - 1][j - 1] + 1,
                    dist[i - 1][j] + 1,
                    dist[i][j - 1] + 1)
    return dist[len(t)][len(s)]


if __name__ == "__main__":
    print(edit_distance(input(), input()))
