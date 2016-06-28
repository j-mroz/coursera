# Uses python3
import sys
from collections import namedtuple

Segment = namedtuple('Segment', 'start end')

def optimal_points(segments):
    points = []
    segments = sorted(segments, key=lambda s: (s[1], s[0]))

    seg_pos, seg_end = 0, len(segments)
    while seg_pos < seg_end:
        start, end = segments[seg_pos]
        seg_next = seg_pos + 1
        while seg_next < seg_end and segments[seg_next][0] <= end:
            start = max(start, segments[seg_next][0])
            seg_next += 1
            seg_pos += 1
        points.append(end)
        seg_pos += 1

    return points

if __name__ == '__main__':
    input = sys.stdin.read()
    n, *data = map(int, input.split())
    segments = list(map(lambda x: Segment(x[0], x[1]), zip(data[::2], data[1::2])))
    points = optimal_points(segments)
    print(len(points))
    for p in points:
        print(p, end=' ')
