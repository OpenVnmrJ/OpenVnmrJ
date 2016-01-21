/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
 * Most of this is based on Fortran routines from:
 * www.sali.freeservers.com/engineering/fortran.html
 */

package vnmr.util;

public class Fit {

    /**
     * Use this to test the class as a standalone program.
     */
    static public void main(String s[]) {

        // Test polyFit
        double[] x = {-2, -1,  0,  1,  2};
        double[] y = { 0,  3,  5,  4, -1};
        double[] c = null;
        for (int j = 0; j < 10; j++) {
            long t0 = System.currentTimeMillis();
            for (int i = 0; i < 1000; i++) {
                c = polyFit(x, y, 2);
            }
            long t1 = System.currentTimeMillis();
            System.out.println("Time per fit = " + ((t1 - t0) * 1) + " us");
        }
        System.out.print("coeffs:"); print(c);
        // Correct answer:  4.914285714285715  -0.1  -1.3571428571428572
    }

    /**
     * Fit x-y data with a polynomial by unweighted least-squares regression.
     * <br> Y = A0 + A1 * X + A2 * X^2 + ...
     * @param x Array of X values.
     * @param y Array of corresponding Y values.
     * @param order The order of the polynomial to fit.  (One less than
     * the number of coefficients returned.)
     * @return An array giving the coefficients of the best fit polynomial.
     * The order is: A0, A1, ...
     */
    public static double[] polyFit(final double[] x,
                                   final double[] y, int order) {

        int n = Math.min(x.length, y.length);
        if (order >= n) {
            return null;
        }

        double ymean = 0;
        for (int i = 0; i < n; i++) {
            ymean += y[i];
        }
        ymean /= n;

        int terms = order + 1;
        int r = terms + 1;

        double[][] am = new double[terms][r];
        for (int i = 0; i < terms; i++) {
            for (int j = 0; j < terms; j++) {
                int k = i + j;
                for (int ii = 0; ii < n; ii++) {
                    am[i][j] += Math.pow(x[ii], k);
                }
            }
            for (int ii = 0; ii < n; ii++) {
                am[i][terms] += y[ii] * Math.pow(x[ii], i);
            }
        }

        double[] coeff = gauss(am);
        //System.out.println("am=");
        //print(am);
        return coeff;
    }

    public static double[] gauss(double[][] am) {
        int n = am.length;
        int m = n + 1;
        final double epsil = 1e-6;
        boolean chec = true;

        // Initialize column order vector
        int[] ordc = new int[n];
        int[] ord = new int[n];
        for (int i = 0; i < n; i++) {
            ordc[i] = i;
        }

        // Segment for partial pivoting
        int n1 = n - 1;
        for (int p = 0; p < n1; p++) {
            pivot(am, ord, ordc, p);

            // Triangularization by eliminating the variables
            int kk = p + 1;
            for (int i = kk; i < n; i++) {
                double qt = 0;
                if (Math.abs(am[p][p]) < epsil) {
                    chec = false;
                } else {
                    qt = am[i][p] / am[p][p];
                }
                for (int j = p; j < m; j++) {
                    am[i][j] -= qt * am[p][j];
                }
            }
        }

        // Checking for the singularity of coefficient matrix
        boolean determ = true;
        for (int i = 0; i < n; i++) {
            if (!chec || Math.abs(am[i][i]) < epsil) {
                determ = false;
                //System.out.println("Gauss: no solution");
                return null;
            }
        }

        if (determ && chec) {
            // Back substitution
            double[] xm = new double[n];
            xm[n-1] = am[n-1][m-1] / am[n-1][n-1];
            for (int i = n - 2; i >= 0; --i) {
                double sum = 0;
                for (int j = i + 1; j < n; j++) {
                    sum += am[i][j] * xm[j];
                }
                xm[i] = (am[i][m-1] - sum) / am[i][i];
            }

            // Rearrange the solution vector
            double[] ym = new double[n];
            for (int i = 0; i < n; i++) {
                ym[ordc[i]] = xm[i];
            }
            return ym;
        }

        return null;
    }

    public static void pivot(double[][] a, int[] ord, int[] ordc, int i) {
        int n = a.length;
        int row = i;
        int col = i;
        for (int p = i; p < n; p++) {
            for (int r = i; r < n; r++) {
                if (Math.abs(a[row][col]) < Math.abs(a[p][r])) {
                    row = p;
                    col = r;
                }
            }
        }

        if (col != i) {
            for (int ii = 0; ii < n; ii++) {
                double tem = a[ii][i];
                a[ii][i] = a[ii][col];
                a[ii][col] = tem;
            }
            int t = ordc[i];
            ordc[i] = ordc[col];
            ordc[col] = t;
        }

        int m = n + 1;
        if (row != i) {
            for (int jj = 0; jj < m; jj++) {
                double tem = a[i][jj];
                a[i][jj] = a[row][jj];
                a[row][jj] = tem;
            }
            int t = ord[i];
            ord[i] = ord[row];
            ord[row] = t;
        }
    }

    /**
     * Handy method to print a 2D array for debugging.
     */
    public static void print(double[][] a) {
        if (a == null) {
            System.out.println("  null");
            return;
        }
        for (int i = 0; i < a.length; i++) {
            for (int j = 0; j < a[i].length; j++) {
                System.out.print("  " + a[i][j]);
            }
            System.out.println();
        }
    }

    /**
     * Handy method to print a 1D array for debugging.
     */
    public static void print(double[] a) {
        if (a == null) {
            System.out.println("  null");
            return;
        }
        for (int i = 0; i < a.length; i++) {
            System.out.print("  " + a[i]);
        }
        System.out.println();
    }
}
