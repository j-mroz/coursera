# Uses python3
import sys

def get_change(n):
    result = 0
    result += n // 10
    n %= 10
    result += n // 5
    n %= 5
    result += n
    return result

if __name__ == '__main__':
    n = int(sys.stdin.read())
    print(get_change(n))
