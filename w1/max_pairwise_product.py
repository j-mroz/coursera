# Uses python3
n = int(input())
a = [int(x) for x in input().split()]
assert(len(a) == n)

result = 0
max1, idx1 = 0, 0
max2, idx2 = 0, 0
for i in range(0, n):
    if a[i] >= max1:
        max2, idx2 = max1, idx1
        max1, idx1 = a[i], i
    elif a[i] >= max2:
        max2, idx2 = a[i], i
result = max1 * max2
print(result)
