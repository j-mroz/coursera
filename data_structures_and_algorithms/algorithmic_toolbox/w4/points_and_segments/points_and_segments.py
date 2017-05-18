# Uses python3
import sys

def fast_count_segments(starts, ends, points):
    cnt = [0] * len(points)

    SEGMENT_START = 1
    POINT = 2
    SEGMENT_END = 3

    line = []
    line += [(s, SEGMENT_START, i) for (i, s) in enumerate(starts)]
    line += [(e, SEGMENT_END, i) for (i, e) in enumerate(ends)]
    line += [(p, POINT, i) for (i, p) in enumerate(points)]
    line.sort()

    counter = 0
    for sstart, stype, sidx in line:
        if stype == SEGMENT_START:
            counter += 1
        elif stype == SEGMENT_END:
            counter -= 1
        else:
            cnt[sidx] = counter

    return cnt

def naive_count_segments(starts, ends, points):
    cnt = [0] * len(points)
    for i in range(len(points)):
        for j in range(len(starts)):
            if starts[j] <= points[i] <= ends[j]:
                cnt[i] += 1
    return cnt

if __name__ == '__main__':
    input = sys.stdin.read()
    data = list(map(int, input.split()))
    n = data[0]
    m = data[1]
    starts = data[2:2 * n + 2:2]
    ends   = data[3:2 * n + 2:2]
    points = data[2 * n + 2:]
    cnt = fast_count_segments(starts, ends, points)
    for x in cnt:
        print(x, end=' ')
