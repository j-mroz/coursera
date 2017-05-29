package main

import (
	"fmt"
	"math/big"
	"testing"
)

func TestMultiplySmallPositive(t *testing.T) {
	for i := 0; i < 20; i++ {
		num := fmt.Sprintf("%d", i)
		ret := multiply(num, num)
		expected := fmt.Sprintf("%d", i*i)

		if ret != expected {
			t.Errorf("multiply(%s, %s) -> %s; expected %s", num, num, ret, expected)
		}
	}
}

func TestMultiplyBigPositive(t *testing.T) {
	a := "3141592653589793238462643383279502884197169399375105820974944592"
	b := "2718281828459045235360287471352662497757247093699959574966967627"

	// Calculate expected value using big.Int
	abig, bbig, cbig := new(big.Int), new(big.Int), new(big.Int)
	abig.SetString(a, 10)
	bbig.SetString(b, 10)
	cbig.Mul(abig, bbig)
	expected := fmt.Sprint(cbig)

	ret := multiply(a, b)
	if ret != expected {
		t.Errorf("\nmultiply(\n"+
			"    %s,\n"+
			"    %s) -> %s;\n"+
			"expected %s",
			a, b, ret, expected)
	}
}

func TestMultiplyBigNegative(t *testing.T) {
	a := "3141592653589793238462643383279502884197169399375105820974944592"
	b := "-2718281828459045235360287471352662497757247093699959574966967627"

	// Calculate expected value using big.Int
	abig, bbig, cbig := new(big.Int), new(big.Int), new(big.Int)
	abig.SetString(a, 10)
	bbig.SetString(b, 10)
	cbig.Mul(abig, bbig)
	expected := fmt.Sprint(cbig)

	ret := multiply(a, b)
	if ret != expected {
		t.Errorf("\nmultiply(\n"+
			"    %s,\n"+
			"    %s) -> %s;\n"+
			"expected %s",
			a, b, ret, expected)
	}
}

func TestMulWord(t *testing.T) {
	a := "2495083031227764747"
	b := "2495083031227764747"
	bw := Word(2495083031227764747)

	// Calculate expected value using big.Int
	abig, bbig, cbig := new(big.Int), new(big.Int), new(big.Int)
	abig.SetString(a, 10)
	bbig.SetString(b, 10)
	cbig.Mul(abig, bbig)
	expected := fmt.Sprint(cbig)

	var aa BigInt
	aa.SetString(a)
	aa.mulWord(bw)
	if aa.String() != expected {
		t.Errorf("%s.mulWord(%s)->%s != %s", a, b, aa.String(), expected)
	}
}

func TestSubBig(t *testing.T) {
	type SubTestData struct {
		a string
		b string
	}
	dataset := []SubTestData{
		{"967627", "3141592653589793238462643383279502884197169399375105820974944592"},
		{"3141592653589793238462643383279502884197169399375105820974944592", "967627"},
		{"-3141592653589793238462643383279502884197169399375105820974944592", "967627"},
		{"967627", "-3141592653589793238462643383279502884197169399375105820974944592"},
		{"3141592653589793238462643383279502884197169399375105820974944592", "-3141592653589793238462643383279502884197169399375105820974944592"},
		{"-1", "3141592653589793238462643383279502884197169399375105820974944592"},
		{"1", "-3141592653589793238462643383279502884197169399375105820974944592"},
		{"-1", "-3141592653589793238462643383279502884197169399375105820974944592"},
		{"438560122493087261206387113258472915044", "346053913903990441693511109927633067322"},
		{"432263421269274844403238904489861846043", "346053913903990441693511109927633067322"},
	}

	for _, ds := range dataset {
		// Calculate expected value using big.Int
		var a, b, c big.Int
		var d, e, f BigInt

		a.SetString(ds.a, 10)
		b.SetString(ds.b, 10)
		c.Sub(&a, &b)

		d.SetString(ds.a)
		e.SetString(ds.b)
		f.Sub(&d, &e)

		if f.String() != c.String() {
			t.Errorf("%v.Sub(%v)->%v != %v", a.String(), b.String(), f.String(), c.String())
		}
	}
}

func TestSubBig2(t *testing.T) {

	a_ := "432263421269274844403238904489861846043"
	b_ := "339757212680178024890362901159021998321"

	// Calculate expected value using big.Int
	var a, b, c big.Int
	var e, f, g BigInt

	a.SetString(a_, 10)
	b.SetString(b_, 10)
	c.Sub(&a, &b)

	e.SetString(a_)
	f.SetString(b_)
	g.Sub(&e, &f)

	if c.String() != g.String() {
		t.Errorf("%v.Sub(%v)->%v != %v", a.String(), b.String(), g.String(), c.String())
	}
}

func TestSubWW(t *testing.T) {
	a := 820974944592
	b := 967627

	_, low := subWW(Word(a), Word(b))

	if low != Word(a-b) {
		t.Errorf("sub(%v, %v)->%v != %v", a, b, low, a-b)
	}
}

func TestAddBig(t *testing.T) {
	type SubTestData struct {
		a string
		b string
	}
	dataset := []SubTestData{
		{"967627", "3141592653589793238462643383279502884197169399375105820974944592"},
		{"3141592653589793238462643383279502884197169399375105820974944592", "967627"},
		{"-3141592653589793238462643383279502884197169399375105820974944592", "967627"},
		{"967627", "-3141592653589793238462643383279502884197169399375105820974944592"},
		{"3141592653589793238462643383279502884197169399375105820974944592", "-3141592653589793238462643383279502884197169399375105820974944592"},
		{"-1", "3141592653589793238462643383279502884197169399375105820974944592"},
		{"1", "-3141592653589793238462643383279502884197169399375105820974944592"},
		{"-1", "-3141592653589793238462643383279502884197169399375105820974944592"},
	}

	for _, ds := range dataset {
		// Calculate expected value using big.Int
		var a, b, c big.Int
		var d, e, f BigInt

		a.SetString(ds.a, 10)
		b.SetString(ds.b, 10)
		c.Add(&a, &b)

		d.SetString(ds.a)
		e.SetString(ds.b)
		f.Add(&d, &e)

		if f.String() != c.String() {
			t.Errorf("%v.Add(%v)->%v != %v", a.String(), b.String(), f.String(), c.String())
		}
	}
}

func TestKaratsuba(t *testing.T) {

	a := "3141592653589793238462643383279502884197169399375105820974944592"
	b := "2718281828459045235360287471352662497757247093699959574966967627"

	// Calculate expected value using big.Int
	abig, bbig, cbig := new(big.Int), new(big.Int), new(big.Int)
	abig.SetString(a, 10)
	bbig.SetString(b, 10)
	cbig.Mul(abig, bbig)
	expected := fmt.Sprint(cbig)

	var d, e BigInt
	d.SetString(a)
	e.SetString(b)

	ret := karatsuba(d, e)
	if ret.String() != expected {
		t.Errorf("\nmultiply(\n"+
			"    %s,\n"+
			"    %s) -> %s;\n"+
			"expected %s",
			a, b, ret.String(), expected)
	}
}

func TestMultiplyBigPositive2(t *testing.T) {
	a := "46288836626478949364051309001780866727"
	b := "46288836626478949364051309001780866727"

	// Calculate expected value using big.Int
	abig, bbig, cbig := new(big.Int), new(big.Int), new(big.Int)
	abig.SetString(a, 10)
	bbig.SetString(b, 10)
	cbig.Mul(abig, bbig)
	expected := fmt.Sprint(cbig)

	ret := multiply(a, b)
	if ret != expected {
		t.Errorf("\nmultiply(\n"+
			"    %s,\n"+
			"    %s) -> %s;\n"+
			"expected %s",
			a, b, ret, expected)
	}
}

func TestKaratsuba2(t *testing.T) {
	type SubTestData struct {
		a string
		b string
	}
	dataset := []SubTestData{
		{"967627", "3141592653589793238462643383279502884197169399375105820974944592"},
		{"3141592653589793238462643383279502884197169399375105820974944592", "967627"},
		{"-3141592653589793238462643383279502884197169399375105820974944592", "967627"},
		{"967627", "-3141592653589793238462643383279502884197169399375105820974944592"},
		{"46288836626478949364051309001780866727", "46288836626478949364051309001780866727"},
		{"3141592653589793238462643383279502884197169399375105820974944592", "3141592653589793238462643383279502884197169399375105820974944592"},
		{"3141592653589793238462643383279502884197169399375105820974944592", "-3141592653589793238462643383279502884197169399375105820974944592"},
		{"-1", "3141592653589793238462643383279502884197169399375105820974944592"},
		{"1", "-3141592653589793238462643383279502884197169399375105820974944592"},
		{"-1", "-3141592653589793238462643383279502884197169399375105820974944592"},
	}

	for _, ds := range dataset {
		// Calculate expected value using big.Int
		var a, b, c big.Int
		var d, e BigInt

		a.SetString(ds.a, 10)
		b.SetString(ds.b, 10)
		c.Mul(&a, &b)

		d.SetString(ds.a)
		e.SetString(ds.b)

		ret := karatsuba(d, e)
		if ret.String() != c.String() {
			// t.Error(ret, c)
			t.Errorf("karatsuba(%v, %v)->%v != %v", ds.a, ds.b, ret.String(), c.String())
		}
	}
}

func TestCmp(t *testing.T) {
	type CmpTestData struct {
		a        string
		b        string
		expected int
	}
	dataset := []CmpTestData{
		{"1", "2", -1},
		{"967620", "967628", -1},
		{"967629", "967628", 1},
		{"967620", "967620", 0},
		{"3141592653589793238462643383279502884197169399375", "9676231967627", 1},
		{"9676231967627", "3141592653589793238462643383279502884197169399375", -1},
		{"3141592653589793238462643383279502884197169399375", "3141592653589793238462643383279502884197169399375", 0},
	}

	for _, ds := range dataset {
		var a, b BigInt
		a.SetString(ds.a)
		b.SetString(ds.b)
		ret := a.Cmp(&b)
		if ret != ds.expected {
			t.Errorf("%v.cmp(%v)->%v != %v", a, b, ret, ds.expected)
		}
	}
}
