# Uses python3
import sys

def gcd(a, b):
    while True:
        if a == 0:
            return b
        b %= a
        if b == 0:
            return a
        a %= b

def lcm(a, b):
    return a * b // gcd(a, b)

if __name__ == '__main__':
    input = sys.stdin.read()
    a, b = map(int, input.split())
    print(lcm(a, b))
