/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

/**
 * This class implements a complex number and defines complex
 * arithmetic and mathematical functions.
 *
 * From http://www.math.ksu.edu/~bennett/jomacg/c.html
 * Updated February 27, 2001
 * Copyright 1997-2001
 * @version 1.0
 * @author Andrew G. Bennett
 */

package vnmr.util;

public class Complex extends Object {

    public static void main(String[] args) {
        double f0 = 400e6;      // Calc. nominal values for this frequency
        double cMatch = 7.15e-14;
        double lCoil = 100e-9;
        double rCoil = 0.1;
        double cTune = 1 / (Math.pow(2 * Math.PI * f0, 2) * lCoil);
        double freq = 400e6;
        double w = 2 * Math.PI * freq; // omega; circular freq
        Complex z0 = new Complex(50, 0); // 50 Ohm load
        Complex zMatch = new Complex(0, -1 / (w * cMatch));
        Complex zTune = new Complex(0, -1 / (w * cTune));
        Complex zCoil = new Complex(rCoil, w * lCoil);
        // z = zMatch + 1 / (1/zTune + 1/zCoil)
        Complex z = zMatch.plus((zTune.inv().plus(zCoil.inv())).inv());
        // reflection = (z - z0) / (z + z0)
        Complex r = (z.minus(z0).div(z.plus(z0)));
        System.out.println("z=" + z + ", r=" + r);
    }


    private double x,y;

    /**
     * Constructs the complex number z
     * @param u Real part
     * @param v Imaginary part
     */
    public Complex(double u,double v) {
        x=u;
        y=v;
    }

    /**
     * Real part of this Complex number
     * (The x-coordinate in rectangular coordinates).
     * @return Re[z] where z is this Complex number.
     */
    public double real() {
        return x;
    }

    /**
     * Imaginary part of this Complex number.
     * (The y-coordinate in rectangular coordinates).
     * @return Im[z] where z is this Complex number.
     */
    public double imag() {
        return y;
    }

    /**
     * The square of the modulus of this Complex number.
     * I.e., the squared distance from the origin in polar coordinates.
     *  @return |z|**2 where z is this Complex number.
     */
    public double mod2() {
        return x * x + y * y;
    }

    /**
     * Modulus of this Complex number
     * (the distance from the origin in polar coordinates).
     *  @return |z| where z is this Complex number.
     */
    public double mod() {
        return Math.hypot(x, y);
    }

    /**
     * Argument of this Complex number
     * (the angle in radians with the x-axis in polar
     * coordinates).
     * @return arg(z) where z is this Complex number.
     */
    public double arg() {
        return Math.atan2(y,x);
    }

    /**
     * Complex conjugate of this Complex number
     * (the conjugate of x+i*y is x-i*y).
     * @return z-bar where z is this Complex number.
     */
    public Complex conj() {
        return new Complex(x,-y);
    }

    /**
     * Addition of Complex numbers (doesn't change this
     * Complex number).
     * <br>(x+i*y) + (s+i*t)
     * @param w is the number to add.
     * @return z+w where z is this Complex number.
     */
    public Complex plus(Complex w) {
        return new Complex(x+w.real(),y+w.imag());
    }

    /**
     * Subtraction of Complex numbers (doesn't change this
     * Complex number).
     * <br>(x+i*y) - (s+i*t)
     * @param w is the number to subtract.
     * @return z-w where z is this Complex number.
     */
    public Complex minus(Complex w) {
        return new Complex(x-w.real(),y-w.imag());
    }

    /**
     * Complex multiplication (doesn't change this Complex
     * number).
     * @param w is the number to multiply by.
     * @return z*w where z is this Complex number.
     */
    public Complex times(Complex w) {
        return new
                Complex(x*w.real()-y*w.imag(),x*w.imag()+y*w.real());
    }

    /**
     * Division of Complex numbers (doesn't change this
     * Complex number).
     * <br>(x+i*y)/(s+i*t) (s^2+t^2)
     * @param w is the number to divide by
     * @return new Complex number z/w where z is this
     * Complex number.
     */
    public Complex div(Complex w) {
        double den=Math.pow(w.mod(),2);
        return new Complex((x*w.real()+y*w.imag())/den,
                           (y*w.real()-x*w.imag())/den);
    }

    /**
     * One over this Complex number (doesn't change this Complex number).
     * @return New Complex number 1/z where z is this Complex number.
     */
    public Complex inv() {
        double den = Math.pow(mod(), 2);
        return new Complex(x / den, -y / den);
    }

    /**
     * Complex exponential (doesn't change this Complex
     * number).
     * @return exp(z) where z is this Complex number.
     */
    public Complex exp() {
        return new
                Complex(Math.exp(x)*Math.cos(y),Math.exp(x)*Math.sin(y));
    }

    /**
     * Principal branch of the Complex logarithm of this
     * Complex number.
     * (doesn't change this Complex number).
     * The principal branch is the branch with -pi < arg <pi.
     * @return log(z) where z is this Complex number.
     */
    public Complex log() {
        return new Complex(Math.log(this.mod()),this.arg());
    }

    /**
     * Complex square root (doesn't change this complex
     * number).
     * Computes the principal branch of the square root,
     * which is the one whose argument is half of this number's argument.
     * @return sqrt(z) where z is this Complex number.
     */
    public Complex sqrt() {
        double r=Math.sqrt(this.mod());
        double theta=this.arg()/2;
        return new Complex(r*Math.cos(theta),r*Math.sin(theta));
    }

    // Real cosh function (used to compute complex trig functions)
    private double cosh(double theta) {
        return (Math.exp(theta)+Math.exp(-theta))/2;
    }

    // Real sinh function (used to compute complex trig functions)
    private double sinh(double theta) {
        return (Math.exp(theta)-Math.exp(-theta))/2;
    }

    /**
     * Sine of this Complex number (doesn't change this Complex number).
     * <br>sin(z)
     * @return sin(z) where z is this Complex number.
     */
    public Complex sin() {
        return new Complex(cosh(y)*Math.sin(x),sinh(y)*Math.cos(x));
    }

    /**
     * Cosine of this Complex number (doesn't change this Complex number).
     * <br>cos(z)
     * @return cos(z) where z is this Complex number.
     */
    public Complex cos() {
        return new Complex(cosh(y)*Math.cos(x),-sinh(y)*Math.sin(x));
    }

    /**
     * Hyperbolic sine of this Complex number
     * (doesn't change this Complex number).
     * <br>sinh(z)
     * @return sinh(z) where z is this Complex number.
     */
    public Complex sinh() {
        return new Complex(sinh(x)*Math.cos(y),cosh(x)*Math.sin(y));
    }

    /**
     * Hyperbolic cosine of this Complex number
     * (doesn't change this Complex number).
     * <br>cosh(z)
     * @return cosh(z) where z is this Complex number.
     */
    public Complex cosh() {
        return new Complex(cosh(x)*Math.cos(y),sinh(x)*Math.sin(y));
    }

    /**
     * Tangent of this Complex number (doesn't change this Complex number).
     * <br>tan(z)
     * @return tan(z) where z is this Complex number.
     */
    public Complex tan() {
        return (this.sin()).div(this.cos());
    }

    /**
     * Negative of this complex number (chs stands for change sign).
     * This produces a new Complex number and doesn't change
     * this Complex number.
     * <br>-(x+i*y)
     * @return -z where z is this Complex number.
     */
    public Complex chs() {
        return new Complex(-x,-y);
    }

    /**
     * Rotate this complex number through a multiple of PI/2.
     * This produces a new Complex number and doesn't change
     * this Complex number.
     *
     * @param quadrant What angle to rotate by: angle = (quadrant * PI / 2),
     * @return New Complex number with the same modulus as this one
     * but rotated phase, or itself if (quadrant % 4) = 0.
     */
    public Complex rotate(int quadrant) {
        quadrant = quadrant % 4;
        switch (quadrant) {
        default:
            return this;
        case 1:
        case -3:
            return new Complex(-y, x);
        case 2:
        case -2:
            return new Complex(-x, -y);
        case 3:
        case -1:
            return new Complex(y, -x);
        }
    }

    /**
     * Rotate the Complex data in the given array through a multiple of PI/2.
     * This puts new Complex numbers in the same data array.
     * @param data Array of complex numbers.
     * @param quadrant What angle to rotate by: 0=no change, 1=PI/2,
     * 2=PI, 3=3*PI/2. Interpreted mod(4).
     */
    public static void rotateData(Complex[] data, int quadrant) {
        if ((quadrant = (quadrant % 4)) != 0) {
            int size = data.length;
            for (int i = 0; i < size; i++) {
                data[i] = data[i].rotate(quadrant);
            }
        }
    }

    /**
     * String representation of this Complex number.
     * @return x+i*y, x-i*y, x, or i*y as appropriate.
     */
    public String toString() {
        if (x!=0 && y>0) {
            return x+" + "+y+"i";
        }
        if (x!=0 && y<0) {
            return x+" - "+(-y)+"i";
        }
        if (y==0) {
            return String.valueOf(x);
        }
        if (x==0) {
            return y+"i";
        }
        // shouldn't get here (unless Inf or NaN)
        return x+" + i*"+y;
    }
}
