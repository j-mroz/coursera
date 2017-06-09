// Copyright (c) 2017 Jaroslaw Mroz
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

package main

import (
	"fmt"
	"log"
	"math"
	"math/big"
	"os"
	"unicode"
)

const (
	wordLowMask      = 0xFFFFFFFF
	wordBits         = 64
	wordLowBits      = 32
	wordHighBits     = wordBits - wordLowBits
	decimalMaxDigits = 19
	decimalBase      = 10000000000000000000
	digitBase        = 10
)

// Word is single leg used in BigInt
type Word uint64

// BigInt is only intended to be used as convenient way to store integers for
// karatsuba multiplicaton algorithm that I'm implementing here.
// big.Int from standard library is far superior to this.
type BigInt struct {
	legs     []Word
	negative bool
}

// SetString sets the BigInt from provided string.
func (bi *BigInt) SetString(arg string) {
	leg, legDigits := Word(0), 0
	bi.legs = make([]Word, 0, 4)

	if arg[0] == '-' {
		arg = arg[1:]
		bi.negative = true
	}

	if len(bi.legs) != 0 {
		bi.legs = bi.legs[:0]
	}

	// Parse digits from left to right.
	for _, ch := range arg {
		if legDigits == decimalMaxDigits {
			bi.mulWord(decimalBase)
			bi.addWord(leg)
			leg, legDigits = 0, 0
		}
		leg = mulAddWBB(leg, digitBase, uint8(ch-'0'))
		legDigits++
	}
	bi.mulWord(Word(math.Pow10(legDigits)))
	bi.addWord(leg)
}

// String - implements string conversion interface.
// Actually cheating, using big.Int
func (bi *BigInt) String() string {
	result := new(big.Int)
	conv := new(big.Int)

	result.SetUint64(0)

	for i := len(bi.legs) - 1; i >= 0; i-- {
		conv.SetUint64(uint64(bi.legs[i]))
		result.Lsh(result, wordBits)
		result.Add(result, conv)
	}

	if bi.negative {
		result.Neg(result)
	}
	return fmt.Sprint(result)
}

// Cmp is just a standard compare function.
func (bi *BigInt) Cmp(num *BigInt) int {
	const leftIsSmaller, areEqual, rightIsSmaller = -1, 0, 1

	leftSize, rightSize := len(bi.legs), len(num.legs)

	if leftSize > 0 && rightSize > 0 && bi.negative != num.negative {
		if bi.negative {
			return leftIsSmaller
		}
		return rightIsSmaller
	}

	if leftSize < rightSize {
		return leftIsSmaller
	}
	if leftSize > rightSize {
		return rightIsSmaller
	}

	i := len(bi.legs) - 1
	for i > 0 && bi.legs[i] == num.legs[i] {
		i--
	}
	if bi.legs[i] < num.legs[i] {
		return leftIsSmaller
	}
	if bi.legs[i] > num.legs[i] {
		return rightIsSmaller
	}
	return areEqual
}

func (bi *BigInt) copy(src *BigInt) {
	if bi != src {
		*bi = *src
		bi.legs = append([]Word{}, bi.legs...)
	}
}

func (bi *BigInt) clone() BigInt {
	var cloned BigInt
	cloned.copy(bi)
	return cloned
}

// Mul multiplies the BigInt by other BigInt.
func (bi *BigInt) Mul(a, b *BigInt) *BigInt {
	bi.copy(a)
	bi.mulNaive(b)
	return bi
}

// mulNaive is naive implementation of BigInts multiplication.
func (bi *BigInt) mulNaive(num *BigInt) {
	negative := (bi.negative != num.negative)

	legs := make([]Word, len(bi.legs))
	copy(legs, bi.legs)

	bi.legs = bi.legs[:0]

	for i, leg := range num.legs {
		var part BigInt
		part.legs = append(part.legs, legs...)
		part.LshWord(i)
		part.mulWord(leg)
		bi.Add(bi, &part)
	}
	bi.negative = negative
}

// Mul multiplies the BigInt by a Word.
func (bi *BigInt) mulWord(num Word) *BigInt {
	if len(bi.legs) == 0 {
		return bi
	}

	var carryHigh, carryLow Word

	for i := range bi.legs {
		var carryHigh1, carryHigh2 Word

		carryHigh1, carryLow, bi.legs[i] = mulAddWWW(bi.legs[i], num, carryLow)
		carryHigh2, carryLow = addWW(carryLow, carryHigh)
		_, carryHigh = addWW(carryHigh1, carryHigh2)

	}

	if carryHigh != 0 {
		bi.legs = append(bi.legs, carryLow, carryHigh)
	} else if carryLow != 0 {
		bi.legs = append(bi.legs, carryLow)
	}
	return bi
}

//LshWord left shifts BigInt by a sh words
func (bi *BigInt) LshWord(sh int) {
	head := make([]Word, sh, sh+len(bi.legs))
	bi.legs = append(head, bi.legs...)
}

// Add sets BigInto to += num
func (bi *BigInt) Add(a, b *BigInt) *BigInt {
	if a.negative != b.negative {
		// x + (-y) = x-y
		// (-x) + y = y-x
		if b.negative {
			b.negative = !b.negative
			bi.Sub(a, b)
			b.negative = !b.negative
		} else {
			a.negative = !a.negative
			bi.Sub(b, a)
			a.negative = !a.negative
		}
	} else {
		bi.copy(a)
		for i, leg := range b.legs {
			bi.addWordAt(leg, i)
		}
	}
	return bi
}

func (bi *BigInt) addWord(num Word) {
	bi.addWordAt(num, 0)
}

func (bi *BigInt) addWordAt(num Word, pos int) {
	if len(bi.legs[pos:]) == 0 {
		bi.legs = append(bi.legs, num)
		return
	}

	carryHigh, carryLow := Word(0), num
	for i := pos; i < len(bi.legs); i++ {
		carryLow, bi.legs[i] = addWW(bi.legs[i], carryLow)
		carryHigh, carryLow = addWW(carryHigh, carryLow)
		// if carryHigh != 0 {
		// 	// Temporary safty check.
		// 	panic("overflow!")
		// }
	}

	if carryLow != 0 {
		bi.legs = append(bi.legs, carryLow)
	}
}

// Sub sets BigInto to -= num
func (bi *BigInt) Sub(a, b *BigInt) *BigInt {
	// x - (-y) = x + y
	// (-x) - y = -(x + y)
	if a.negative != b.negative {
		a.negative = !a.negative
		bi.Add(a, b)
		a.negative = !a.negative
		bi.negative = a.negative
		return bi
	}

	if a.Cmp(b) < 0 {
		a, b = b, a
		bi.copy(a)
		bi.negative = !bi.negative
	} else {
		bi.copy(a)
	}

	if len(bi.legs) < len(b.legs) {
		tail := make([]Word, len(b.legs)-len(a.legs))
		// TODO Just copied legs ... don't append?
		bi.legs = append(a.legs, tail...)
	}

	var carry Word
	for i := range b.legs {
		// From big.Int.subVV_g
		x, y := bi.legs[i], b.legs[i]
		z := x - y - carry
		bi.legs[i] = z
		carry = ((y & ^x) | ((y | ^x) & z)) >> (wordBits - 1)
	}

	for i := len(b.legs); i < len(bi.legs); i++ {
		x := bi.legs[i]
		z := x - carry
		carry = ((0 & ^x) | ((0 | ^x) & z)) >> (wordBits - 1)
		bi.legs[i] = z
	}

	// Not needed as we ensured that we substract from bigger number
	// if carry != 0 {
	// 	// bi.legs = append(bi.legs, carry)
	// 	// bi.negative = !bi.negative
	// }

	return bi
}

func mulAddWBB(w Word, mulVal, addVal uint8) Word {
	return w*Word(mulVal) + Word(addVal)
}

func mulAddWWW(w, mulVal, addVal Word) (carryHigh, carryLow, ret Word) {
	var mulCarry, addCarry Word

	mulCarry, ret = mulWW(w, mulVal)
	addCarry, ret = addWW(ret, addVal)
	carryHigh, carryLow = addWW(mulCarry, addCarry)

	return
}

func addWW(num1, num2 Word) (high, low Word) {
	highNum1, lowNum1 := (num1 >> wordLowBits), (num1 & wordLowMask)
	highNum2, lowNum2 := (num2 >> wordLowBits), (num2 & wordLowMask)

	high = (highNum1 + highNum2 + ((lowNum1 + lowNum2) >> wordLowBits)) >> wordLowBits
	low = num1 + num2

	return high, low
}

func subWW(num1, num2 Word) (borrow, ret Word) {
	// copied from subVV_g, original idea from "Hacker's Delight"
	ret = num1 - num2
	borrow = ((num1 & ^num2) | ((num1 | ^num2) & ret)) >> 1

	return
}

func mulWW(num1, num2 Word) (high, low Word) {
	var carry Word
	highNum1, lowNum1 := (num1 >> wordLowBits), (num1 & wordLowMask)
	highNum2, lowNum2 := (num2 >> wordLowBits), (num2 & wordLowMask)

	low = lowNum1 * lowNum2
	mid := (highNum1 * lowNum2) + (lowNum1 * highNum2)
	high = highNum1 * highNum2

	carry, low = addWW(low, (mid&wordLowMask)<<wordLowBits)
	high += mid>>wordLowBits + carry

	return
}

func max(a, b int) int {
	if a < b {
		return a
	}
	return b
}

func karatsuba(num1, num2 BigInt) BigInt {
	negative := (num1.negative != num2.negative)
	num1.negative = false
	num2.negative = false
	result := karatsubaAbs(num1, num2)
	result.negative = negative
	return result
}

func karatsubaAbs(num1, num2 BigInt) BigInt {
	if len(num2.legs) == 1 {
		result := num1.clone()
		result.mulWord(num2.legs[0])
		return result
	}
	if len(num1.legs) == 1 {
		result := num2.clone()
		result.mulWord(num1.legs[0])
		return result
	}

	// Calculates the size of the numbers.
	m2 := max(len(num1.legs), len(num2.legs)) / 2

	// Splits numbers into high and low halfs
	var high1, low1, high2, low2 BigInt
	high1.legs = num1.legs[m2:]
	low1.legs = num1.legs[:m2]
	high2.legs = num2.legs[m2:]
	low2.legs = num2.legs[:m2]

	// 3 calls made to numbers approximately half the size.
	z0 := karatsuba(low1, low2)
	var t1, t2 BigInt
	z1 := karatsuba(*t1.Add(&low1, &high1), *t2.Add(&low2, &high2))
	z2 := karatsuba(high1, high2)

	result := z2.clone()
	result.LshWord(m2 * 2)

	tmp := z1.clone()
	tmp.Sub(&tmp, &z2)
	tmp.Sub(&tmp, &z0)
	tmp.LshWord(m2)
	result.Add(&result, &tmp)

	result.Add(&result, &z0)

	return result

}

func isInteger(arg string) bool {
	if arg[0] == '-' && len(arg) > 1 {
		arg = arg[1:]
	}
	for _, symbol := range arg {
		if !unicode.IsDigit(symbol) {
			return false
		}
	}
	return true
}

func multiply(num1, num2 string) string {
	a, b, c := new(BigInt), new(BigInt), new(BigInt)
	a.SetString(num1)
	b.SetString(num2)
	c.Mul(a, b)
	return fmt.Sprint(c)
}

func main() {
	const inpitArgsError = "Expected 2 integers"
	if len(os.Args[1:]) != 2 {
		log.Fatal(inpitArgsError)
	}
	x := uint32(1)
	y := uint32(9)
	c := (^x & y) | ((^x | y) & (x - y))
	fmt.Println(x-y, c)

	num1, num2 := os.Args[1], os.Args[2]

	if !isInteger(num1) || !isInteger(num2) {
		log.Fatal(inpitArgsError)
	}

	fmt.Println(num1, "*", num2, "=", multiply(num1, num2))

	a, b := new(BigInt), new(BigInt)
	a.SetString(num1)
	b.SetString(num2)
	res := karatsuba(*a, *b)
	fmt.Println("karatsuba, ", num1, "*", num2, "=", res.String())
}
