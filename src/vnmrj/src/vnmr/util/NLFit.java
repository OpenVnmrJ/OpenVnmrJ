/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.io.IOException;
import java.util.Random;

/**
 * This class contains static methods for solving nonlinear
 * least-squares function minimization problems.  The user must supply
 * an object which is subclassed from Fit.BasisFunctions.  The interface
 * is the same as for linear fits, but the semantics is somewhat
 * different.  The data to be fit is contained in the object, typically,
 * an array of (x,y) values.
 * A function evaluation returns the difference between the data value
 * and a calculation based on the parameters.
 * <p>Example usage:<pre>
 *           // Define data and functional form
 *           double sqrt2o2 = 1 / Math.sqrt(2);
 *           double[] x = {0, sqrt2o2, 1, sqrt2o2, 0};
 *           double[] y = {-1, -sqrt2o2, 0, sqrt2o2, 1};
 *           CircleFunc func = new CircleFunc(x, y);
 *
 *           // Make a first guess, and call lmdif1 to optimize it
 *           double[] a = {0.7, 0.5, 0};
 *           NLFitResult result = lmdif1(func, a, 0);
 *           a = result.getCoeffs();
 * </pre>
 * <p>Adapted from the FORTRAN language MINPACK-1 routines from:
 * <br>Argonne National Laboratory. Minpack Project. March 1980.
 * <br>Burton S. Garbow, Kenneth E. Hillstrom, Jorge J. More.
 */
public class NLFit extends LinFit {

    /**
     * For testing the routines.
     */
    public static void main (String[] args) throws IOException {

        Random rand = new Random(); // For adding noise to tests

        final String[] choices = {
            "circle",
        };
        // NB: Keep these in sync with the above choices
        final int CIRCLE = 0;

        String choice = choices[0];
        if (args.length > 0) {
            choice = args[0];
        }
        if (choice.equalsIgnoreCase(choices[CIRCLE])) {
            double sqrt2o2 = 1 / Math.sqrt(2);
            double sigma = 0.1;
            final int npts = 5;
            final int ndats = 2 * npts;
            double[] r = new double[ndats];
            for (int i = 0; i < ndats; i++) {
                r[i] = rand.nextGaussian() * sigma;
            }
            double[] x = {0 + r[0],
                          sqrt2o2 + r[1],
                          1 + r[2],
                          sqrt2o2 + r[3],
                          0 + r[4]};
            double[] y = {-1 + r[5],
                          -sqrt2o2 + r[6],
                          0 + r[7],
                          sqrt2o2 + r[8],
                          1 + r[9]};
            CircleFunc func = new CircleFunc(x, y);
            //double[] a = {0, 1, 0.5};
            //double[] a = {0.7, 0.5, 0};
            double[] a = func.getEstimatedParameters();
            NLFitResult result = lmdif1(func, a, 1e-3);
            a = result.getCoeffs();
            System.err.println("Solution for " + choice + ":");
            System.err.println("x=" + a[0] + ", y=" + a[1]
                               + ", r=" + a[2]);
            System.err.println("Result code: " + result.getInfo());
            System.err.println("Number of evaluations: "
                               + result.getNumberEvaluations());
            System.err.println("Sigma: " + Math.sqrt(result.getChisq()));
            
            double[] f = func.evaluateBasisFunctions(a);
            int m = f.length;
            System.err.println("Function values: ");
            for (int i = 0; i < m; i++) {
                System.out.println(f[i] + " ");
            }
        }
    }


    /**
     * Get precision parameters for double precision arithmetic.
     * @param i The type of parameter to return:
     * <br> 1: The precision, 2**(1-nbits).
     * <br> 2: The smallest accurately representable magnitude.
     * <br> 3: The largest accurately representable magnitude.
     * <p>
    */
    public static double dpmpar(int i) {
        /*
         * These are the constants for double precision IEEE arithmetic.
         */
        switch (i) {
        case 1: return 2.22044604926e-16;
        case 2: return 2.22507385852e-308;
        case 3: return 1.79769313485e+308;
        }
        return Double.NaN;
    }

    /**
     * Given an n-vector x, this function calculates the
     * euclidean norm of x.
     * <p>
     * The euclidean norm is computed by accumulating the sum of
     * squares in three different sums. The sums of squares for the
     * small and large components are scaled so that no overflows
     * occur. Non-destructive underflows are permitted. Underflows
     * and overflows do not occur in the computation of the unscaled
     * sum of squares for the intermediate components.
     * The definitions of small, intermediate and large components
     * depend on two constants, rdwarf and rgiant. The main
     * restrictions on these constants are that rdwarf**2 not
     * underflow and rgiant**2 not overflow. The constants
     * given here are suitable for every known computer.
     *
     * @param x The input array.
     * @return The Euclidean norm, the RMS element value.
     */
    public static double enorm(double[] x) {
        double agiant, floatn, s1, s2, s3, xabs, x1max, x3max;
        final double rdwarf = 3.834e-20;
        final double rgiant = 1.304e19;

        s1 = 0;
        s2 = 0;
        s3 = 0;
        x1max = 0;
        x3max = 0;
        int n = x.length;
        floatn = n;
        agiant = rgiant / floatn;
        for (int i = 0; i < n; i++) {
            xabs = Math.abs(x[i]);
            if (xabs > rdwarf && xabs < agiant) {
                //
                // sum for intermediate components.
                //
                s2 = s2 + xabs * xabs;
            } else if (xabs <= rdwarf) {
                //
                // sum for small components.
                //
                if (xabs > x3max) {
                    double tmp = x3max / xabs;
                    s3 = 1 + s3 * tmp * tmp;
                    x3max = xabs;
                } else {
                    if (xabs != 0) {
                        double tmp = xabs / x3max;
                        s3 = s3 + tmp * tmp;
                    }
                }
            } else {
                //
                // sum for large components.
                //
                if (xabs > x1max) {
                    double tmp = x1max / xabs;
                    s1 = 1 + s1 * tmp * tmp;
                    x1max = xabs;
                } else {
                    double tmp = xabs / x1max;
                    s1 = s1 + tmp * tmp;
                }
            }
        }
        //
        // calculation of norm.
        //
        double enorm;
        if (s1 != 0) {
            enorm = x1max * Math.sqrt(s1 + (s2 / x1max) / x1max);
        } else if (s2 == 0) {
            enorm = x3max * Math.sqrt(s3);
        } else if (s2 >= x3max) {
            enorm = Math.sqrt(s2 * (1 + (x3max / s2) * (x3max * s3)));
        } else {
            enorm = Math.sqrt(x3max * ((s2 / x3max) + (x3max * s3)));
        }
        return enorm;
    }

    /**
     * Computes a forward-difference approximation
     * to the m by n Jacobian matrix associated with a specified
     * problem of m functions in n variables.
     * @param     fcn  The the user-supplied object which
     *     calculates the functions. 
     * @param   x  Input array of length n.
     * @param   fvec  Input array of length m  (m >= n) which must contain the
     *     functions evaluated at x.
     * @param   ldfjac  A positive integer input variable not less than m
     *     which specifies the leading dimension of the array fjac. I.e., the
     *     number of rows in fjac.
     * @param   epsfcn  An input variable used in determining a suitable
     *     step length for the forward-difference approximation. This
     *     approximation assumes that the relative errors in the
     *     functions are of the order of epsfcn. If epsfcn is less
     *     than the machine precision, it is assumed that the relative
     *     errors in the functions are of the order of the machine
     *     precision.
     * @return   Output m by n array which contains the
     *     approximation to the Jacobian matrix evaluated at x.
     */
    public static double[][] fdjac2(BasisFunctions fcn,
                                    double[] x,
                                    double[] fvec,
                                    int ldfjac,
                                    double epsfcn) {
          int n = x.length;
          int m = fvec.length;
          double[][] fjac = new double[m][n];

          //
          // epsmch is the machine precision.
          //
          double epsmch = dpmpar(1);
          double eps = Math.sqrt(Math.max(epsfcn, epsmch));

          for (int j = 0; j < n; j++) {
              double temp = x[j];
              double h = eps * Math.abs(temp);
              if (h == 0) {
                  h = eps;
              }
              x[j] = temp + h;
              double[] wa  = fcn.evaluateBasisFunctions(x);
              if (wa == null) {
                  return null;
              }
              x[j] = temp;
              for (int i = 0; i < m; i++) {
                  fjac[i][j] = (wa[i] - fvec[i]) / h;
              }
          }
          return fjac;
      }


    /**
     * Minimize the sum of the squares of
     * m nonlinear functions in n variables by a modification of
     * the Levenberg-Marquardt algorithm. The user must provide a
     * BasisFunctions object which calculates the functions. The Jacobian is
     * then calculated by a forward-difference approximation.
     *
     * @param     fcn  The the user-supplied object which
     *     calculates the m function values. 
     *
     * @param  xguess  An input array of length n containing
     *     an initial estimate of the solution vector.
     *
     * @param  ftol  A nonnegative input variable. Termination
     *     occurs when both the actual and predicted relative
     *     reductions in the sum of squares are at most ftol.
     *     Therefore, ftol measures the relative error desired
     *     in the sum of squares.
     *
     * @param  xtol  A nonnegative input variable. Termination
     *     occurs when the relative error between two consecutive
     *     iterates is at most xtol. Therefore, xtol measures the
     *     relative error desired in the approximate solution.
     *
     * @param  gtol  A nonnegative input variable. Termination
     *     occurs when the cosine of the angle between fvec and
     *     any column of the Jacobian is at most gtol in absolute
     *     value. Therefore, gtol measures the orthogonality
     *     desired between the function vector and the columns
     *     of the Jacobian.
     *
     * @param  maxfev  A positive integer input variable. Termination
     *     occurs when the number of calls to fcn is at least
     *     maxfev by the end of an iteration.
     *
     * @param  epsfcn  An input variable used in determining a suitable
     *     step length for the forward-difference approximation. This
     *     approximation assumes that the relative errors in the
     *     functions are of the order of epsfcn. If epsfcn is less
     *     than the machine precision, it is assumed that the relative
     *     errors in the functions are of the order of the machine
     *     precision.
     *
     * @param  diag  An array of length n. Contains positive entries
     *     that serve as
     *     multiplicative scale factors for the variables.
     *     If diag is null, the values are calculated internally.
     *
     * @param  factor  A positive input variable used in determining the
     *     initial step bound. This bound is set to the product of
     *     factor and the euclidean norm of diag*x if nonzero, or else
     *     to factor itself. In most cases factor should lie in the
     *     interval (0.1, 100).  A generally recommended value is 100.
     *
     * @param  nprint  An integer input variable that enables controlled
     *     printing of iterates if it is positive. In this case,
     *     fcn is called and x and fvec are printed at the beginning of the first
     *     iteration and every nprint iterations thereafter and
     *     immediately prior to return.
     *     If nprint is not positive, no printing occurs.
     *
     * @param  ldfjac  A positive integer input variable not less than m
     *     which specifies the leading dimension of the array fjac.
     *
     * @return The final estimate of the solution vector, the final
     *     function evaluations using this vector, and some
     *     intermediate results.
     *
     * @see NLFit.NLFitResult
    */

    public static NLFitResult lmdif(BasisFunctions fcn,
                                    double[] xguess,
                                    double ftol,
                                    double xtol,
                                    double gtol,
                                    int maxfev,
                                    double epsfcn,
                                    double[] diag,
                                    double factor,
                                    int nprint,
                                    int ldfjac) {
        int iter = 0;
        double actred,dirder,epsmch,fnorm1,gnorm,
                pnorm,prered,ratio,
                sum,temp,temp1,temp2;
        double fnorm = 0;
        double xnorm = 0;
        double delta = 0;
        double par = 0;
        
        int n = xguess.length;
        int m = fcn.getYLength();
        double[] wa1  = new double[n];
        double[] wa2  = new double[n];
        double[] wa3  = new double[n];
        double[] wa4  = new double[m];

        // Stuff for output
        double[] fvec = null;   // The functions evaluated at x
        int info = 0;           // result status
        int nfev = 0;           // # calls to fcn
        double[][] fjac = null; // Jacobian in upper triangular n x n part
        int[] ipvt = null;      //
        double[] qtf = new double[n];    //
        double[] x = new double[n];
        for (int i = 0; i < n; i++) {
            x[i] = xguess[i];
        }

        // epsmch is the machine precision.
        epsmch = dpmpar(1);
        //
        int iflag = 0;

        // check the input parameters for errors.
        boolean quit = (n <= 0 || m < n || ldfjac < m
                         || ftol < 0 || xtol < 0 || gtol < 0
                         || maxfev <= 0 || factor <= 0); // go to 300
        int mode;
        if (diag == null) {
            mode = 1;
            diag = new double[n];
        } else {
            mode = 2;
            for (int j = 0; j < n; j++) {
                if (diag[j] <= 0) {
                    quit = true;
                    break;
                }
            }
        }

        if (!quit) {
            // evaluate the function at the starting point
            // and calculate its norm.
            iflag = 1;
            fvec = fcn.evaluateBasisFunctions(x);
            quit = (fvec == null);
            nfev = 1;
        }
        if (!quit) {
            fnorm = enorm(fvec);

            // initialize levenberg-marquardt parameter and iteration counter.
            par = 0;
            iter = 1;
        }

        //     beginning of the outer loop.
        while (!quit) {

            // calculate the jacobian matrix.
            iflag = 2;
            fjac = fdjac2(fcn, x, fvec, ldfjac, epsfcn);
            nfev = nfev + n;
            if (fjac == null) {
                quit = true;
                break;
            }

            // if requested, call fcn to enable printing of iterates.
            if (nprint > 0) {
                iflag = 0;
                if ((iter - 1) % nprint == 0) {
                    fvec = fcn.evaluateBasisFunctions(x);
                    // TODO: print function values here?
                    if (fvec == null) {
                        quit = true;
                        break;
                    }
                }
            }

            // compute the qr factorization of the jacobian.
            ipvt = new int[n];
            qrfac(fjac, ipvt, wa1, wa2);

            // on the first iteration and if mode is 1, scale according
            // to the norms of the columns of the initial jacobian.
            if (iter == 1) {
                if (mode != 2) {
                    for (int j = 0; j < n; j++) {
                        diag[j] = wa2[j];
                        if (wa2[j] == 0) {
                            diag[j] = 1;
                        }
                    }
                }

                // on the first iteration, calculate the norm of the scaled x
                // and initialize the step bound delta.
                for (int j = 0; j < n; j++) {
                    wa3[j] = diag[j] * x[j];
                }
                xnorm = enorm(wa3);
                delta = factor * xnorm;
                if (delta == 0) {
                    delta = factor;
                }
            }

            // form (q transpose)*fvec and store the first n components in
            // qtf.
            for (int i = 0; i < m; i++) {
                wa4[i] = fvec[i];
            }
            for (int j = 0; j < n; j++) {
                if (fjac[j][j] != 0) {
                    sum = 0;
                    for (int i = j; i < m; i++) {
                        sum = sum + fjac[i][j] * wa4[i];
                    }
                    temp = -sum / fjac[j][j];
                    for (int i = j; i < m; i++) {
                        wa4[i] += fjac[i][j] * temp;
                    }
                }
                fjac[j][j] = wa1[j];
                qtf[j] = wa4[j];
            }

            // compute the norm of the scaled gradient.
            gnorm = 0;
            if (fnorm != 0) {
                for (int j = 0; j < n; j++) {
                    int k = ipvt[j];
                    if (wa2[k] != 0) {
                        sum = 0;
                        for (int i = 0; i <= j; i++) {
                            sum += fjac[i][j] * (qtf[i] / fnorm);
                        }
                        gnorm = Math.max(gnorm, Math.abs(sum / wa2[k]));
                    }
                }
            }

            // test for convergence of the gradient norm.
            if (gnorm <= gtol) {
                info = 4;
                quit = true;
                break;
            }

            // rescale if necessary.
            if (mode != 2) {
                for (int j = 0; j < n; j++) {
                    diag[j] = Math.max(diag[j],wa2[j]);
                }
            }

            // beginning of the inner loop.
            do {
                // determine the Levenberg-Marquardt parameter.
                par = lmpar(fjac, ipvt, diag, qtf, delta, par, wa1, wa2);

                // store the direction p and x + p. calculate the norm of p.
                for (int j = 0; j < n; j++) {
                    wa1[j] = -wa1[j];
                    wa2[j] = x[j] + wa1[j];
                    wa3[j] = diag[j]*wa1[j];
                }
                pnorm = enorm(wa3);

                // on the first iteration, adjust the initial step bound.
                if (iter == 1) {
                    delta = Math.min(delta, pnorm);
                }

                // evaluate the function at x + p and calculate its norm.
                iflag = 1;
                wa4 = fcn.evaluateBasisFunctions(wa2);
                nfev = nfev + 1;
                if (wa4 == null) {
                    quit = true;
                    break;
                }
                fnorm1 = enorm(wa4);

                // compute the scaled actual reduction.
                actred = -1;
                if (0.1 * fnorm1 < fnorm) {
                    double tmp = fnorm1 / fnorm;
                    actred = 1 - tmp * tmp;
                }

                // compute the scaled predicted reduction and
                // the scaled directional derivative.
                for (int j = 0; j < n; j++) {
                    wa3[j] = 0;
                    int k = ipvt[j];
                    temp = wa1[k];
                    for (int i = 0; i <= j; i++) {
                        wa3[i] = wa3[i] + fjac[i][j] * temp;
                    }
                }
                temp1 = enorm(wa3) / fnorm;
                temp2 = (Math.sqrt(par) * pnorm) / fnorm;
                prered = temp1 * temp1 + temp2 * temp2 / 0.5;
                dirder = -(temp1 * temp1 + temp2 * temp2);

                // compute the ratio of the actual to the predicted
                // reduction.
                ratio = 0;
                if (prered != 0) {
                    ratio = actred / prered;
                }

                // update the step bound.
                if (ratio <= 0.25) {
                    if (actred >= 0) {
                        temp = 0.5;
                    } else {
                        temp = 0.5 * dirder / (dirder + 0.5 * actred);
                    }
                    if ((0.1 * fnorm1 >= fnorm) || temp < 0.1) {
                        temp = 0.1;
                    }
                    delta = temp * Math.min(delta, pnorm / 0.1);
                    par = par / temp;
                } else {
                    if (par == 0 || ratio >= 0.75) {
                        delta = pnorm / 0.5;
                        par = 0.5 * par;
                    }
                }

                // test for successful iteration.
                if (ratio >= 1e-4) {

                    // successful iteration. update x, fvec, and their norms.
                    for (int j = 0; j < n; j++) {
                        x[j] = wa2[j];
                        wa2[j] = diag[j] * x[j];
                    }
                    for (int i = 0; i < m; i++) {
                        fvec[i] = wa4[i];
                    }

                    xnorm = enorm(wa2);
                    fnorm = fnorm1;
                    iter++;
                }

                // tests for convergence.
                if (Math.abs(actred) <= ftol
                    && prered <= ftol
                    && 0.5*ratio <= 1)
                {
                    info = 1;
                }
                if (delta <= xtol*xnorm) {
                    info = 2;
                }
                if (Math.abs(actred) <= ftol
                    && prered <= ftol
                    && 0.5*ratio <= 1
                    && info == 2)
                {
                    info = 3;
                }
                if (info != 0) {
                    quit = true;
                    break;
                }

                // tests for termination and stringent tolerances.
                if (nfev >= maxfev) {
                    info = 5;
                }
                if (Math.abs(actred) <= epsmch
                    && prered <= epsmch
                    && 0.5*ratio <= 1)
                {
                    info = 6;
                }
                if (delta <= epsmch*xnorm) {
                    info = 7;
                }
                if (gnorm <= epsmch) {
                    info = 8;
                }
                if (info != 0) {
                    quit = true;
                    break;
                }
            } while (ratio < 1e-4);
        }

        // termination, either normal or user imposed.
        if (iflag < 0) {
            info = iflag;
        }
        iflag = 0;
        if (nprint > 0) {
            fvec = fcn.evaluateBasisFunctions(x);
            // TODO: print function values here?
        }
        NLFitResult result = new NLFitResult(x, fvec, info, nfev, fjac,
                                             ipvt, qtf);
        return result;
    }


    /**
     * Minimize the sum of the squares of
     * m nonlinear functions in n variables by a modification of the
     * levenberg-marquardt algorithm. This is done by using the more
     * general least-squares solver lmdif. The user must provide a
     * subroutine which calculates the functions. The jacobian is
     * then calculated by a forward-difference approximation.
     *
     * @param     fcn  The the user-supplied object which
     *     calculates the function values. 
     *
     * @param  x  An input array of length n containing
     *     an initial estimate of the solution vector.
     *
     * @param tol A nonnegative input variable.  Termination occurs
     *     when the algorithm estimates either that the relative
     *     error in the sum of squares is at most tol or that
     *     the relative error between x and the solution is at
     *     most tol.  Zero is a reasonable value if you want to
     *     go to the numerical limits of the computation.
     *
     * @return The final estimate of the solution vector.
     */
    public static NLFitResult lmdif1(BasisFunctions fcn,
                                     double[] x, double tol) {

        int n = x.length;
        int m = fcn.getYLength();

        // check the input parameters for errors.
        if (n <= 0 || m < n || tol < 0) {
            return null;
        }

        // call lmdif.
        int maxfev = 200 * (n + 1);
        double factor = 100;
        double ftol = tol;
        double xtol = tol;
        double gtol = 0;
        double epsfcn = 0;
        int nprint = 0;
        NLFitResult result =
                lmdif(fcn, x, ftol, xtol, gtol, maxfev, epsfcn, null,
                      factor, nprint, m);
        if (result.getInfo() == 8) {
            result.setInfo(4);
        }
        return result;
    }


    /**
     * Determine a value for the Levenberg-Marquardt parameter.
     * Given an m by n matrix a, an n by n nonsingular diagonal
     * matrix d, an m-vector b, and a positive number delta,
     * the problem is to determine a value for the parameter
     * par such that if x solves the system
     *<pre>
     *       a*x = b ,     sqrt(par)*d*x = 0 ,
     *</pre>
     * in the least squares sense, and dxnorm is the euclidean
     * norm of d*x, then either par is 0 and
     *<pre>
     *       (dxnorm-delta) <= 0.1*delta ,
     *</pre>
     * or par is positive and
     *<pre>
     *       abs(dxnorm-delta) <= 0.1*delta .
     *</pre>
     * This subroutine completes the solution of the problem
     * if it is provided with the necessary information from the
     * qr factorization, with column pivoting, of a. That is, if
     * a*p = q*r, where p is a permutation matrix, q has orthogonal
     * columns, and r is an upper triangular matrix with diagonal
     * elements of nonincreasing magnitude, then lmpar expects
     * the full upper triangle of r, the permutation matrix p,
     * and the first n components of (q transpose)*b. On output
     * lmpar also provides an upper triangular matrix s such that
     *<pre>
     *        t   t                   t
     *       p *(a *a + par*d*d)*p = s *s .
     *</pre>
     * s is employed within lmpar and may be of separate interest.
     *
     * Only a few iterations are generally needed for convergence
     * of the algorithm. If, however, the limit of 10 iterations
     * is reached, then the output par will contain the best
     * value obtained so far.
     *
     * @param  r  An n by n array used for both input and output.
     *     On input the full upper triangle
     *     must contain the full upper triangle of the matrix r.
     *     On output the full upper triangle is unaltered, and the
     *     strict lower triangle contains the strict upper triangle
     *     (transposed) of the upper triangular matrix s.<br>
     *
     * @param  ipvt  An integer input array of length n which defines the
     *     permutation matrix p such that a*p = q*r. Column j of p
     *     is column ipvt(j) of the identity matrix.
     *
     * @param  diag An input array of length n which must contain the
     *     diagonal elements of the matrix d.
     *
     * @param  qtb  An input array of length n which must contain the first
     *     n elements of the vector (q transpose)*b.
     *
     * @param  delta  A positive input variable which specifies an upper
     *     bound on the Euclidean norm of d*x.
     *
     * @param  par  A nonnegative input containing an
     *     initial estimate of the Levenberg-Marquardt parameter.
     *
     * @param  x  An output array of length n which contains the least
     *     squares solution of the system a*x = b, sqrt(par)*d*x = 0,
     *     for the output par.
     *
     * @param  sdiag  An output array of length n which contains the
     *     diagonal elements of the upper triangular matrix s.
     *
     * @return The final estimate of the Levenberg-Marquardt parameter.
     */
    public static double lmpar(double[][] r,
                               int[] ipvt,
                               double[] diag,
                               double[] qtb,
                               double delta,
                               double par,
                               double[] x, // Output
                               double[] sdiag // Output
                               ) {

        int n = r[0].length;
        double[] wa1 = new double[n];
        double[] wa2 = new double[n];
        int iter, jm1;
        double  dxnorm, fp, gnorm, parc, parl, paru, sum, temp;

        // dwarf is the smallest positive magnitude.
        double dwarf = dpmpar(2);

        // compute and store in x the gauss-newton direction. if the
        // jacobian is rank-deficient, obtain a least squares solution.
        int nsing = n;
        for (int j = 0; j < n; j++) {
            wa1[j] = qtb[j];
            if (r[j][j] == 0 && nsing == n) {
                nsing = j - 1;
            }
            if (nsing < n) {
                wa1[j] = 0;
            }
        }
        if (nsing >= 1) {
            for (int j = nsing - 1; j >= 0; j--) {
                wa1[j] = wa1[j] / r[j][j];
                temp = wa1[j];
                jm1 = j - 1;
                if (jm1 >= 1) {
                    for (int i = 0; i < jm1; i++) {
                        wa1[i] = wa1[i] - r[i][j] * temp;
                    }
                }
            }
        }
        for (int j = 0; j < n; j++) {
            x[ipvt[j]] = wa1[j];
        }

        // initialize the iteration counter.
        // evaluate the function at the origin, and test
        // for acceptance of the gauss-newton direction.
        iter = 0;
        for (int j = 0; j < n; j++) {
            wa2[j] = diag[j] * x[j];
        }
        dxnorm = enorm(wa2);
        fp = dxnorm - delta;

        if (fp > 0.1 * delta) {
            // if the jacobian is not rank deficient, the newton
            // step provides a lower bound, parl, for the zero of
            // the function. otherwise set this bound to zero.
            parl = 0;
            if (nsing >= n) {
                for (int j = 0; j < n; j++) {
                    int k = ipvt[j];
                    wa1[j] = diag[k] * (wa2[k] / dxnorm);
                }
                for (int j = 0; j < n; j++) {
                    sum = 0;
                    jm1 = j - 1;
                    for (int i = 0; i < jm1; i++) {
                        sum += r[i][j] * wa1[i];
                    }
                    wa1[j] = (wa1[j] - sum) /r[j][j];
                }
                temp = enorm(wa1);
                parl = ((fp / delta) / temp) / temp;
            }

            // calculate an upper bound, paru, for the zero of the function.
            for (int j = 0; j < n; j++) {
                sum = 0;
                for (int i = 0; i <= j; i++) {
                    sum += r[i][j] * qtb[i];
                }
                wa1[j] = sum / diag[ipvt[j]];
            }

            gnorm = enorm(wa1);
            paru = gnorm / delta;
            if (paru == 0) {
                paru = dwarf / Math.min(delta, 0.1);
            }

            // if the input par lies outside of the interval (parl,paru),
            // set par to the closer endpoint.
            par = Math.max(par, parl);
            par = Math.min(par, paru);
            if (par == 0) {
                par = gnorm / dxnorm;
            }

            // beginning of an iteration.
            while (true) {
                iter++;

                // evaluate the function at the current value of par.
                if (par == 0) {
                    par = Math.max(dwarf, 1e-3 * paru);
                }
                temp = Math.sqrt(par);
                for (int j = 0; j < n; j++) {
                    wa1[j] = temp * diag[j];
                }
                qrsolv(r, ipvt, wa1, qtb, x, sdiag);
                for (int j = 0; j < n; j++) {
                    wa2[j] = diag[j] * x[j];
                }
                dxnorm = enorm(wa2);
                temp = fp;
                fp = dxnorm - delta;

                // If the function is small enough, accept the current value
                // of par. Also test for the exceptional cases where parl
                // is zero or the number of iterations has reached 10.
                if ((Math.abs(fp) <= 0.1 * delta)
                    || (parl == 0 && fp <= temp && temp < 0)
                    || (iter == 10))
                {
                    break;
                }
                // compute the newton correction.
                for (int j = 0; j < n; j++) {
                    int k = ipvt[j];
                    wa1[j] = diag[k] * (wa2[k] / dxnorm);
                }
                for (int j = 0; j < n; j++) {
                    wa1[j] = wa1[j] / sdiag[j];
                    temp = wa1[j];
                    for (int i = j + 1; i < n; i++) {
                        wa1[i] -= r[i][j] * temp;
                    }
                }
                temp = enorm(wa1);
                parc = ((fp / delta) / temp) / temp;

                // depending on the sign of the function, update parl or paru.
                if (fp > 0) {
                    parl = Math.max(parl,par);
                } else if (fp < 0) {
                    paru = Math.min(paru,par);
                }

                // compute an improved estimate for par.
                par = Math.max(parl, par + parc);
            } // end of an iteration.
        } //  220 continue

        // termination.
        if (iter == 0) {
            par = 0;
        }
        return par;
    }


    /**
     * Compute a QR factorization of a given matrix.
     * Uses Householder transformations with column
     * pivoting (optional) to compute a QR factorization of the
     * m by n matrix a. That is, qrfac determines an orthogonal
     * matrix q, a permutation matrix p, and an upper trapezoidal
     * matrix r with diagonal elements of nonincreasing magnitude,
     * such that a*p = q*r. The Householder transformation for
     * column k, k = 1,2,...,min(m,n), is of the form
     *<pre>
     *                       t
     *       i - (1/u(k))*u*u
     *</pre>
     * where u has zeros in the first k-1 positions. The form of
     * this transformation and the method of pivoting first
     * appeared in the corresponding linpack subroutine.
     *
     * @param  a  An m by n array used for input and output.
     *     On input <i>a</i> contains the matrix for
     *     which the QR factorization is to be computed. On output
     *     the strict upper trapezoidal part of a contains the strict
     *     upper trapezoidal part of r, and the lower trapezoidal
     *     part of a contains a factored form of q (the non-trivial
     *     elements of the u vectors described above).
     *
     * @param  ipvt  An integer output array. If non-null column
     *     pivoting is enforced and ipvt
     *     defines the permutation matrix p such that a*p = q*r.
     *     Column j of p is column ipvt(j) of the identity matrix.
     *     If ipvt is null, no column pivoting is done.
     *
     * @param  rdiag  An output array of length n which contains the
     *     diagonal elements of r.
     *
     * @param  acnorm  An output array of length n which contains the
     *     norms of the corresponding columns of the input matrix a.
     *     If this information is not needed, then acnorm can coincide
     *     with rdiag.
     */
    public static void  qrfac(double[][] a,
                              int[] ipvt,
                              double[] rdiag,
                              double[] acnorm) {

        boolean pivot = (ipvt != null);
        int m = a.length;
        int n = a[0].length;
        double[] wa = new double[n];
        double ajnorm, epsmch, sum, temp;

        // epsmch is the machine precision.
        epsmch = dpmpar(1);

        // compute the initial column norms and initialize several arrays.
        double[] tmpm = new double[m];
        for (int j = 0; j < n; j++) {
            for (int i = 0; i < m; i++) {
                tmpm[i] = a[i][j];
            }
            acnorm[j] = enorm(tmpm);
            rdiag[j] = acnorm[j];
            wa[j] = rdiag[j];
            if (pivot) {
                ipvt[j] = j;
            }
        }

        // reduce a to r with householder transformations.
        int minmn = Math.min(m, n);
        for (int j = 0; j < minmn; j++) { //do 110 j = 1, minmn
            if (pivot) { //go to 40

                // bring the column of largest norm into the pivot position.
                int kmax = j;
                for (int k = j; k < n; k++) { //do 20 k = j, n
                    if (rdiag[k] > rdiag[kmax]) {
                        kmax = k;
                    }
                }
                if (kmax != j) { //go to 40
                    for (int i = 0; i < m; i++) { // do 30 i = 1, m
                        temp = a[i][j];
                        a[i][j] = a[i][kmax];
                        a[i][kmax] = temp;
                    }
                    rdiag[kmax] = rdiag[j];
                    wa[kmax] = wa[j];
                    int k = ipvt[j];
                    ipvt[j] = ipvt[kmax];
                    ipvt[kmax] = k;
                }
            }

            // compute the householder transformation to reduce the
            // j-th column of a to a multiple of the j-th unit vector.
            double[] tmpmj = new double[m - j];
            for (int i = 0; i < m - j; i++) {
                tmpmj[i] = a[j+i][j];
            }
            ajnorm = enorm(tmpmj);
            if (ajnorm != 0) { //go to 100
                if (a[j][j] < 0) {
                    ajnorm = -ajnorm;
                }
                for (int i = j; i < m; i++) {
                    a[i][j] = a[i][j] / ajnorm;
                }
                a[j][j]++;

                // apply the transformation to the remaining columns
                // and update the norms.
                int jp1 = j + 1;
                //if (n < jp1) go to 100
                for (int k = jp1; k < n; k++) { //do 90 k = jp1, n
                    sum = 0;
                    for (int i = j; i < m; i++) {
                        sum = sum + a[i][j] * a[i][k];
                    }
                    temp = sum / a[j][j];
                    for (int i = j; i < m; i++) {
                        a[i][k] -= temp * a[i][j];
                    }
                    if (pivot || rdiag[k] != 0) { // go to 80
                        temp = a[j][k] / rdiag[k];
                        rdiag[k] *= Math.sqrt(Math.max(0, 1 - temp * temp));
                        temp = rdiag[k] / wa[k];
                        if (0.05 * temp * temp <= epsmch) { // go to 80
                            for (int i = 0; i < m - j; i++) {
                                tmpmj[i] = a[j+i][k];
                            }
                            rdiag[k] = enorm(tmpmj);
                            wa[k] = rdiag[k];
                        }
                    }
                }
            }
            rdiag[j] = -ajnorm;
        }
    }

    /**
     * Solve a system of equations given the QR decomposition.
     * Given an m by n matrix a, an n by n diagonal matrix d,
     * and an m-vector b, the problem is to determine an x which
     * solves the system
     *<pre>
     *       a*x = b ,     d*x = 0 ,
     *</pre>
     * in the least squares sense.
     *<p>
     * This subroutine completes the solution of the problem
     * if it is provided with the necessary information from the
     * qr factorization, with column pivoting, of a. That is, if
     * a*p = q*r, where p is a permutation matrix, q has orthogonal
     * columns, and r is an upper triangular matrix with diagonal
     * elements of nonincreasing magnitude, then qrsolv expects
     * the full upper triangle of r, the permutation matrix p,
     * and the first n components of (q transpose)*b. The system
     * a*x = b, d*x = 0, is then equivalent to
     *<pre>
     *              t       t
     *       r*z = q *b ,  p *d*p*z = 0 ,
     *</pre>
     * where x = p*z. If this system does not have full rank,
     * then a least squares solution is obtained. On output qrsolv
     * also provides an upper triangular matrix s such that
     *<pre>
     *        t   t               t
     *       p *(a *a + d*d)*p = s *s .
     *</pre>
     * s is computed within qrsolv and may be of separate interest.
     *
     * @param  r  An n by n array used for both input and output.
     *     On input the full upper triangle
     *     must contain the full upper triangle of the matrix r.
     *     On output the full upper triangle is unaltered, and the
     *     strict lower triangle contains the strict upper triangle
     *     (transposed) of the upper triangular matrix s.
     *
     * @param  ipvt  An integer input array of length n which defines the
     *     permutation matrix p such that a*p = q*r. Column j of p
     *     is column ipvt(j) of the identity matrix.
     *
     * @param  diag  An input array of length n which must contain the
     *     diagonal elements of the matrix d.
     *
     * @param  qtb  An input array of length n which must contain the first
     *     n elements of the vector (q transpose)*b.
     *
     * @param  x  An output array of length n which contains the least
     *     squares solution of the system a*x = b, d*x = 0.
     *
     * @param  sdiag  An output array of length n which contains the
     *     diagonal elements of the upper triangular matrix s.
     */
    public static void qrsolv(double[][] r, // Input / Output
                          int[] ipvt,
                          double[] diag,
                          double[] qtb,
                          double[]x, // Output
                          double[] sdiag // Output
                          ) {
        int n = ipvt.length;
        double[] wa = new double[n];
        int nsing;
        double cos, cotan, qtbpj, sin, sum, tan, temp;

        // copy r and (q transpose)*b to preserve input and initialize s.
        // in particular, save the diagonal elements of r in x.
        for (int j = 0; j < n; j++) {
            for (int i = j; i < n; i++) {
                r[i][j] = r[j][i];
            }
            x[j] = r[j][j];
            wa[j] = qtb[j];
        }

        // eliminate the diagonal matrix d using a givens rotation.
        for (int j = 0; j < n; j++) {
            // prepare the row of d to be eliminated, locating the
            // diagonal element using p from the qr factorization.
            int ip = ipvt[j];
            if (diag[ip] != 0) {
                for (int k = j; k < n; k++) {
                    sdiag[k] = 0;
                }
                sdiag[j] = diag[ip];

                // the transformations to eliminate the row of d
                // modify only a single element of (q transpose)*b
                // beyond the first n, which is initially zero.
                qtbpj = 0;
                for (int k = j; k < n; k++) {
                    // determine a givens rotation which eliminates the
                    // appropriate element in the current row of d.
                    if (sdiag[k] != 0) {
                        if (Math.abs(r[k][k]) < Math.abs(sdiag[k])) {
                            cotan = r[k][k] / sdiag[k];
                            sin = 0.5 / Math.sqrt(0.25 + 0.25 * cotan * cotan);
                            cos = sin * cotan;
                        } else {
                            tan = sdiag[k] / r[k][k];
                            cos = 0.5 / Math.sqrt(0.25 + 0.25 * tan * tan);
                            sin = cos * tan;
                        }

                        // compute the modified diagonal element of r and
                        // the modified element of ((q transpose)*b,0).
                        r[k][k] = cos * r[k][k] + sin * sdiag[k];
                        temp = cos * wa[k] + sin * qtbpj;
                        qtbpj = -sin * wa[k] + cos * qtbpj;
                        wa[k] = temp;

                        // accumulate the tranformation in the row of s.
                        int kp1 = k + 1;
                        //if (n < kp1) go to 70
                        for (int i = kp1; i < n; i++) {
                            temp = cos * r[i][k] + sin * sdiag[i];
                            sdiag[i] = -sin * r[i][k] + cos * sdiag[i];
                            r[i][k] = temp;
                        }
                    }
                }
            }

            // store the diagonal element of s and restore
            // the corresponding diagonal element of r.
            sdiag[j] = r[j][j];
            r[j][j] = x[j];
        }

        // solve the triangular system for z. if the system is
        // singular, then obtain a least squares solution.
        nsing = n;
        for (int j = 0; j < n; j++) {
            if (sdiag[j] == 0 && nsing == n) {
                nsing = j - 1;
            }
            if (nsing < n) {
                wa[j] = 0;
            }
        }
        for (int j = nsing - 1; j >= 0; j--) {
            sum = 0;
            for (int i = j + 1; i < nsing; i++) {
                sum = sum + r[i][j] * wa[i];
            }
            wa[j] = (wa[j] - sum) / sdiag[j];
        }

        // permute the components of z back to components of x.
        for (int j = 0; j < n; j++) {
            x[ipvt[j]] = wa[j];
        }
    }

    /**
     * Solve a linear system of equations.
     * @param matrix The equations as an n x n+1 augmented matrix. (The
     * last column is the result vector.)
     * @return The answer in an array of length n.
     */
    public static double[] solve(double[][] matrix) {
        int degree = matrix.length - 1;
        boolean inputOk = true;
        if (degree < 1) {
            inputOk = false;
        } else {
            for (int i = 0; i <= degree; i++) {
                if (matrix[i].length != degree + 2) {
                    inputOk = false;
                    break;
                }
            }
        }
        if (!inputOk) {
            System.err.println("TuneUtilities.solve: "
                               + "requires n by n+1 matrix as input (n > 1)");
            return null;
        }

        int i,j,k;
        int pivot;         /* row number of largest element in current
                            * column in lower triangular matrix */
        double tmp;

        for (i=0; i<=degree; i++) {

            /* find pivot element */

            pivot=i;
            for (j=i + 1 ; j <= degree; j++) {
                if (Math.abs(matrix[j][i] - matrix[pivot][i])
                    > Math.abs(matrix[pivot][i]))
                {
                    pivot = j;
                }
            }
            if (matrix[pivot][i] == 0.0) {
                /* avoid division by zero */
                return null;
            }

            /* swap current row with pivot element row, if necessary */

            if (pivot != i) {
                for (j = 0; j < degree + 2; j++) {
                    tmp = matrix[i][j];
                    matrix[i][j] = matrix[pivot][j];
                    matrix[pivot][j] = tmp;
                }
            }

            /* convert pivot element to 1.0 */

            tmp = matrix[i][i];
            matrix[i][i] = 1.0;
            for (j = i + 1; j < degree + 2; j++) {
                matrix[i][j] /= tmp;
            }

            /* zero out lower portion of column */

            for (j = i + 1; j <= degree; j++) {
                tmp = matrix[j][i];
                matrix[j][i] = 0.0;
                for (k = i + 1; k < degree + 2; k++) {
                    matrix[j][k] -= matrix[i][k]*tmp;
                }
            }

            /* zero out upper portion of column */

            for (j = i - 1; j >= 0; j--) {
                tmp = matrix[j][i];
                matrix[j][i] = 0.0;
                for (k = i + 1; k < degree + 2; k++) {
                    matrix[j][k] -= matrix[i][k]*tmp;
                }
            }
        }

        /*System.out.println("matrix=");
        for(i=0;i<=degree;i++) {
            for(j=0;j<=degree+1;j++) {
                System.out.print(Fmt.f(7, 6, matrix[i][j], false) + "  ");
            }
            System.out.println();
        }/*DBG*/

        double[] rtn = new double[degree + 1];
        for (i = 0; i <= degree; i++) {
            rtn[i] = matrix[i][degree + 1];
        }
        return rtn;
    }



    /**
     * Holds the result of a non-linear fitting calculation.
     * 
     */
    public static class NLFitResult  extends LinFit.FitResult {

        protected double [] m_fvec;
        protected int m_info;
        protected int m_nfev;
        protected double[][] m_fjac;
        protected int[] m_ipvt;
        protected double[] m_qtf;

        /**
         * Used by lmdif() to store the results of its computation.
         * The main information is just the solution vector, <i>a</i>.
         * @param a The solution vector of length n.
         * @param fvec A vector of length m, giving function evaluation at
         * each data point.
         * @param info Diagnostic error code.
         * @param nfev Number of times function was called to get result.
         * @param fjac An m by n array. The upper n by n submatrix
         *     of fjac contains an upper triangular matrix r with
         *     diagonal elements of nonincreasing magnitude such that
         *<pre>
         *            t     t           t
         *           p *(jac *jac)*p = r *r,
         *</pre>
         *     where p is a permutation matrix and jac is the final
         *     calculated jacobian. Column j of p is column ipvt(j)
         *     (see below) of the identity matrix. the lower trapezoidal
         *     part of fjac contains information generated during
         *     the computation of r.
         * @param ipvt An integer array of length n.
         *     Defines a permutation matrix p such that jac*p = q*r,
         *     where jac is the final calculated jacobian, q is
         *     orthogonal (not stored), and r is upper triangular
         *     with diagonal elements of nonincreasing magnitude.
         *     column j of p is column ipvt(j) of the identity matrix.
         * @param qtf An array of length n which contains
         *     the first n elements of the vector (q transpose)*fvec.
         */
        public NLFitResult(double[] a, double[] fvec, int info, int nfev,
                           double[][] fjac, int[] ipvt, double[] qtf) {
            super(a);
            m_fvec = fvec;
            m_info = info;
            m_nfev = nfev;
            m_fjac = fjac;
            m_ipvt = ipvt;
            m_qtf = qtf;
        }

        /**
         * Get the goodness of fit.  This is just the sum
         * of squared differences between the model and the data,
         * divided by the number of data points minus the number
         * of parameters (degrees of freedom).
         * May be used to estimate the variance.
         */
        public double getChisq() {
            double[] a = getCoeffs();
            int n = a.length;
            int m = m_fvec.length;
            if (n >= m) {
                return Double.NaN;
            } else {
                m_chisq = 0;
                for (int i = 0; i < m; i++) {
                    m_chisq += m_fvec[i] * m_fvec[i];
                }
                return m_chisq / (m - n);
            }
        }

        /**
         *   Returns a code indicating why the computation terminated.
         *<pre>
         *     info = 0  improper input parameters.
         *
         *     info = 1  algorithm estimates that the relative error
         *               in the sum of squares is at most tol.
         *
         *     info = 2  algorithm estimates that the relative error
         *               between x and the solution is at most tol.
         *
         *     info = 3  conditions for info = 1 and info = 2 both hold.
         *
         *     info = 4  fvec is orthogonal to the columns of the
         *               jacobian to machine precision.
         *
         *     info = 5  number of calls to fcn has reached or
         *               exceeded 200*(n+1).
         *
         *     info = 6  tol is too small. no further reduction in
         *               the sum of squares is possible.
         *
         *     info = 7  tol is too small. no further improvement in
         *               the approximate solution x is possible.
         *</pre>
         */
        public int getInfo() {
            return m_info;
        }

        public void setInfo(int i) {
            m_info = i;
        }

        public int getNumberEvaluations() {
            return m_nfev;
        }
    }

    /**
     * Get the equation of the perpendicular bisector of a line segment.
     * @return X(midpoint), Y(midpoint), Slope.
     */
    public static double[] getBisector(double x0, double y0,
                                       double x1, double y1) {
        double[] ans = new double[3];
        ans[0] = (x0 + x1) / 2;
        ans[1] = (y0 + y1) / 2;
        if (y0 == y1) {
            ans[2] = Double.NaN;
        } else {
            ans[2] = -(x1 - x0) / (y1 - y0);
        }
        return ans;
    }

    /**
     * Get the equation of the circle that passes through 3 given points.
     * @return X(center), Y(center), R.
     */
    public static double[] getCircle(double x0, double y0,
                                     double x1, double y1,
                                     double x2, double y2) {
        // Get two perpendicular bisectors of line segments between points
        double[] line0 = getBisector(x0, y0, x1, y1);
        double[] line1 = getBisector(x1, y1, x2, y2);
        if (Double.isNaN(line0[2])) {
            line0 = getBisector(x0, y0, x2, y2);
        } else if (Double.isNaN(line1[2])) {
            line1 = getBisector(x0, y0, x2, y2);
        }
        
        // Get center
        double[][] matrix = new double[2][3];
        matrix[0][0] = line0[2];
        matrix[0][1] = -1;
        matrix[0][2] = line0[2] * line0[0] - line0[1];
        matrix[1][0] = line1[2];
        matrix[1][1] = -1;
        matrix[1][2] = line1[2] * line1[0] - line1[1];
        double[] center = solve(matrix);
        
        double[] ans = new double[3];
        if (center != null) {
            double dx = center[0] - x1;
            double dy = center[1] - y1;
            double r = Math.hypot(dx, dy);
            ans[0] = center[0];
            ans[1] = center[1];
            ans[2] = r;
        }
        return ans;
    }


    /**
     * A class for specifying data to be fit to a circle.  A circle is
     * specified by three double values in an array: these are the X
     * and Y coordinates of the center and the radius, respectively.
     */
    public static class CircleFunc extends LinFit.BasisFunctions {

        private double[] m_xData;
        private double[] m_yData;

        /**
         * Specify the data that is to be fit with a circle.
         * @param x The X-coordinates of the data points.
         * @param y The Y-coordinates of the data points.
         */
        public CircleFunc(double[] x, double[] y) {
            m_xData = x;
            m_yData = y;
            setXLength(3);
            setYLength(x.length);
        }

        /**
         * Get the x,y coordinates of the data that the fit is based on.
         * 
         * @return The x,y coordinates as a 2D array of doubles.
         */
        public double[][] getDataCoordinates() {
            int n = Math.min(m_xData.length, m_yData.length);
            double[][] rtn = new double[n][2];
            for (int i = 0; i < n; i++) {
                rtn[i][0] = m_xData[i];
                rtn[i][1] = m_yData[i];
            }
            return rtn;
        }

        /**
         * Get the square of the distance between 2 points in the data set.
         * @param i0 Index of one point.
         * @param i1 Index of another point.
         * @return Distance**2 between the two points.
         */
        private double distance2(int i0, int i1) {
            double dx = m_xData[i0] - m_xData[i1];
            double dy = m_yData[i0] - m_yData[i1];
            return dx * dx + dy * dy;
        }

        /**
         * Makes an initial guess at the center location and radius
         * of the circle defined by the data.  This implementation
         * assumes that order of points in the data array is roughly
         * monotonic with their order going around the circle.
         * @return The initial guess at the parameter values in an array:
         * <blockquote> a[0] = the x-coordinate of the center of the circle.
         * <br> a[1] = the y-coordinate of the center of the circle.
         * <br> a[2] = the radius of the circle.
         * </blockquote>
         */
        public double[] getEstimatedParameters() {
            int size = m_xData.length;
            if (size < 3) {
                return null;    // Too few points to define a circle
            }
            /*
             * Figure out which 3 data points to use for our estimate.
             * Problem with just using extremes is that if we have data
             * all around the circle, the end points may be nearly
             * coincident.
             * Ideal would be 120 degrees between points, I guess.
             * Note: totally unoptomized for speed.
             */
            int i0 = 0;
            int i1 = size / 2;
            int i2 = size - 1;
            double d01 = distance2(i0, i1);
            double d02 = distance2(i0, i2);
            double d12 = distance2(i1, i2);
            while (d02 < d01 || d02 < d12) {
                if (d01 > d12) {
                    i0++;
                    if (i0 == i1) {
                        break;
                    }
                    d01 = distance2(i0, i1);
                    d02 = distance2(i0, i2);
                } else {
                    i2--;
                    if (i2 == i1) {
                        break;
                    }
                    d02 = distance2(i0, i2);
                    d12 = distance2(i1, i2);
                }
            }
            if (i0 == i1 || i2 == i1) {
                // Algorithm failed, just use extreme values
                i0 = 0;
                i2 = size - 1;
            }

            return getCircle(m_xData[i0], m_yData[i0],
                             m_xData[i1], m_yData[i1],
                             m_xData[i2], m_yData[i2]);
        }


        /**
         * Evaluate the function for all the data points that
         * were defined in the constructor for this object.  This is
         * just the distance of each point from the specified circle.
         * @param a The specification of the circle.
         * <blockquote> a[0] = the x-coordinate of the center of the circle.
         * <br> a[1] = the y-coordinate of the center of the circle.
         * <br> a[2] = the radius of the circle.
         * </blockquote>
         * @return A vector containing the function evaluation for all
         * the data points based on the given parameters.
         */
        public double[] evaluateBasisFunctions(double[] a) {
            double xc = a[0];
            double yc = a[1];
            double r = a[2];
            int m = m_xData.length;
            double[] rtn = new double[m];
            for (int i = 0; i < m; i++) {
                double dx = xc - m_xData[i];
                double dy = yc - m_yData[i];
                rtn[i] = Math.hypot(dx, dy) - r;
            }
            return rtn;
        }
    }
}
