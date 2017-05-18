# Uses python3
import sys

def get_number_of_inversions(a, left, right):
    buf = a[:]
    return get_number_of_inversions_impl(a, left, right, buf)

def get_number_of_inversions_impl(a, left, right, buf):
    number_of_inversions = 0
    length = right - left
    if length <= 1:
        return number_of_inversions

    half = length // 2
    mid = left + half
    number_of_inversions += get_number_of_inversions_impl(a, left, mid, buf)
    number_of_inversions += get_number_of_inversions_impl(a, mid, right, buf)

    lpos, llen = left, mid - left
    rpos, rlen = mid, right - mid
    bi = 0
    # buff = []
    while llen + rlen > 0:
        while lpos < mid and (rlen == 0 or a[lpos] <= a[rpos]):
            buf[bi], bi = a[lpos], bi + 1
            # buff.append(a[lpos])
            lpos, llen = lpos + 1, llen - 1
        while rpos < right and (llen == 0 or a[rpos] < a[lpos]):
            buf[bi], bi = a[rpos], bi + 1
            # buff.append(a[rpos])
            rpos, rlen = rpos + 1, rlen - 1
            number_of_inversions += llen

    a[left:right] = buf[:length]

    return number_of_inversions


if __name__ == '__main__':
    input = sys.stdin.read()
    n, *a = list(map(int, input.split()))
    print(get_number_of_inversions(a, 0, n))
