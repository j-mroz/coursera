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
	*bi = *src
	bi.legs = append([]Word{}, bi.legs...)
}

// Mul multiplies the BigInt by other BigInt.
func (bi *BigInt) Mul(a, b *BigInt) *BigInt {
	// *bi = *a
	// bi.legs = append([]Word{}, bi.legs...)
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
		var carryHigh1, carryHigh2, carryHigh3 Word

		carryHigh1, carryLow, bi.legs[i] = mulAddWWW(bi.legs[i], num, carryLow)
		carryHigh2, carryLow = addWW(carryLow, carryHigh)
		carryHigh3, carryHigh = addWW(carryHigh1, carryHigh2)
		if carryHigh3 != 0 {
			// temporary safty check
			panic("overflow!")
		}
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
	head := make([]Word, sh, len(bi.legs))
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
		if carryHigh != 0 {
			// Temporary safty check.
			panic("overflow!")
		}
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

	// rhsLegs := num.legs
	if a.Cmp(b) < 0 {
		a, b = b, a
		// rhsLegs = bi.legs
		// bi.legs = append([]Word{}, num.legs...)
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
		carry = ((y & ^x) | ((y | ^x) & z)) >> (wordBits - 1)
		bi.legs[i] = z
	}

	if carry != 0 {
		bi.legs = append(bi.legs, carry)
		bi.negative = !bi.negative
	}

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
	highNum1, lowNum1 := (num1 >> wordLowBits), (num1 & wordLowMask)
	highNum2, lowNum2 := (num2 >> wordLowBits), (num2 & wordLowMask)

	low = lowNum1 * lowNum2
	mid := (highNum1 * lowNum2) + (lowNum1 * highNum2)
	high = highNum1 * highNum2
	low += (mid & wordLowMask) << wordLowBits
	high += mid >> wordLowBits

	return
}

func max(a, b int) int {
	if a < b {
		return a
	}
	return b
}

// func karatsuba(num1, num2 *BigInt) *BigInt {
// 	// if len(num2.legs) == 1 {
// 	// 	return num1.mulWord(num2.legs[0])
// 	// }
// 	// if len(num1.legs) == 1 {
// 	// 	return num2.mulWord(num1.legs[0])
// 	// }
// 	//
// 	// // calculates the size of the numbers
// 	// m := max(len(num1.legs), len(num2.legs)) / 2
// 	//
// 	// var high1, low1, high2, low2 BigInt
// 	// // high1.legs = append([]Word{}, num1.legs[m:]...)
// 	// // low1.legs = append([]Word{}, num1.legs[:m]...)
// 	// // high2.legs = append([]Word{}, num2.legs[m:]...)
// 	// // low2.legs = append([]Word{}, num2.legs[:m]...)
// 	// //
// 	// // /* 3 calls made to numbers approximately half the size */
// 	// z0 := karatsuba(&low1, &low2)
// 	// z1 := karatsuba(low1.Add(&high1), low2.Add(&high2))
// 	// z2 := karatsuba(&high1, &high2)
// 	//
// 	//
// 	// z1.Sub(z2).Sub(z0)
// 	// return (z2*10 ^ (2 * m2)) + ((z1-z2-z0)*10 ^ (m2)) + (z0)
// 	// return num1
// }

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
}
