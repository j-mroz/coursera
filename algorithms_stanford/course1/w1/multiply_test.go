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
