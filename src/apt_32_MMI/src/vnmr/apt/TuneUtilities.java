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

package vnmr.apt;

import java.io.*;
import java.net.*;
import java.util.*;
import java.util.zip.CRC32;

import javax.swing.Icon;
import javax.swing.ImageIcon;

import java.awt.Color;
import java.awt.Image;
import java.awt.Toolkit;

import vnmr.util.Fmt;
import vnmr.util.NLFit;
import static vnmr.apt.AptDefs.*;

/**
 * A collection of static methods that are useful for the probe tuning s/w.
 */
public class TuneUtilities {

    /**
     * Solve a system of two linear equations.
     * The following equations are solved for x, y:
     * <pre>
     * c[0][0] * x + c[0][1] * y = c[0][2]
     * c[1][0] * x + c[1][1] * y = c[1][2]
     * </pre>
     * <p>
     * For solving larger systems of linear equations, consider using
     * {@link  vnmr.util.NLFit#solve(double[][])}.
     * @param c An array containing the six coefficients in the equations.
     * @return An array containing the solution x and y,
     * or null if the system is indeterminate.
     */
    public static double[] solveLinear(double[][] c) {
        double[] ans = null;

        if (c.length != 2 || c[0].length != 3 || c[1].length != 3) {
            Messages.postDebug("Fitting", "TuneUtilities.solveLinear: "
                               + "Wrong number of coefficients");
        } else {
            double det = c[0][0] * c[1][1] - c[0][1] * c[1][0];
            if (det == 0) {
                Messages.postDebug("Fitting", "TuneUtilities.solveLinear: "
                               + "Degenerate system");
            } else {
                ans = new double[2];
                ans[0] = (c[1][1] * c[0][2] - c[0][1] * c[1][2]) / det;
                ans[1] = (-c[1][0] * c[0][2] + c[0][0] * c[1][2]) / det;
            }
        }
        return ans;
    }

    /**
     * Fits y = a + b * x
     * @param x The abscissas of the fit.
     * @param y The data values to fit.
     * @param wts Individual data weights: 1 / sigma(y)**2
     * @return Array of {a, b, sigma(a), sigma(b)}
     */
    public static double[] linFit(double x[], double y[], double wts[]) {
        double[] rtn = new double[5]; // {a, b, sigma(a), sigma(b), chisq}
        int ndata = Math.min(x.length, y.length);

        double ss = 0;
        double sx = 0;
        double sy = 0;
        if (wts != null) {
            for (int i = 0; i < ndata; i++) {
                ss += wts[i];
                sx += x[i] * wts[i];
                sy += y[i] * wts[i];
            }
        } else {
            for (int i = 0; i < ndata; i++) {
                sx += x[i];
                sy += y[i];
            }
            ss = ndata;
        }

        double sxoss = sx / ss;
        double st2 = 0;
        rtn[1] = 0;
        if (wts != null) {
            for (int i = 0; i < ndata; i++) {
                double sigmaInv = Math.sqrt(wts[i]);
                double t = (x[i] - sxoss) * sigmaInv;
                st2 += t * t;
                rtn[1] += t * y[i] * sigmaInv;
            }
        } else {
            for (int i = 0; i < ndata; i++) {
                double t = (x[i] - sxoss);
                st2 += t * t;
                rtn[1] += t * y[i];
            }
        }

        rtn[1] /= st2;
        rtn[0] = (sy - sx * rtn[1]) / ss;
        rtn[2] = Math.sqrt((1 + (sx * sx) / (ss * st2)) / ss);
        rtn[3] = Math.sqrt(1 / st2);

        double chisq = 0;
        if (wts != null) {
            for (int i = 0; i < ndata; i++) {
                double delta = y[i] - rtn[0] - rtn[1] * x[i];
                chisq += delta * delta * wts[i];
            }
        } else {
            for (int i = 0; i < ndata; i++) {
                double delta = y[i] - rtn[0] - rtn[1] * x[i];
                chisq += delta * delta;
            }
            double sigdat = Math.sqrt(chisq / (ndata - 2));
            rtn[2] *= sigdat;
            rtn[3] *= sigdat;
        }
        rtn[4] = chisq;

        return rtn;
    }

    /**
     * Fit a polynomial to a set of data.
     * @param x The data x values.
     * @param y The data y values.
     * @param degree The degree of the polynomial to fit.
     * @return The coefficients of the best-fit polynomial.
     */
    public static double[] polyFit(double x[], double y[],int degree) {
        int npts = x.length;
        double[] S = new double[2 * degree + 1]; /* sums of x[i]^k */
        double[] T = new double[degree + 1]; /* sums of x[i]^k * y[i] */
        double[][] aug = new double[degree+1][degree+2]; /* augmented matrix */

        // Compute sums required for curve fitting.
        for (int i = 0; i < degree; i++) {
            S[i + 1] = 0.0;
            S[i + degree + 1] = 0.0;
            T[i] = 0.0;
        }
        S[0] = npts;
        T[degree] = 0.0;
        for (int i = 0; i < npts; i++) {
            double xi = x[i];
            double xik = xi;
            double yi = y[i];
            for (int j = 1; j <= degree; j++) {
                S[j] += xik;
                T[j] += xik * yi;
                xik = xi * xik;
            }
            for (int j = degree + 1; j <= 2 * degree; j++){
                S[j] += xik;
                xik = xi * xik;
            }
            T[0] += yi;
        }

        // Build the augmented matrix
        for (int i = 0; i <= degree; i++) {
            for (int j = i + 1; j <= degree; j++) {
                aug[i][j] = aug[j][i] = S[i+j];
            }
        }
        for (int i = 0; i <= degree; i++) {
            aug[i][i] = S[i+i];
            aug[i][degree+1] = T[i];
        }

        /*ProbeTune.printDebugMessage(4, "aug=");
        for (int i = 0; i <= degree; i++) {
            for (int j = 0; j <= degree + 1; j++) {
                ProbeTune.printDebugMessage(4, Fmt.f(7, 3, aug[i][j], false) + "  ");
            }
            ProbeTune.printlnDebugMessage(4, "");
        }/*CMP*/



        // Solve the normal equations.
        double[] coeff = NLFit.solve(aug);

        return coeff;
    }

    /**
     * Calculate the residual in the fit of a polynomial
     * to the sqrt of given data.
     * Calculates the Root Mean Square residual of a polynomial fit to
     * the square root of the data points passed in.
     * A set of data points is assumed to be equally spaced in x.
     * Fits a polynomial through a subrange of the data.
     * Calculates the RMS residual of the data with respect to the fit.
     * Does not return the fit coefficients; they are just calculated
     * and used internally.
     * @param y The array of data values.
     * @param begin The index of the first datum to use.
     * @param end One more than the index of the last datum to use.
     * @param degree The degree of the polynomial to fit.
     * @return The RMS residual of the fit.
     */
    public static double rootPolyResidual(double[] y,
                                          int begin, int end, int degree) {
        int len = end - begin;
        double[] x = new double[len];
        double[] yy = new double[len];

        // Make arrays of equally spaced data
        for (int i = 0; i < len; i++) {
            x[i] = i;
            yy[i] = Math.sqrt(y[i + begin]);
        }

        // Get coeffs of fit
        double[] a = polyFit(x, yy, degree);

        // Calculate residuals
        double resid;
        if (a == null) {
            resid = -1;
        } else {
            double resid2 = 0;
            for (int i = 0; i < len; i++) {
                double r = yy[i] - polyValue(x[i], a);
                resid2 += r * r;
            }
            resid2 /= (len - degree - 1);
            resid = Math.sqrt(resid2);
        }
        return resid;
    }

    /**
     * Evaluates a polynomial at a given point. The polynomial
     * is defined as
     * <pre>y = a[0] + a[1]*x + a[2]*x^2 + ... + a[n]*x^n.</pre>
     * @param x The point to evaluate.
     * @param a The polynomial coefficients.
     * @return The y value at the given point.
     */
    public static double polyValue(double x, double[] a) {
        double y = 0;
        for (int j = a.length - 1; j >= 0; --j) {
            y = (y * x) + a[j];
        }
        return y;
    }

    /**
     * Evaluates the derivative of a polynomial at a given point.
     * The polynomial is defined as
     * <pre>y = a[0] + a[1]*x + a[2]*x^2 + ... + a[n]*x^n.</pre>
     * @param x The point to evaluate.
     * @param a The polynomial coefficients.
     * @return The dy/dx value at the given point.
     */
    public static double polyDeriv(double x, double[] a) {
        double y = 0;
        for (int j = a.length - 1; j > 0; --j) {
            y = (y * x) + j * a[j];
        }
        return y;
    }

    /**
     * Given a polynomial function, find the x value that gives the specified
     * y value.  The caller also specifies the range of x values to search.
     * Uses binary search, so it will succeed iff the function values at
     * the given "xlimits" bracket the desired "y" value.
     * @param y The y value for which to find the x.
     * @param a The coefficients of the polynominal.
     * @param xlimits Array of two doubles, specifying the two ends of the
     * x range to search.
     * @return The requested x value, or NaN if the function values at
     * the two ends of the range do not bracket the given y value.
     */
    public static double invPoly(double y, double[] a, double[] xlimits) {
        final double TOL = 1e-10;
        double xmin = xlimits[0];
        double xmax = xlimits[1];
        double ymin = polyValue(xmin, a);
        double ymax = polyValue(xmax, a);
        if ((ymin - y) * (ymax -y) > 0) {
            // xlimits may not bracket the solution
            return Double.NaN;
        }
        double ftnSign = ymax - ymin;
        double diff = xmax - xmin;
        int iters = 0;
        while (diff > TOL) {
            double xi = (xmax + xmin) / 2;
            double ydiff = y - polyValue(xi, a);
            if (ydiff * ftnSign > 0) {
                xmin = xi;
            } else {
                xmax = xi;
            }
            diff = xmax - xmin;
            iters++;
        }
        Messages.postDebug("TuneUtilities.poly", "" + iters);
        return (xmax + xmin) / 2;
    }

    /**
     * Find the median of a Collection of Doubles.
     * For an odd number of values, returns the middle value.
     * For an even number, returns the average of the middle two values.
     * @param list The collection of doubles.
     * @return The median value.
     */
    public static double getMedian(Collection<Double> list) {
        ArrayList<Double> sortList = new ArrayList<Double>(list);
        Collections.sort(sortList);
        int size = list.size();
        double v1 = ((Double)sortList.get((size - 1) / 2)).doubleValue();
        double v2 = ((Double)sortList.get(size / 2)).doubleValue();
        return (v1 + v2) / 2;
    }

    /**
     * Find the mean and standard deviation of an array of doubles.
     * The standard deviation is the scatter of the individual values
     * about the mean.
     * @param vals The array of values.
     * @return Array of 2 doubles: mean and standard deviation, respectively.
     */
    public static double[] getMean(double[] vals) {
        double[] meanAndSdv = new double[2];
        int n = vals.length;
        double sum = 0;
        for (double x : vals) {
            sum += x;
        }
        meanAndSdv[0] = sum / n;

        double sumsq = 0;
        for (double x : vals) {
            double dx = x - meanAndSdv[0];
            sumsq += dx * dx;
        }
        meanAndSdv[1] = Math.sqrt(sumsq / (n - 1));

        return meanAndSdv;
    }

    /**
     * Find the index of the maximum value in an array of doubles.
     * @param values The array of values.
     * @return The maximum value in the array.
     */
    public static int getIndexOfMax(double[] values) {
        int size = values.length;
        if (size <= 0) {
            return -1;
        }
        int maxIdx = 0;
        double maxVal = values[0];
        for (int i = 1; i < size; i++) {
            if (maxVal < values[i]) {
                maxVal = values[i];
                maxIdx = i;
            }
        }
        return maxIdx;
    }

    /**
     * Given two equal length arrays of doubles, where one holds numerators
     * and the other holds denominators, find the index of the elements for
     * which the absolute value of the corresponding fraction is greatest.
     * A denominator is allowed to be 0; if the corresponding numerator is also
     * 0, the corresponding fraction will NOT be larger than ANY other.
     * If multiple indices tie for the largest fraction,
     * the first such index is returned.
     * @param num An array of numerators.
     * @param dem An array of denominators.
     * @return The index, i, for which abs(num[i]/dem[i]) is greatest, or -1
     * if one of the arrays has 0 length.
     */
    public static int getIndexOfMaxAbsRatio(double[] num, double[] dem) {
        int size = Math.min(num.length, dem.length);
        if (size <= 0) {
            return -1;
        }
        int maxIdx = 0;
        double maxR = Math.abs(num[0] / dem[0]); // Could be POSITIVE_INFINITY
        for (int i = 1; i < size; i++) {
            double ratio = Math.abs(num[i] / dem[i]);
            if (maxR < ratio) {
                maxR = ratio;
                maxIdx = i;
            }
        }
        return maxIdx;
    }

    /**
     * Given two equal length arrays of doubles, where one holds numerators
     * and the other holds denominators, find the index of the elements for
     * which the absolute value of the corresponding fraction is greatest.
     * This is the same as getIndexOfMaxAbsRatio(), except that
     * if multiple indices tie for the largest fraction,
     * the last such index is returned.
     * @param num An array of numerators.
     * @param dem An array of denominators.
     * @return The index, i, for which abs(num[i]/dem[i]) is greatest, or -1
     * if one of the arrays has 0 length.
     */
    public static int getIndexOfMaxAbsRatioR(double[] num, double[] dem) {
        int size = Math.min(num.length, dem.length);
        if (size <= 0) {
            return -1;
        }
        int maxIdx = size - 1;
        double maxR = Math.abs(num[maxIdx] / dem[maxIdx]);
        for (int i = maxIdx - 1; i >= 0; --i) {
            double ratio = Math.abs(num[i] / dem[i]);
            if (maxR < ratio) {
                maxR = ratio;
                maxIdx = i;
            }
        }
        return maxIdx;
    }

    /**
     * Find the maximum absolute value in an array of doubles.
     * @param values An array of doubles.
     * @return The absolute value of the array element with the largest
     * absolute value.
     */
    public static double getMaxAbs(double[] values) {
        int size = values.length;
        if (size <= 0) {
            return Double.NaN;
        }
        double maxVal = Math.abs(values[0]);
        for (int i = 1; i < size; i++) {
            double x = Math.abs(values[i]);
            if (maxVal < x) {
                maxVal = x;
            }
        }
        return maxVal;
    }

    /**
     * Open a socket by hostname and port number.
     * Uses a default timeout set in this method.
     * @param id A string that prefixes any error messages produced
     * by this method.
     * @param host The name or IP address of the host.
     * @param port The port number to connect to.
     * @return The socket, or null on failure.
     */
    public static Socket getSocket(String id, String host, int port) {
        final int TIMEOUT = 500; // ms
        Socket socket = null;
        try {
            Messages.postDebug("Sockets", id + ": Trying connection (waiting "
                               + TIMEOUT + " ms)");
            InetAddress inetAddr = InetAddress.getByName(host);
            InetSocketAddress inetSocketAddr;
            inetSocketAddr = new InetSocketAddress(inetAddr, port);
            socket = new Socket();
            socket.connect(inetSocketAddr, TIMEOUT);
        } catch (IOException ioe) {
            if (ioe instanceof SocketTimeoutException) {
                Messages.postError(id + ": Timeout connecting");
                socket = null;
            } else {
                Messages.postError(id + ": IOException connecting");
            }
        }
        return socket;
    }

    /**
     * Get an Icon from the JAR file.
     * @param f The name of the icon file.
     * @param c The class where it is to be found.
     * @return The Icon, or null.
     */
    public static Icon getIcon(String f, Class<?> c) {
        Icon imageIcon = null;
        try {
            URL imageURL = c.getResource(f);
            if (imageURL == null) {
                return null;
            }
            java.awt.image.ImageProducer I_P;
            I_P = (java.awt.image.ImageProducer)imageURL.getContent();
            Toolkit tk = Toolkit.getDefaultToolkit();
            Image img = tk.createImage(I_P);
            if (img == null) {
                return null;
            }
            imageIcon = new ImageIcon(img);
        } catch (IOException e) {
            return null;
        }
        return imageIcon;
    }

    /**
     * Get an Image resource -- probably from the JAR file.
     * @param f The name of the image. May be an absolute path.
     * @param c If name is not absolute, the class loader for this
     * path determines the path used to find the image.
     * @return The Image, or null.
     */
    public static Image getImage(String f, Class<?> c) {
        Image img = null;
        try {
            URL imageURL = c.getResource(f);
            if (imageURL == null) {
                return null;
            }
            java.awt.image.ImageProducer I_P;
            I_P = (java.awt.image.ImageProducer)imageURL.getContent();
            Toolkit tk = Toolkit.getDefaultToolkit();
            img = tk.createImage(I_P);
        } catch (IOException e) {
            return null;
        }
        return img;
    }

    /**
     * Construct a Color that is a brighter or darker version of a given
     * color.
     * Positive percent changes make the color brighter, unless it
     * is already as bright as it can get without changing the hue.
     * A percent change of -100 will always give black.
     * @param c The original color.
     * @param percent The desired percentage change in brightness.
     * @return The new color.
     */
    public static Color changeBrightness(Color c, int percent) {
        if (c == null) {
            return c;
        }
        int r = c.getRed();
        int g = c.getGreen();
        int b = c.getBlue();
        float[] hsb = Color.RGBtoHSB(r, g, b, null);
        float fraction = (float)percent / 100;
        float remainder = (float)0;

        hsb[2] = hsb[2] * (1 + fraction);
        if (hsb[2] < 0) {
            remainder = hsb[2];
            hsb[2] = 0;
        } else if (hsb[2] > 1) {
            // Can't make it bright enough with Brightness alone,
            // decrease Saturation.
            remainder = hsb[2] - 1;
            hsb[2] = 1;
            hsb[1] = hsb[1] * (1 - 5 * remainder);
            if (hsb[1] < 0) {
                hsb[1] = 0;
            }
        }
        return Color.getHSBColor(hsb[0], hsb[1], hsb[2]);
    }

    /**
     * Save a given string as a file at the given path.
     * The file contents will be valid and complete at the time it can
     * first be accessed.
     * Thread safe.
     * @param path The path to the file to write.
     * @param buffer The String that is put into the file.
     * @return True if successful, otherwise false.
     */
    public static synchronized boolean writeFileSafely(String path,
                                                       String buffer) {
        // Open a temp file, write the buffer, and rename the file
        if (path == null) {
            return false;
        }
        String tmppath = path + ".tmp";
        boolean rtn = writeFile(tmppath, buffer);

        // Move the temp file to requested place
        File tmpfile = new File(tmppath);
        File file = new File(path);
        if (!tmpfile.renameTo(file)) {
            file.delete();
            if (!tmpfile.renameTo(file)) {
                Messages.postError("Unable to rename " + tmppath
                                   + " to " + path);
                rtn = false;
            }
        }
        return rtn;
    }

    /**
     * Save a given string as a file at the given path.
     * @param path The path to the file to write.
     * @param buffer The String that is put into the file.
     * @return True if successful, otherwise false.
     */
    public static synchronized boolean writeFile(String path, String buffer) {
        if (path == null) {
            return false;
        }
        File file = new File(path);
        PrintWriter out = null;
        boolean rtn = true;
        try {
            out = new PrintWriter (new BufferedWriter(new FileWriter(file)));
            out.print(buffer);
        } catch (IOException ioe) {
            Messages.postError("TuneUtilities.writeToFile: Error writing "
                               + path + ": " + ioe);
            rtn = false;
        } finally {
            try {
                out.close();
            } catch (Exception e) {
            }
        }
        return rtn;
    }

    /**
     * Append a given string to a file, prepending a header line iff
     * the file is non-existent or empty, and appending a line separator.
     * @param path Pathname of the file.
     * @param buffer The string to write.
     * @param header The header line.
     * @return True if the write succeeded.
     */
    public static boolean appendLog(String path, String buffer, String header) {
        File file = new File(path);
        // NB: file.length() returns 0 if the file does not exist.
        if (header != null && (!file.canWrite() || file.length() == 0)) {
            // Prepend the header to the stuff to write
            buffer = header + NL + buffer;
        }
        return appendLog(path, buffer);
    }

    /**
     * Append the given string to a file, appending a line separator.
     * Thread safe.
     * @param path Pathname of the file.
     * @param buffer The string to write.
     * @return True if the write succeeded.
     */
    public static synchronized boolean appendLog(String path, String buffer) {
        // Open a file and append the buffer
        File file = new File(path);
        if (!file.canWrite()) {
            file.delete();
        }
        PrintWriter out = null;
        boolean rtn = true;
        boolean append = true;
        try {
            out = new PrintWriter
                    (new BufferedWriter(new FileWriter(file, append)));
            out.print(buffer + NL);
        } catch (IOException ioe) {
            Messages.postError("TuneUtilities.appendLog: Error writing "
                               + path + ": " + ioe);
            rtn = false;
        } finally {
            out.close();
        }
        return rtn;
    }

    public static BufferedReader getReader(String filepath) {
        return getReader(filepath, false);
}

    public static BufferedReader getReader(String filepath,
                                           boolean wantErrorMsg) {
        BufferedReader in = null;
        String errMsg = "(NULL file path)";
        if (filepath != null) {
            try {
                in = new BufferedReader(new FileReader(filepath));
            } catch (IOException ioe) {
                errMsg = ioe.toString();
            }
        }
        if (in == null && wantErrorMsg) {
            Messages.postError("Cannot read the file \""
                               + filepath + "\": " + errMsg);
        }
        return in;
    }

    /**
     * Read the contents of a file and return it as a String.
     * @param filepath The filepath to read from.
     * @param wantErrorMsg If true, errors print a message.
     * @return The contents of the file; maybe empty, but never null.
     */
    public static String readFile(String filepath, boolean wantErrorMsg) {
        StringBuffer sb = new StringBuffer();
        BufferedReader in = null;
        try {
            in = getReader(filepath, wantErrorMsg);
            String line = null;
            while ((line = in.readLine()) != null) {
                sb.append(line).append("\n");
            }
        } catch (Exception e) {
            if (wantErrorMsg) {
                Messages.postError("Cannot read file: " + filepath);
            }
        } finally {
            try { in.close(); } catch (Exception e) {}
        }
        return sb.toString();
    }

    /**
     * Get the standard CRC32 checksum of a byte array.
     * @param buf The bytes to calculate the checksum of.
     * @return The checksum.
     */
    public static long getCrc32(byte[] buf) {
        CRC32 crc = new CRC32();
        crc.update(buf);
        return crc.getValue();
    }

    /**
     * Read a string from the System Properties with a given key.
     * @param key The key (name) of the property string.
     * @param deflt The value to be returned if the property doesn't exist.
     * @return The property string (or the default).
     */
    public static String getProperty(String key, String deflt) {
        String prop = System.getProperty(key);
        return (prop == null) ? deflt : prop.trim();
    }

    /**
     * Read a double value from the System Properties with a given key.
     * @param key The key (name) of the property string.
     * @param deflt The value to be returned if the property doesn't exist
     * or is not a numeric value.
     * @return The property string (or the default).
     */
    public static double getDoubleProperty(String key, double deflt) {
        double rtn = deflt;
        String prop = getProperty(key, "");
        if (prop != null && (prop = prop.trim()).length() > 0) {
            try {
                rtn = Double.valueOf(prop);
            } catch (NumberFormatException nfe) {
                Messages.postError("Property \"" + key
                                   + "\" is set to non-numeric value \""
                                   + prop + "\"");
            }
        }
        return rtn;
    }

    /**
     * Read an integer value from the System Properties with a given key.
     * @param key The key (name) of the property string.
     * @param deflt The value to be returned if the property doesn't exist
     * or is not a numeric value.
     * @return The property string (or the default).
     */
    public static int getIntProperty(String key, int deflt) {
        return (int)Math.round(getDoubleProperty(key, deflt));
    }

    /**
     * Read a boolean value from the System Properties with a given key.
     * @param key The key (name) of the property string.
     * @param deflt The value to be returned if the property doesn't exist.
     * @return True if the property string is "true" or "yes", ignoring case
     * (or the default, if the property is not set).
     */
    public static boolean getBooleanProperty(String key, boolean deflt) {
        String val = getProperty(key, Boolean.toString(deflt));
        return (val.equalsIgnoreCase("on")
                || val.equalsIgnoreCase("yes")
                || val.equalsIgnoreCase("true"));
    }

    /**
     * Adjusts a given angle by a multiple of 2*PI to make it close to
     * another angle.
     * @param theta1 The angle to adjust (radians).
     * @param theta2 The reference angle to get close to (radians).
     * @return The adjusted value of theta1 (radians).
     */
    public static double wrapAngle(double theta1, double theta2) {
        while (Math.abs(theta1 - theta2) > Math.PI) {
            if (theta1 > theta2) {
                /*System.err.println("theta1: " + Fmt.f(2, theta1) + " --> "
                                   + Fmt.f(2, theta1 - 2 * Math.PI));/*DBG*/
                theta1 -= 2 * Math.PI;
            } else {
                /*System.err.println("theta1: " + Fmt.f(2, theta1) + " --> "
                               + Fmt.f(2, theta1 + 2 * Math.PI));/*DBG*/
                theta1 += 2 * Math.PI;
            }
        }
        return theta1;
    }


    /**
     *  Copies src file to dst file.
     * If the dst file does not exist, it is created.
     * @param src The source file.
     * @param dst The destination file.
     * @throws Exception If the input could not be read, or the output
     * could not be written.
     */
    public static void copy(File src, File dst) throws Exception {

        InputStream in = new FileInputStream(src);
        OutputStream out = new FileOutputStream(dst);


        // Transfer bytes from in to out
        byte[] buf = new byte[1024];
        int len;

        try {
            while ((len = in.read(buf)) > 0) {
                //ProbeTune.printlnDebugMessage(4, "Length: " + len);
                out.write(buf, 0, len);
            }
        } finally {
            try {
                in.close();
            } catch (Exception e) {}
            try {
                out.close();
            } catch (Exception e) {}
        }
    }

    public static void copyDir(File src, File dst) throws Exception {
        if (src.isDirectory()){

            dst.mkdirs();
            String list[] = src.list();

            for (int i = 0; i < list.length; i++)
            {
                File dest1 = new File(dst.getAbsolutePath() + File.separator
                                      + list[i]);
                File src1 = new File(src.getAbsolutePath() + File.separator
                                     + list[i]);
                copy(src1 , dest1);
            }
        }
    }

    public static double calculateSigma(String path) {
        double sigma = 0;
        String buf = readFile(path, true);
        String[] lines = buf.split("\n+");
        double sum = 0;
        int n = 0;
        for (String line : lines) {
            try {
                sum += Double.parseDouble(line);
                n++;
            } catch (NumberFormatException nfe) {
                return -1;
            }
        }
        double mean = sum / n;
        sum = 0;
        for (String line : lines) {
            double dx = Double.parseDouble(line) - mean;
            sum += dx * dx;
        }
        sigma = Math.sqrt(sum / (n - 1));
        Messages.postInfo("mean=" + Fmt.g(7, mean)
                          + ", sigma=" + Fmt.g(3, sigma)
                          + ", n=" + n + " in file: " + path);
        return sigma;
    }
}
