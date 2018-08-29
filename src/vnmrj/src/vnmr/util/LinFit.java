/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.io.*;

/**
 * A collection of static methods for fitting functions to data.
 * Algorithms are mostly based on Numerical Recipes routines.
 */
public class LinFit {

    /**
     * For testing the routines.
     */
    public static void main (String[] args) throws IOException {

        final String[] choices = {
            "SVDecompose",
            "SVDfit",
        };
        // NB: Keep these in sync with the above choices
        final int SVDCMP = 0;
        final int SVDFIT = 1;

        
        //BufferedReader reader
        //        = new BufferedReader(new InputStreamReader(System.in));

        String choice = choices[0];
        if (args.length > 0) {
            choice = args[0];
        }
        if (choice.equalsIgnoreCase(choices[SVDCMP])) {
            System.err.println("Test SVDecomposition");

            // Construct a random matrix
            int m = 50;
            int n = 10;
            double[][] a = new double[m][n];
            double[][] b = new double[m][n];
            for (int i = 0; i < m; i++) {
                for (int j = 0; j < n; j++) {
                    //String line = reader.readLine();
                    //a[i][j] = b[i][j] = Double.parseDouble(line);
                    a[i][j] = b[i][j] = Math.random();
                    //System.out.println(" " + a[i][j]);
                }
            }

            // Decompose it
            SVDecomposition svd = SVDecompose(b);

            // Check that it worked
            double max = 0;
            for (int i = 0; i < m; i++) {
                for (int j = 0; j < n; j++) {
                    double diff = a[i][j] - svd.recomposeValue(i, j);
                    System.err.println("diff(" + i + ", " + j + ") = " + diff);
                    if (max < Math.abs(diff)) {
                        max = Math.abs(diff);
                    }
                }
            }
            System.err.println("Max diff = " + max);
        } else if (choice.equalsIgnoreCase(choices[SVDFIT])) {
            System.err.println("Test SVD Polynomial Fitting");

            // Expected a[] for these data is {0.076190, -0.042857, 0.966667}
            double[] x = {0, 1, 2, 3, -1, -2, -3};
            double[] y = {0, 1, 4.1, 8.5, 1.1, 3.9, 9};
            PolyBasisFunctions poly = new PolyBasisFunctions(2);
            SvdFitResult ans = svdFit(x, y, null, poly);
            double[] a = ans.getCoeffs();
            double[][] covar = ans.getCovariance();
            for (int i = 0; i < a.length; i++) {
                System.err.println("a[" + i + "] = " + a[i]);
            }
            System.err.println("Covariances:");
            for (int i = 0; i < covar.length; i++) {
                for (int j = 0; j < covar[i].length; j++) {
                    System.err.print(covar[i][j] + "  ");
                }
                System.err.println();
            }
            
        } else {
            System.err.println("Unrecognized choice: \"" + choice + "\"");
            System.err.println("Valid choices are:");
            for (int i = 0; i < choices.length; i++) {
                System.err.println("  " + choices[i]);
            }
        }
    }

    /**
     * Calculate the hypotenuse of a right triangle with the given legs.
     * Done so as to avoid overflow when squaring large numbers.
     */
    public static double hypot(double a, double b) {
        double absa = Math.abs(a);
        double absb = Math.abs(b);
        if (absa > absb) {
            double b_a = absb / absa;
            return absa * Math.sqrt(1 + b_a * b_a);
        } else if (absb == 0) {
            return 0;
        } else {
            double a_b = absa / absb;
            return absb * Math.sqrt(1 + a_b * a_b);
        }
    }

    /**
     * Returns a number with the absolute value of the first argument,
     * and the sign of the second argument.
     */
    public static double sign(double value, double sign) {
        return sign >= 0 ? Math.abs(value) : -Math.abs(value);
    }

    /**
     * A 1-dimensional interface to the svdFit method.
     * @param x An array of input values at which data is available.
     * @param y An array a data points to fit.
     * @param sig The standard deviations of the individual data points.
     * If sig is null, unit weights are used.
     * @param basis Provides methods for evaluating the basis functions
     * of the fit.
     * @return Contains the fit coefficients and chi-square of the fit,
     * as well as a method for getting the covariances.
     */
    public static SvdFitResult svdFit(double[] x, double[] y, double[] sig,
                                      BasisFunctions basis) {
        int n = x.length;
        double[][] xx = new double[n][1];
        for (int i = 0; i < n; i++) {
            xx[i][0] = x[i];
        }
        return svdFit(xx, y, sig, basis);
    }


    /**
     * Determine coefficients for a multi-dimensional, linear fit.
     * @param x An array of input vectors.  Its first dimension is the
     * number of input points.  The second is the
     * number of dimensions in the multi-dimensional fit.
     * @param y An array a data points to fit.
     * @param sig The standard deviations of the individual data points.
     * If sig is null, unit weights are used.
     * @param basis Provides methods for evaluating the basis functions
     * of the fit.
     * @return Contains the fit coefficients and chi-square of the fit,
     * as well as a method for getting the covariances.
     */
    public static SvdFitResult svdFit(double[][] x, double[] y, double[] sig,
                                      BasisFunctions basis) {
        final double TOL = 1e-5;

        int ndata = y.length;
        int ma = basis.getYLength();

        double[] b = new double[ndata];
        double[] afunc;
        double[][] u = new double[ndata][ma];

        if (sig == null) {
            sig = new double[ndata];
            for (int i = 0; i < ndata; i++) {
                sig[i] = 1;
            }
        }

        for (int i = 0; i < ndata; i++) {
            afunc = basis.evaluateBasisFunctions(x[i]);
            double tmp = 1 / sig[i];
            for (int j = 0; j < ma; j++) {
                u[i][j] = afunc[j] * tmp;
            }
            b[i] = y[i] * tmp;
        }
        SVDecomposition svd = SVDecompose(u);

        double[] w = svd.w;
        double wmax = w[0];
        for (int j = 1; j < ma; j++) {
            if (w[j] > wmax) {
                wmax = w[j];
            }
        }
        double thresh = TOL * wmax;
        for (int j = 0; j < ma; j++) {
            if (w[j] < thresh) {
                w[j] = 0;
            }
        }
        double[] a = svdBacksub(svd, b);

        // Evaluate the fit
        double chisq = 0;
        for (int i = 0; i < ndata; i++) {
            afunc = basis.evaluateBasisFunctions(x[i]);
            double sum = 0;
            for (int j = 0; j < ma; j++) {
                sum += a[j] * afunc[j];
            }
            double tmp = (y[i] - sum) / sig[i];
            chisq += tmp * tmp;
        }
        SvdFitResult rtn = new SvdFitResult(a, chisq, svd);
        return rtn;
    }

    /**
     * Find the Singular Value Decomposition (SVD) of the given matrix.
     * The input array has dimensions MxN, where M >= N.  The input
     * array is destroyed in the computation.
     * @param a The matrix to be decomposed; destroyed by the computation.
     * @return Three matrices constituting the SVD of the input matrix.
     */
    public static SVDecomposition SVDecompose(double[][] a) {
        int m = a.length;
        int n = a[0].length;
        if (m < n) {
            System.err.println("SVDecompose: input matrix must have at "
                               + "least as many rows as columns.");
            return null;
        }

        int p = 0;
        double norm = 0;
        double scale = 0;
        double f;
        double g = 0;
        double h;
        double sum;
        double[] rv1 = new double[n];
        double[] w = new double[n];
        double[][] v = new double[n][n];

        for (int i = 0; i < n; i++) {
            p = i + 1;
            rv1[i] = scale * g;
            g = sum = scale = 0;
            if (i <= m) {
                for (int k = i; k < m; k++) {
                    scale += Math.abs(a[k][i]);
                }
                if (scale != 0) {
                    for (int k = i; k < m; k++) {
                        a[k][i] /= scale;
                        sum += a[k][i] * a[k][i];
                    }
                    f = a[i][i];
                    g = -sign(Math.sqrt(sum), f);
                    h = f * g - sum;
                    a[i][i] = f - g;
                    if (i != n) {
                        for (int j = p; j < n; j++) {
                            sum = 0;
                            for (int k = i; k < m; k++) {
                                sum += a[k][i] * a[k][j];
                            }
                            f = sum / h;
                            for (int k = i; k < m; k++) {
                                a[k][j] += f * a[k][i];
                            }
                        }
                    }
                    for (int k = i; k < m; k++) {
                        a[k][i] *= scale;
                    }
                }
            }
            w[i] = scale * g;
            g = sum = scale = 0;
            if (i < m && i != n - 1) {
                for (int k = p; k < n; k++) {
                    scale += Math.abs(a[i][k]);
                }
                if (scale != 0) {
                    for (int k = p; k < n; k++) {
                        a[i][k] /= scale;
                        sum += a[i][k] * a[i][k];
                    }
                    f = a[i][p];
                    g = -sign(Math.sqrt(sum), f);
                    h = f * g - sum;
                    a[i][p] = f - g;
                    for (int k = p; k < n; k++) {
                        rv1[k] = a[i][k] / h;
                    }
                    if (i != m - 1) {
                        for (int j = p; j < m; j++) {
                            sum = 0;
                            for (int k = p; k < n; k++) {
                                sum += a[j][k] * a[i][k];
                            }
                            for (int k = p; k < n; k++) {
                                a[j][k] += sum * rv1[k];
                            }
                        }
                    }
                    for (int k = p; k < n; k++) {
                        a[i][k] *= scale;
                    }
                }
            }
            norm = Math.max(norm, (Math.abs(w[i]) + Math.abs(rv1[i])));
        }

        /*
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                System.err.println("a(" + i + ", " + j + ") = " + a[i][j]);
            }
        }
        for (int j = 0; j < n; j++) {
            System.err.println("rv1(" + j + ") = " + rv1[j]);
            //System.err.println("w(" + j + ") = " + w[j]);
        }/*CMP*/
        for (int i = n - 1; i >= 0; i--) {
            if (i < n - 1) {
                if (g != 0) {
                    for (int j = p; j < n; j++) {
                        v[j][i] = (a[i][j] / a[i][p]) / g;
                    }
                    for (int j = p; j < n; j++) {
                        sum = 0;
                        for (int k = p; k < n; k++) {
                            sum += a[i][k] * v[k][j];
                        }
                        for (int k = p; k < n; k++) {
                            v[k][j] += sum * v[k][i];
                        }
                    }
                }
                for (int j = p; j < n; j++) {
                    v[i][j] = v[j][i] = 0;
                }
            }
            v[i][i] = 1;
            g = rv1[i];
            p = i;
        }

        /*
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                System.err.println("v(" + i + ", " + j + ") = " + v[i][j]);
            }
        }/*CMP*/
        for (int i = n - 1; i >= 0; i--) {
            p = i + 1;
            g = w[i];
            if (i < n) {
                for (int j = p; j < n; j++) {
                    a[i][j] = 0;
                }
            }
            if (g != 0) {
                g= 1 / g;
                if (i != n) {
                    for (int j = p; j < n; j++) {
                        sum = 0;
                        for (int k = p; k < m; k++) {
                            sum += a[k][i] * a[k][j];
                        }
                        f = (sum / a[i][i]) * g;
                        for (int k = i; k < m; k++) {
                            a[k][j] += f * a[k][i];
                        }
                    }
                }
                for (int j = i; j < m; j++) {
                    a[j][i] *= g;
                }
            } else {
                for (int j = i; j < m; j++) {
                    a[j][i] = 0;
                }
            }
            ++a[i][i];
        }
        /*
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                System.err.println("a(" + i + ", " + j + ") = " + a[i][j]);
            }
        }/*CMP*/
        for (int k = n - 1; k >= 0; k--) {
            for (int its = 1; its <= 30; its++) {
                /*System.err.println("its=" + its);/*CMP*/
                boolean flag = true;
                int q = 0;
                for (p = k; p >= 0; p--) {
                    q = p - 1;
                    /*System.err.println("rv1[" + p + "]=" + rv1[p]);/*CMP*/
                    if (Math.abs(rv1[p]) + norm == norm) {
                        flag = false;
                        break;
                    }
                    // Note: can't get here when q=-1 (p=0) because rv1[0]=0.
                    /*System.err.println("w[" + q + "]=" + w[q]);/*CMP*/
                    if (Math.abs(w[q]) + norm == norm) {
                        break;
                    }
                }
                if (flag) {
                    double c = 0;
                    double s = 1;
                    for (int i = p; i < k; i++) {
                        f = s * rv1[i];
                        if (Math.abs(f) + norm != norm) {
                            g = w[i];
                            //h = Math.hypot(f, g);
                            h = hypot(f, g);
                            w[i] = h;
                            h = 1 / h;
                            c = g * h;
                            s = -f * h;
                            for (int j = 0; j < m; j++) {
                                double y = a[j][q];
                                double z = a[j][i];
                                a[j][q] = y * c + z * s;
                                a[j][i] = z * c - y * s;
                            }
                        }
                    }
                }
                /*
                for (int i = 0; i < m; i++) {
                    for (int j = 0; j < n; j++) {
                        System.err.println("a(" + i + ", " + j + ") = " + a[i][j]);
                    }
                }/*CMP*/
                double z = w[k];
                if (p == k) {
                    if (z < 0) {
                        w[k] = -z;
                        for (int j = 0; j < n; j++) {
                            v[j][k] = -v[j][k];
                        }
                    }
                    break;
                }
                /*
                for (int j = 0; j < n; j++) {
                    //System.err.println("rv1(" + j + ") = " + rv1[j]);
                    System.err.println("w(" + j + ") = " + w[j]);
                }/*CMP*/
                if (its >= 30) {
                    System.err.println("SVDecompose: does not converge.");
                    return null;
                }
                double x = w[p];
                q = k - 1;
                double y = w[q];
                g = rv1[q];
                h = rv1[k];
                f = ((y - z) * (y + z) + (g - h) * (g + h)) / (2 * h * y);
                //g = Math.hypot(f, 1);
                g = hypot(f, 1);
                f= ((x - z) * (x + z) + h * ((y / (f + sign(g, f))) - h)) / x;
                /*System.err.println("f=" + f);/*CMP*/
                double c = 1;
                double s = 1;
                for (int j = p; j <= q; j++) {
                    /*System.err.println("--- j=" + j);/*CMP*/
                    int jj = j + 1;
                    g = rv1[jj];
                    y = w[jj];
                    h = s * g;
                    g = c * g;
                    //z = Math.hypot(f, h);
                    z = hypot(f, h);
                    rv1[j] = z;
                    c = f / z;
                    s = h / z;
                    f = x * c + g * s;
                    g = g * c - x * s;
                    h = y * s;
                    y *= c;
                    for (int i = 0; i < n; i++) {
                        x = v[i][j];
                        z = v[i][jj];
                        v[i][j] =x * c +z * s;
                        v[i][jj] =z * c -x * s;
                    }
                    //z = Math.hypot(f, h);
                    z = hypot(f, h);
                    w[j] = z;
                    if (z != 0) {
                        z = 1 / z;
                        c = f * z;
                        s = h * z;
                    }
                    f =(c * g) + (s * y);
                    x =(c * y) - (s * g);
                    for (int i = 0; i < m; i++) {
                        y = a[i][j];
                        z = a[i][jj];
                        /*System.err.println("y=" + y + ", z=" + z);/*CMP*/
                        a[i][j] = y * c +z * s;
                        a[i][jj] = z * c -y * s;
                    }
                }
                rv1[p] = 0;
                rv1[k] = f;
                w[k] = x;
                /*
                for (int i = 0; i < m; i++) {
                    for (int j = 0; j < n; j++) {
                        System.err.println("a(" + i + ", " + j + ") = " + a[i][j]);
                    }
                }/*CMP*/
            }
        }
        SVDecomposition ans = new SVDecomposition(a, w, v);
        return ans;
    }

    /**
     * Backsubstitute into the SVD of a matrix.  The SVD is a representation
     * of the matrix A.  If A*x = b, then x = A(inverse)*b, and we are
     * plugging the vector b into this equation to get the x vector that
     * produces b in the original equation.
     * @param svd The Singular Value Decomposition of the matrix A.
     * @param b The result vector to backsubstitute in.
     * @return The x vector that produces b.
     */
    public static double[] svdBacksub(SVDecomposition svd, double[] b) {
        double[][] u = svd.u;
        double[][] v = svd.v;
        double[] w = svd.w;
        int n = w.length;
        int m = u.length;
        double[] x = new double[n];
        double[] tmp = new double[n];

        for (int j = 0; j < n; j++) {
            double s = 0;
            if (w[j] != 0) {
                for (int i = 0; i < m; i++) {
                    s += u[i][j] * b[i];
                }
                s /= w[j];
            }
            tmp[j] = s;
        }
        for (int j = 0; j < n; j++) {
            double s = 0;
            for (int i = 0; i < n; i++) {
                s += v[j][i] * tmp[i];
            }
            x[j] = s;
        }
        return x;
    }

    /**
     * A container class that has the Singular Value Decomposition of
     * a real matrix stored in 3 arrays. The original matrix is obtained from
     * <pre>
     * A(i,j) = [sum over k of] W(k) * U(i,k) * V(j,k)
     * </pre>
     * where W represents an NxN diagonal matrix,
     * <br>V is an NxN orthogonal matrix (i.e., V(transpose) = V(inverse)),
     * <br>and U is an MxN orthonormal matrix (i.e., U(transpose) = U(inverse)),
     * <br>and M >= N.
     * <p>
     * So the matrix equation is: <b>A</b> =  <b>U</b> <b>W</b> <b>V</b>(t).
     */
    public static class SVDecomposition {
        
        public double[][] u;

        public double[] w;

        public double[][] v;

        /**
         * Create a SVDecomposition object consisting of the given arrays.
         * The dimensions of the arrays are not checked!  They must be:
         * <pre>
         * u[m][n]
         * w[n]
         * v[n][n] </pre>
         *  where m >= n.
         */
        public SVDecomposition(double[][] u, double[] w, double[][]v) {
            this.u = u;
            this.w = w;
            this.v = v;
        }

        /**
         * For testing: calculates the A(i,j) element of the original
         * matrix from the SVD.
         */
        public double recomposeValue(int i, int j) {
            int n = w.length;
            double value = 0;
            for (int k = 0; k < n; k++) {
                value += w[k] * u[i][k] * v[j][k];
            }
            return value;
        }
    }


    /**
     * The prototype for defining basis functions for fitting routines.
     * An instance of this class is passed to the fit routine.
     * Extentions of this class need to
     * <ol>
     * <li> Define the <tt>evaluateBasisFunctions(double[] x)</tt> method to
     * return a vector of basis function values for the input x vector.
     * <li> For multi-dimensional fits, call setXLength() in the constructor,
     * so the user
     * can query the dimension of the input vector.  (Not strictly
     * necessary if the user knows the dimension <i>a priori</i>).
     * <li> Optionally, call setYLength() in the constructor, so the user
     * can query how many basis function values to expect.
     * <li> Set any other information in the constructor that
     * <tt>evaluateBasisFunctions()</tt> will need to do its work.
     * </ol>
     * @see LinFit.PolyBasisFunctions
     */
    public static abstract class BasisFunctions {
        /**
         * The length of the independent variable vector.
         */
        private int m_nDimensions = 1;

        /**
         * The number of basis functions.
         */
        private int m_nFunctions = 0;

        /**
         * Returns the number of dimensions in the independent variable.
         */
        public int getXLength() {
            return m_nDimensions;
        }

        protected void setXLength(int len) {
            m_nDimensions = len;
        }

        /**
         * Returns the number of basis functions.
         */
        public int getYLength() {
            if (m_nFunctions == 0) {
                double[] x = new double[m_nDimensions];
                double[] y = evaluateBasisFunctions(x);
                m_nFunctions = y.length;
            }
            return m_nFunctions;
        }

        protected void setYLength(int len) {
            m_nFunctions = len;
        }

        /**
         * Convenience routine to use when the <b>x</b> vector is 1-dimensional.
         * If <tt>xarray</tt> has length 1 and <tt>xarray[0] = x</tt>, then
         * <tt>yarray=evaluateBasisFunctions(x)</tt> is equivalent to
         * <tt>yarray=evaluateBasisFunctions(xarray)</tt>.
         */
        public double[] evaluateBasisFunctions(double x) {
            double[] xx = new double[1];
            xx[0] = x;
            return evaluateBasisFunctions(xx);
        }

        /**
         * Evaluates all the basis functions at the point <b>x</b>.
         * Implementing classes need to define this.
         * @param x A vector specifying the evaluation point.
         * @return A vector containing y values of all the basis
         * functions at point <b>x</b>.
         */
        public abstract double[] evaluateBasisFunctions(double[] x);
    }


    /**
     * A polynomial fitting function.
     */
    public static class PolyBasisFunctions extends BasisFunctions {

        /**
         * Instantiate basis functions for a polynomial of given degree.
         * @param degree The degree of the polynomial.
         */
        public PolyBasisFunctions(int degree) {
            setYLength(degree + 1);
        }

        /**
         * Evaluate 1, x, x^2, ..., x^n for the given value of x.
         * @param xarray The first element of this array is the value of x.
         * @return A vector containing y values of all the basis
         * functions at point <b>x</b>.
         * @see LinFit.BasisFunctions#evaluateBasisFunctions(double).
         */
        public double[] evaluateBasisFunctions(double[] xarray) {
            int n = getYLength();
            double[] y = new double[n];
            y[0] = 1;
            for (int i = 1; i < n; i++) {
                y[i] = y[i - 1] * xarray[0];
            }
            return y;
        }
    }

    /**
     * A base class for returning the results of a fit to data.  This
     * class includes the fit coefficients and the chi-square of the
     * fit, which should both be available from all fitting routines.
     * The getCovariance() method defined here just returns null, and
     * should be overridden by those subclasses that can calculate it.
     */
    public static class FitResult {

        /** The fit vector. */
        private double[] m_a;

        /** The Chi-square of the fit. */
        protected double m_chisq = Double.NaN;

        /**
         * Construct a basic FitResult containing only the coefficients
         * of the fit.  Calling getChisq() on the resulting object will
         * return Double.NaN.
         * @param a The coefficients of the fit.
         */
        public FitResult(double[] a) {
            m_a = a;
        }

        /**
         * Construct a FitResult containing the coefficients and
         * the chi-square for the fit.
         * @param a The coefficients of the fit.
         * @param chisq The chi-square of the fit.
         */
        public FitResult(double[] a, double chisq) {
            m_a = a;
            m_chisq = chisq;
        }

        /**
         * Get the number of coefficients in the fit.
         */
        public int getNCoeffs() {
            return m_a.length;
        }

        /**
         * Get the coefficients of the fit.
         * @return An array containing the coefficient of each basis function.
         */
        public double[] getCoeffs() {
            return m_a;
        }

        /**
         * Get the goodness of fit.  If the standard deviations of the
         * data points was provided, this variable will be chi-square
         * distributed with n = nPoints - nCoeffs degrees of freedom.
         * If no standard deviations were provided, it is just the sum
         * of squared differences between the model and the data, and
         * may be used to estimate the variance.
         */
        public double getChisq() {
            return m_chisq;
        }

        /**
         * Returns null for the covariance matrix.  May be overridden
         * in extensions of the base class if they
         * can calculate the covariance.
         */
        public double[][] getCovariance() {
            return null;
        }
    }
    


    public static class SvdFitResult extends FitResult {

        /** The SVD used to get the fit. */
        private SVDecomposition m_svd;

        /**
         * Constructor used to return a FitResult from the SDV fitting routine.
         * @param a The coefficients of the fit.
         * @param chisq The chi-square of the fit.
         * @param svd The SVD used to calculate the fit.  This is needed if
         * covariances are to be calculated.
         */
        public SvdFitResult(double[] a, double chisq, SVDecomposition svd) {
            super(a, chisq);
            m_svd = svd;
        }

        /**
         * Calculate the covariance matrix for the fit coefficients.
         * (The diagonal elements give the variance in each coefficient.)
         * @return The (symmetric) covariance matrix.
         */
        public double[][] getCovariance() {
            int n = getNCoeffs();
            double[] wti = new double[n];
            double[][] covar = new double[n][n];
            double[] w = m_svd.w;
            double[][] v = m_svd.v;

            for (int i = 0; i < n; i++) {
                wti[i] = 0;
                if (w[i] != 0) {
                    wti[i] = 1 / (w[i] * w[i]);
                }
            }
            for (int i = 0; i < n; i++) {
                for (int j = 0; j <= i; j++) {
                    double sum = 0;
                    for (int k = 0; k < n; k++) {
                        sum += v[i][k] * v[j][k] * wti[k];
                    }
                    covar[j][i] = covar[i][j] = sum;
                }
            }
            return covar;
        }
    }
}
