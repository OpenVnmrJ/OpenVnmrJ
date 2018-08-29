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

/*
 * Adapted from the PtPlot plotting package.
 */

/* A signal plotter.

@Copyright (c) 1997-2001 The Regents of the University of California.
All rights reserved.

Permission is hereby granted, without written agreement and without
license or royalty fees, to use, copy, modify, and distribute this
software and its documentation for any purpose, provided that the
above copyright notice and the following two paragraphs appear in all
copies of this software.

IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY
FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF
THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE
PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY OF
CALIFORNIA HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
ENHANCEMENTS, OR MODIFICATIONS.

*/
package vnmr.bo;

// TO DO:
//   - steps between points rather than connected lines.
//   - cubic spline interpolation
//
// NOTE: The XOR drawing mode is needed in order to be able to erase
// plotted points and restore the grid line, tick marks, and boundary
// rectangle.  This introduces a number of artifacts, particularly
// where lines cross.  A better alternative in the long run would be
// use Java 2-D, which treats each notation on the screen as an object,
// and supports redrawing only damaged regions of the screen.

// NOTE: There are quite a few subjective spacing parameters, all
// given, unfortunately, in pixels.  This means that as resolutions
// get better, this program may need to be adjusted.


import java.awt.Component;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.Color;
import java.awt.Dimension;
import java.io.*;
import java.util.*;

import javax.swing.Icon;
import javax.swing.JComponent;
import java.awt.Rectangle;


//////////////////////////////////////////////////////////////////////////
//// Plot
/**
A flexible signal plotter.  The plot can be configured and data can
be provided either through a file with commands or through direct
invocation of the public methods of the class.
<p>
When calling the public methods, in most cases the changes will not
be visible until paintComponent() is called.  To request that this
be done, call repaint().  One exception is addPoint(), which
makes the new point visible immediately if the plot is visible on
the screen.
<p>
This base class supports a simple file syntax that has largely been
replaced by the XML-based PlotML syntax.  To read a file or a
URL in this older syntax, use the read() method.
This older syntax contains any number commands,
one per line.  Unrecognized commands and commands with syntax
errors are ignored.  Comments are denoted by a line starting with a
pound sign "#".  The recognized commands include those supported by
the base class, plus a few more.  The commands are case
insensitive, but are usually capitalized.  The number of data sets
to be plotted does not need to be specified.  Data sets are added as needed.
Each dataset can be optionally identified with
color (see the base class) or with unique marks.  The style of
marks used to denote a data point is defined by one of the following
commands:
<pre>
Marks: none
Marks: points
Marks: dots
Marks: various
Marks: pixels
</pre>
Here, "points" are small dots, while "dots" are larger.  If "various"
is specified, then unique marks are used for the first ten data sets,
and then recycled. If "pixels" are specified, then each point is
drawn as one pixel.
Using no marks is useful when lines connect the points in a plot,
which is done by default.  However, if persistence is set, then you
may want to choose "pixels" because the lines may overlap, resulting
in annoying gaps in the drawn line.
If the above directive appears before any DataSet directive, then it
specifies the default for all data sets.  If it appears after a DataSet
directive, then it applies only to that data set.
<p>
To disable connecting lines, use:
<pre>
Lines: off
</pre>
To reenable them, use
<pre>
Lines: on
</pre>
You can also specify "impulses", which are lines drawn from a plotted point
down to the x axis.  Plots with impulses are often called "stem plots."
These are off by default, but can be turned on with the
command:
<pre>
Impulses: on
</pre>
or back off with the command
<pre>
Impulses: off
</pre>
If that command appears before any DataSet directive, then the command
applies to all data sets.  Otherwise, it applies only to the current data
set.
To create a bar graph, turn off lines and use any of the following commands:
<pre>
Bars: on
Bars: <i>width</i>
Bars: <i>width, offset</i>
</pre>
The <i>width</i> is a real number specifying the width of the bars
in the units of the x axis.  The <i>offset</i> is a real number
specifying how much the bar of the <i>i</i><sup>th</sup> data set
is offset from the previous one.  This allows bars to "peek out"
from behind the ones in front.  Note that the frontmost data set
will be the first one.  To turn off bars, use
<pre>
Bars: off
</pre>
To specify data to be plotted, start a data set with the following command:
<pre>
DataSet: <i>string</i>
</pre>
Here, <i>string</i> is a label that will appear in the legend.
It is not necessary to enclose the string in quotation marks.
To start a new dataset without giving it a name, use:
<pre>
DataSet:
</pre>
In this case, no item will appear in the legend.
New datasets are plotted <i>behind</i> the previous ones.
If the following directive occurs:
<pre>
ReuseDataSets: on
</pre>
Then datasets with the same name will be merged.  This makes it
easier to combine multiple datafiles that contain the same datasets
into one file.  By default, this capability is turned off, so
datasets with the same name are not merged.
The data itself is given by a sequence of commands with one of the
following forms:
<pre>
<i>x</i>, <i>y</i>
draw: <i>x</i>, <i>y</i>
move: <i>x</i>, <i>y</i>
<i>x</i>, <i>y</i>, <i>yLowErrorBar</i>, <i>yHighErrorBar</i>
draw: <i>x</i>, <i>y</i>, <i>yLowErrorBar</i>, <i>yHighErrorBar</i>
move: <i>x</i>, <i>y</i>, <i>yLowErrorBar</i>, <i>yHighErrorBar</i>
</pre>
The "draw" command is optional, so the first two forms are equivalent.
The "move" command causes a break in connected points, if lines are
being drawn between points. The numbers <i>x</i> and <i>y</i> are
arbitrary numbers as supported by the Double parser in Java.
If there are four numbers, then the last two numbers are assumed to
be the lower and upper values for error bars.
The numbers can be separated by commas, spaces or tabs.
<p>
This plotter has some <A NAME="ptplot limitations">limitations</a>:
<ul>
<li> If you zoom in far enough, the plot becomes unreliable.
     In particular, if the total extent of the plot is more than
     2<sup>32</sup> times extent of the visible area, quantization
     errors can result in displaying points or lines.
     Note that 2<sup>32</sup> is over 4 billion.
<li> The limitations of the log axis facility are listed in
     the <code>_gridInit()</code> method in the PlotBox class.
</ul>

@author Edward A. Lee, Christopher Hylands
 */
public class Plot extends PlotBox implements Cloneable {

    ///////////////////////////////////////////////////////////////////
    ////                         public methods                    ////

    public Plot() {
    }

    // NB: This constructor not needed if we use shallowCopy() for EPS plots
    public Plot(Plot oldPlot) {
        super();
        setSize(oldPlot.getSize().width, oldPlot.getSize().height);
        setColors(oldPlot.getColors());
        //setMarkColor(Color.GREEN, 0);
        //setMarkColor(Color.ORANGE, 2);
        setTitle(oldPlot.getTitle());
        setTitleFont(oldPlot.getTitleFont());
        setXAxis(oldPlot.getXAxis());
        setYAxis(oldPlot.getYAxis());
        setXLabel(oldPlot.getXLabel());
        setYLabel(oldPlot.getYLabel());
        setLabelFont(oldPlot.getLabelFont());
        setXLog(oldPlot.getXLog());
        setYLog(oldPlot.getYLog());
        setXRange(oldPlot.getXRange()[0], oldPlot.getXRange()[1]);
        setYRange(oldPlot.getYRange()[0], oldPlot.getYRange()[1]);
        setGrid(oldPlot.getGrid());
        setGridColor(oldPlot.getGridColor());
        setTickColor(oldPlot.getTickColor());
        setPoints(oldPlot.getPoints());
        setFormats(oldPlot.getFormats());
        setPrevx(oldPlot.getPrevx());
        setPrevy(oldPlot.getPrevy());
        setIcons(oldPlot.getIcons());
    }

    /**
     * Makes a "shallow copy" of this class instance.  "Shallow" in the
     * sense that the copy gets pointers to the same objects that the
     * original uses.  E.g., the data is not copied, only a single pointer
     * to the data structure.
     * @return The copy of this plot.
     */
    public Plot shallowCopy() {
        try {
            // This relies on the fact that Object.clone() does a shallow copy,
            // and no other ancestors implement clone().
            return (Plot)super.clone();
        } catch (CloneNotSupportedException cnse) {
            // Won't happen, if this class declares "implements Cloneable"
            return (Plot)null;
        }
    }

    public synchronized void export(OutputStream out,
                                    int x, int y,
                                    int ptWidth, int ptHeight,
                                    String creator, String date, String title) {
        try {
            Plot pplot = shallowCopy();
            pplot.setPlotBackground(Color.WHITE);
            pplot.setGridColor(Color.LIGHT_GRAY);
            pplot.setTickColor(Color.BLACK);

            //pplot.setFillButton(false);
            //pplot.setTailButton(false);
            //pplot.removeLegendButtons();
            javax.swing.JFrame.setDefaultLookAndFeelDecorated(true);
            javax.swing.JFrame frame = new javax.swing.JFrame("Print Preview");
            frame.setDefaultCloseOperation(javax.swing.JFrame.DISPOSE_ON_CLOSE);
            frame.getContentPane().add(pplot);
            frame.getContentPane().setBackground(Color.WHITE);
            frame.pack();
            frame.setVisible(true);

            ptHeight = 500;
            //pplot.setBounds(0, 0, ptWidth, ptHeight);
            System.out.println("panelWidth=" + getSize().width
                               + ", legendWidth=" + _legendRectangle.width);
            double scale = 0.75;
            int pixWidth = (int)(ptWidth / scale);
            int pixHeight = (int)(ptHeight / scale);
            EPSGraphics g = new EPSGraphics(out, x, y,
                                            ptWidth, ptHeight,
                                            pixWidth, pixHeight,
                                            creator,
                                            date,
                                            title);
            //int rectWidth = (pixWidth - _legendRectangle.width);
            pplot._drawPlot(g, false,
                            new Rectangle(0, 0, pixWidth, pixHeight));
            //pplot._drawPlot(g, false);
            g.showpage();
        } catch (RuntimeException ex) {
            String message = "Export failed: " + ex.getMessage();
            System.err.println(message);
            //Messages.writeStackTrace(ex, message);
            // TODO: Need more error notification if PS plot fails?
            // Rethrow the exception so that we don't report success,
            // and so the stack trace is displayed on standard out.
            //throw (RuntimeException)ex.fillInStackTrace();
        }
    }

    /**
     * Print to a given file with given page position and size.
     * @param filepath Full path name of the file.
     * @param x X position on page of left side of plot, in points.
     * @param y Y position of top of plot, from bottom of page, in points.
     * @param ptWidth Width of plot in points.
     * @param ptHeight Height of plot in points.
     * @param creator String identifying the creator of the plot,
     * (e.g. "VnmrJ-LC").
     * @param date The date and time the plot was created (current clock time).
     * @param title Title to identify the plot (not included in plot).
     */
    public void printPS(String filepath,
                        int x, int y,
                        int ptWidth, int ptHeight,
                        String creator, String date, String title) {
        //Messages.postDebug("printPS", "x=" + x + ", y=" + y
        //                   + ", w=" + ptWidth + ", h=" + ptHeight);
        OutputStream out = null;
        try {
            out =new FileOutputStream(filepath);
        } catch (FileNotFoundException fnfe) {
            System.err.println("PlotBox.printEPS(): File not found: "
                               + filepath);
            return;
        }
        //export(out, x, y, ptWidth, ptHeight, creator, date, title);
        try {
            Plot pplot = shallowCopy();
            pplot.setPlotBackground(Color.WHITE);
            pplot.setGridColor(changeBrightness(Color.WHITE, -5));
            pplot.setTickColor(Color.BLACK);

            //pplot.setFillButton(false);
            //pplot.setTailButton(false);
            //pplot.removeLegendButtons();
            javax.swing.JFrame.setDefaultLookAndFeelDecorated(true);
            javax.swing.JFrame frame = new javax.swing.JFrame("Print Preview");
            frame.setDefaultCloseOperation(javax.swing.JFrame.DISPOSE_ON_CLOSE);
            frame.getContentPane().add(pplot);
            frame.getContentPane().setBackground(Color.WHITE);
            frame.pack();
            //frame.setVisible(true);

            ptHeight = 500;
            //pplot.setBounds(0, 0, ptWidth, ptHeight);
            System.out.println("panelWidth=" + getSize().width
                               + ", legendWidth=" + _legendRectangle.width);
            double scale = 0.75;
            int pixWidth = (int)(ptWidth / scale);
            int pixHeight = (int)(ptHeight / scale);
            EPSGraphics g = new EPSGraphics(out, x, y,
                                            ptWidth, ptHeight,
                                            pixWidth, pixHeight,
                                            creator,
                                            date,
                                            title);
            //int rectWidth = (pixWidth - _legendRectangle.width);
            pplot._drawPlot(g, false,
                            new Rectangle(0, 0, pixWidth, pixHeight));
            //pplot._drawPlot(g, false);
            g.showpage();
        } catch (RuntimeException ex) {
            String message = "Export failed: " + ex.getMessage();
            System.err.println(message);
            //Messages.writeStackTrace(ex, message);
            // TODO: Need more error notification if PS plot fails?
            // Rethrow the exception so that we don't report success,
            // and so the stack trace is displayed on standard out.
            //throw (RuntimeException)ex.fillInStackTrace();
        }
    }

    /** Add a legend (displayed at the upper right) for the specified
     *  data set with the specified string.  Short strings generally
     *  fit better than long strings.
     *  @param dataset The dataset index.
     *  @param legend The label for the dataset.
     */
    public synchronized void addLegend(int dataset, String legend) {
        _checkDatasetIndex(dataset);
        super.addLegend(dataset, legend);
    }

    /**
     * Set the specified data set to plot the given array of x,y values. Data
     * set indices begin with zero. If the data set does not exist, create it.
     * The fourth argument indicates whether the points should be connected by
     * line segments. Regardless of the value of this argument, lines will not
     * drawn if setConnected() has been called with a false argument.
     * <p>
     * In order to work well with swing and be thread safe, this method actually
     * defers execution to the event dispatch thread, where all user interface
     * actions are performed. Thus, the points will not be set immediately
     * (unless you call this method from within the event dispatch thread). All
     * the methods that do this deferring coordinate so that they are executed
     * in the order that you called them.
     * 
     * @param dataset The data set index.
     * @param x The array of X-coordinates.
     * @param y The array of Y-coordinates.
     * @param npts The number of points in the two arrays.
     * @param connected If true, line segments are drawn connecting the points.
     * @param resetScale If true, new scaling will be calculated.
     */
    public synchronized void setPoints(final int dataset,
                                       final double[] x, final double[] y,
                                       final int npts,
                                       final boolean connected,
                                       final boolean resetScale) {
        Runnable doSetPoints = new Runnable() {
            public void run() {
                _setPoints(dataset, x, y, null, null, npts,
                           connected, false, resetScale);
            }
        };
        _deferIfNecessary(doSetPoints);
    }

    public void setPoints(final int dataset,
                          final double[] x, final double[] y,
                          final boolean connected) {
        setPoints(dataset, x, y, connected, true);
    }

    public void setPoints(final int dataset,
                          final double[] x, final double[] y,
                          final boolean connected,
                          final boolean resetScale) {
        int npts = 0;
        if (x != null && y != null) {
            npts = Math.min(x.length, y.length);
        }
        setPoints(dataset, x, y, npts, connected, resetScale);
    }

    /** In the specified data set, add the specified x, y point to the
     *  plot.  Data set indices begin with zero.  If the data set
     *  does not exist, create it.  The fourth argument indicates
     *  whether the point should be connected by a line to the previous
     *  point.  Regardless of the value of this argument, a line will not
     *  drawn if either there has been no previous point for this dataset
     *  or setConnected() has been called with a false argument.
     *  <p>
     *  In order to work well with swing and be thread safe, this method
     *  actually defers execution to the event dispatch thread, where
     *  all user interface actions are performed.  Thus, the point will
     *  not be added immediately (unless you call this method from within
     *  the event dispatch thread). All the methods that do this deferring
     *  coordinate so that they are executed in the order that you
     *  called them.
     *
     *  @param dataset The data set index.
     *  @param x The X positions of the points.
     *  @param y The Y positions of the points.
     *  @param connected If true, a line is drawn to connect to the previous
     *   point.
     */
    public void addPoint(final int dataset, final double x,
            final double y, final boolean connected) {
        addPoint(dataset, x, y, connected, true, false);
    }

    public void addPoint(final int dataset, final double x,
                         final double y, final boolean connected,
                         final boolean drawit) {
        addPoint(dataset, x, y, connected, drawit, false);
    }

    /**
     * @param dataset The number of the dataset to put the point in.
     * @param x The x coordinate of the point.
     * @param y The y coordinate of the point.
     * @param connected If true, draw connecting line to the previous point.
     * @param drawit If true, new point will be drawn immediately.
     * @param fix If true, don't let the window walk when point is drawn.
     */
    public synchronized void addPoint(final int dataset, final double x,
                                      final double y, final boolean connected,
                                      final boolean drawit, final boolean fix) {
        //if (drawit) {
            Runnable doAddPoint = new Runnable() {
                public void run() {
                    _addPoint(dataset, x, y, 0, 0,
                              connected, false, drawit, fix);
                }
            };
            _deferIfNecessary(doAddPoint);
        //} else {
        //    _addPoint(dataset, x, y, 0, 0, connected, false, drawit, fix);
        //}
    }

    /** In the specified data set, add the specified x, y point to the
     *  plot with error bars.  Data set indices begin with zero.  If
     *  the dataset does not exist, create it.  yLowEB and
     *  yHighEB are the lower and upper error bars.  The sixth argument
     *  indicates whether the point should be connected by a line to
     *  the previous point.
     *  The new point will be made visible if the plot is visible
     *  on the screen.  Otherwise, it will be drawn the next time the plot
     *  is drawn on the screen.
     *  This method is based on a suggestion by
     *  Michael Altmann <michael@email.labmed.umn.edu>.
     *  <p>
     *  In order to work well with swing and be thread safe, this method
     *  actually defers execution to the event dispatch thread, where
     *  all user interface actions are performed.  Thus, the point will
     *  not be added immediately (unless you call this method from within
     *  the event dispatch thread).  All the methods that do this deferring
     *  coordinate so that they are executed in the order that you
     *  called them.
     *
     *  @param dataset The data set index.
     *  @param x The X position of the new point.
     *  @param y The Y position of the new point.
     *  @param yLowEB The low point of the error bar.
     *  @param yHighEB The high point of the error bar.
     *  @param connected If true, a line is drawn to connect to the previous
     *   point.
     */
    public synchronized void addPointWithErrorBars(final int dataset,
            final double x, final double y, final double yLowEB,
            final double yHighEB, final boolean connected) {
        Runnable doAddPoint = new Runnable() {
            public void run() {
                _addPoint(dataset, x, y, yLowEB, yHighEB, connected,
                          true, true, false);
            }
        };
        _deferIfNecessary(doAddPoint);
    }

    public void addPoly(double x0, double x1, double dx,
                         double offset, double[] a, int dataset) {
        int jLast = (int)(((x1 + dx / 2) - x0) / dx);
        double x = x0;
        for (int j = 0; j <= jLast; x += dx, j++) {
            double y = 0;
            for (int i = a.length - 1; i >= 0; i--) {
                y = ((x - offset) / dx) * y + a[i];
            }
            addPoint(dataset, x, y, j > 0, true, true);
        }
    }

    public void addPoly(int dataset, int npts, double x0, double x1,
                        double offset, double[] a) {
        
        double dx = (npts < 2) ? (x1 - x0) : (x1 - x0) / (npts - 1);
        double x = x0;
        for (int j = 0; j <= npts; x += dx, j++) {
            double y = 0;
            for (int i = a.length - 1; i >= 0; i--) {
                y = x * y + a[i];
            }
            addPoint(dataset, x + offset, y, j > 0, true, true);
        }
    }

    /** Clear the plot of all data points.  If the argument is true, then
     *  reset all parameters to their initial conditions, including
     *  the persistence, plotting format, and axes formats.
     *  For the change to take effect, you must call repaint().
     *  @param format If true, clear the format controls as well.
     *  <p>
     *  In order to work well with swing and be thread safe, this method
     *  actually defers execution to the event dispatch thread, where
     *  all user interface actions are performed.  Thus, the clear will
     *  not be executed immediately (unless you call this method from within
     *  the event dispatch thread).  All the methods that do this deferring
     *  coordinate so that they are executed in the order that you
     *  called them.
     */
    public synchronized void clear(final boolean format) {
        ///System.out.println("clear("+format+")");/*CMP*/
        Runnable doClear = new Runnable() {
            public void run() {
                _clear(format);
            }
        };
        _deferIfNecessary(doClear);
    }

    /** Clear the plot of data points in the specified dataset.
     *  This does not call repaint() to request an update of the display.
     *
     *  @param dataset The dataset to clear.
     */
    public synchronized void clear(final int dataset) {
        _clear(dataset);
    }

    /** Erase the point at the given index in the given dataset.  If
     *  lines are being drawn, also erase the line to the next points
     *  (note: not to the previous point).  The point is not checked to
     *  see whether it is in range, so care must be taken by the caller
     *  to ensure that it is.
     *  <p>
     *  In order to work well with swing and be thread safe, this method
     *  actually defers execution to the event dispatch thread, where
     *  all user interface actions are performed.  Thus, the point will
     *  not be erased immediately (unless you call this method from within
     *  the event dispatch thread).  All the methods that do this deferring
     *  coordinate so that they are executed in the order that you
     *  called them.
     *
     *  @param dataset The data set index.
     *  @param index The index of the point to erase.
     */
    public synchronized void erasePoint(final int dataset, final int index) {
        Runnable doErasePoint = new Runnable() {
            public void run() {
                _erasePoint(dataset, index);
            }
        };
        _deferIfNecessary(doErasePoint);
    }

    public void addIcon(Icon icon, double x, double y) {
        _icons.add(new PlotIcon(icon, x, y));
        repaint();
    }

    public List<PlotIcon> getIcons() {
        return _icons;
    }

    public void setIcons(List<PlotIcon> icons) {
        _icons = icons;
    }

    public void clearIcons() {
        _icons.clear();
    }

    /**
     * Add a circle to the plot at a given layer.
     * @param x X position of the circle's center in user units.
     * @param y Y position of the circle's center in user units.
     * @param r Radius of the circle in user units.
     * @param color The color of the circle.
     * @param layer The layer of the circle.
     * @return The Circle object that was added.
     */
    public Circle addCircle(double x, double y, double r, Color color, int layer) {
        Circle circle = new Circle(x, y, r, color, layer);
        _circles.add(circle);
        repaint();
        return circle;
    }

    /**
     * Add a circle to the plot.
     * @param x X position of the circle's center in user units.
     * @param y Y position of the circle's center in user units.
     * @param r Radius of the circle in user units.
     * @param color The color of the circle.
     * @return The Circle object that was added.
     */
    public Circle addCircle(double x, double y, double r, Color color) {
        Circle circle = new Circle(x, y, r, color);
        _circles.add(circle);
        repaint();
        return circle;
    }

    /**
     * Add an arc of a circle to the plot.
     * @param x X position of the circle's center in user units.
     * @param y Y position of the circle's center in user units.
     * @param r Radius of the circle in user units.
     * @param theta0 The angle to the start of the arc in radians.
     * @param deltaTheta The length of the arc in radians.
     * @param color The color of the circle.
     * @param layer  0 => on top of data, 100 => behind data. (Other values
         * are not drawn.)
     * @return The Circle object that was added.
     */
    public Circle addCircle(double x, double y, double r, double theta0,
                            double deltaTheta, Color color, int layer) {
        Circle circle = new Circle(x, y, r, theta0, deltaTheta, color, layer);
        _circles.add(circle);
        repaint();
        return circle;
    }

    /**
     * Delete the given circle from the plot.
     * Usage: <pre>
     * Plot.Circle circle = plot.addCircle(0, 0, 1, color);
     * plot.deleteCircle(circle);
     * </pre>
     * @param circle The circle to delete.
     */
    public void deleteCircle(Circle circle) {
        _circles.remove(circle);
    }

    /**
     * Delete all circles from the plot.
     */
    public void clearCircles() {
        _circles.clear();
    }

    /**
     * Delete all the circles of a given color from the plot.
     * @param color The color of circle to get rid of.
     */
    public void clearCircles(Color color) {
        Iterator<Circle> itr = _circles.iterator();
        while(itr.hasNext()) {
            Circle circ = itr.next();
            if (circ._color.equals(color)) {
                itr.remove();
            }
        }
    }

    /** Rescale so that the data that is currently plotted just fits.
     *  This overrides the base class method to ensure that the protected
     *  variables _xBottom, _xTop, _yBottom, and _yTop are valid.
     *  This method calls repaint(), which eventually causes the display
     *  to be updated.
     *  <p>
     *  In order to work well with swing and be thread safe, this method
     *  actually defers execution to the event dispatch thread, where
     *  all user interface actions are performed.  Thus, the fill will
     *  not occur immediately (unless you call this method from within
     *  the event dispatch thread).  All the methods that do this deferring
     *  coordinate so that they are executed in the order that you
     *  called them.
     */
    public synchronized void fillPlot() {
        Runnable doFill = new Runnable() {
            public void run() {
                _fillPlot();
            }
        };
        _deferIfNecessary(doFill);
    }

    /** Return whether the default is to connect
     *  subsequent points with a line.  If the result is false, then
     *  points are not connected.  When points are by default
     *  connected, individual points can be not connected by giving the
     *  appropriate argument to addPoint().  Also, a different default
     *  can be set for each dataset, overriding this global default.
     * @return Returns true if points are connected by default.
     */
    public boolean getConnected() {
        return _connected;
    }

    /** Return whether a line will be drawn from any
     *  plotted point down to the x axis.
     *  A plot with such lines is also known as a stem plot.
     * @return Returns true if impulses (stems) are drawn.
     */
    public boolean getImpulses() {
        return _impulses;
    }

    /** Get the marks style, which is one of
     *  "none", "points", "dots", or "various".
     *  @return A string specifying the style for points.
     */
    public synchronized String getMarksStyle() {
        // NOTE: If the number of marks increases, we will need to do
        // something better here...
        if (_marks == 0) {
            return "none";
        } else if (_marks == 1) {
            return "points";
        } else if (_marks == 2) {
            return "dots";
        } else if (_marks == 3) {
            return "various";
        } else {
            return "pixels";
        }
    }

    /** Return the actual number of data sets.
     *  @return The number of data sets that have been created.
     */
    public synchronized int getNumDataSets() {
        return _points.size();
    }

    /**
     * Get the maximum X value in any data set.
     * @return The maximum value.
     */
    public double getMaxX() {
        if (_xyInvalid) {
            double[] xr = getXRange();
            return xr[1];
        } else {
            return _xTop;
        }
    }

    /**
     * Set false to cause all data to be rechecked for max/min values
     * before auto-scaling.
     * @param b True if the max and min of the data values are up to date.
     */
    public void setScalingValid(boolean b) {
        _xyInvalid = !b;
    }

    /** Get the interpolated Y-value of the data at a given X-coordinate.
     *  Uses linear interpolation between data points.
     *  For extrapolation, returns the value of the first or last point.
     *  If there is no data, returns zero.
     *  <p> Note: Currently assumes the data is sorted in order of increasing
     *  X-coordinate.
     *  @param x The X-coordinate
     *  @param dataset Which dataset to use
     *  @return The data value
     */
    public synchronized double getDataValueAt(double x, int dataset) {
        if (_points == null || _points.size() <= dataset) {
            return 0;
        }
        ArrayList<PlotPoint> pts = _points.get(dataset);
        int index = getIndexOfXCoordinate(x, pts);
        int n;
        if (pts == null || (n = pts.size()) == 0) {
            return 0;
        }
        if (index < 0) {
            return (pts.get(0)).y;
        } else if (index >= n - 1) {
            return (pts.get(n - 1)).y;
        } else {
            PlotPoint pt1 = pts.get(index);
            PlotPoint pt2 = pts.get(index + 1);
            double x1 = pt1.originalx;
            double x2 = pt2.originalx;
            if (x1 == x2) {
                return pt1.y;
            }
            return pt1.y + (x - x1) * (pt2.y - pt1.y) / (x2 - x1);
        }
    }

    /** Get the X and Y-values of a datum at a given index.
     *  Zero is the index of the first point.
     *  A negative index counts from the last point forward, where
     *  the last point is -1.
     *  If the index refers to a non-existent point, returns the
     *  coordinates of the closest end-point.
     *  If there are no data points, returns zeros.
     *  @param index The index of the desired point
     *  @param dataset Which dataset to use
     *  @return A double[2], where rtn[0]=x and rtn[1]=y
     */
    protected synchronized double[] getCoordsOfPoint(int index, int dataset) {
        double[] rtn = new double[2];
        if (_points.size() <= dataset) {
            return rtn;         // Return zeros
        }
        ArrayList<PlotPoint> pts = _points.get(dataset);
        int n;
        if ((n = pts.size()) == 0 || n == 0) {
            return rtn;         // Return zeros
        }
        if (index < 0) {
            index += n;
        }
        if (index < 0) {
            index = 0;
        } else if (index > n - 1) {
            index = n - 1;
        }
        rtn[0] = (pts.get(index)).originalx;
        rtn[1] = (pts.get(index)).y;
        return rtn;
    }

    protected int getIndexOfXCoordinate(double x, ArrayList<PlotPoint> pts) {
        int top = pts.size();
        int bottom = -1;
        int middle;

        while (top - bottom > 1) {
            middle = (top + bottom) / 2;
            if (x > (pts.get(middle)).originalx) {
                bottom = middle;
            } else {
                top = middle;
            }
        }
        return bottom;
    }

    /**
         * Whether the given data set is to be used for auto scaling.
         * @param index The index of the data set.
         * @return Returns true iff the data set exists.
         */
    protected boolean isScalingDataSet(int index) {
        boolean rtn = false;
        if (isShown(index)) {
            if (m_scalingDataSets == null) {
                rtn = (index >= 0 && index < getNumDataSets());
            } else {
                int len = m_scalingDataSets.length;
                for (int i = 0; i < len; i++) {
                    if (m_scalingDataSets[i] == index) {
                        rtn = true;
                    }
                }
            }
        }
        return rtn;
    }

    /**
         * Get the next data set to use for auto scaling.
         * @param index The index of the previous data set. Use -1
         * to get the first valid data set.
         * @return Returns the index of the next valid data set after
         * the given index, or -1 if there are no more.
         */
    protected int getNextScalingDataSet(int index) {
        int rtn = -1;
        if (m_scalingDataSets == null) {
            if (index < -1) {
                index = -1;
            }
            while (++index < getNumDataSets()) {
                if (isShown(index)) {
                    rtn = index;
                    break;
                }
            }
        } else {
            int len = m_scalingDataSets.length;
            // Initialize to current entry in list
            int i = len;
            if (index < 0) {
                i = 0;
            } else {
                // Find where we are now
                for (i = 0;
                     i < len && m_scalingDataSets[i] != index;
                     i++);
                i++;          // Go to first potential answer
            }
            // Go to next displayed data set in list
            for ( ; i < len && !isShown(m_scalingDataSets[i]); i++);
            rtn = i < len ? m_scalingDataSets[i] : -1;
        }
        return rtn;
    }

    /**
         * Specify which data sets should be used for vertical scaling
         * with the cursor.
         * @param indices A vector containing the indices of the datasets
         * to use.  Specify null to use all non-empty datasets.
         */
    public void setScalingDataSets(int[] indices) {
        m_scalingDataSets = indices;
    }

    /** Read a file with the old syntax (non-XML).
     *  Override the base class to register that we are reading a new
     *  data set.
     *  @param in The input stream.
     *  @exception IOException If the stream cannot be read.
     */
    public synchronized void read(InputStream in) throws IOException {
        super.read(in);
        _firstinset = true;
        _sawfirstdataset = false;
    }

    /** Create a sample plot.  This is not actually done immediately
     *  unless the calling thread is the event dispatch thread.
     *  Instead, it is deferred to the event dispatch thread.
     *  It is important that the calling thread not hold a synchronize
     *  lock on the Plot object, or deadlock will result (unless the
     *  calling thread is the event dispatch thread).
     */
    public synchronized void samplePlot() {
        // This needs to be done in the event thread.
        Runnable sample = new Runnable() {
            public void run() {
                synchronized (Plot.this) {
                    // Create a sample plot.
                    clear(true);

                    setTitle("Sample plot");
                    setYRange(-4, 4);
                    setXRange(0, 100);
                    setXLabel("time");
                    setYLabel("value");
                    addYTick("-PI", -Math.PI);
                    addYTick("-PI/2", -Math.PI/2);
                    addYTick("0", 0);
                    addYTick("PI/2", Math.PI/2);
                    addYTick("PI", Math.PI);
                    setMarksStyle("none");
                    setImpulses(true);

                    boolean first = true;
                    for (int i = 0; i <= 100; i++) {
                        double xvalue = (double)i;

                        // NOTE: jdk 1.3beta has a bug exhibited here.
                        // The value of the second argument in the calls
                        // to addPoint() below is corrupted the second
                        // time that this method is called.  The print
                        // statement below shows that the value is
                        // correct before the call.
                        // System.out.println("x value: " + xvalue);
                        // For some bizarre reason, this problem goes
                        // away when this code is executed in the event
                        // dispatch thread.

                        addPoint(0, xvalue,
                                5 * Math.cos(Math.PI * i/20), !first);
                        addPoint(1, xvalue,
                                4.5 * Math.cos(Math.PI * i/25), !first);
                        addPoint(2, xvalue,
                                4 * Math.cos(Math.PI * i/30), !first);
                        addPoint(3, xvalue,
                                3.5* Math.cos(Math.PI * i/35), !first);
                        addPoint(4, xvalue,
                                3 * Math.cos(Math.PI * i/40), !first);
                        addPoint(5, xvalue,
                                2.5 * Math.cos(Math.PI * i/45), !first);
                        addPoint(6, xvalue,
                                2 * Math.cos(Math.PI * i/50), !first);
                        addPoint(7, xvalue,
                                1.5 * Math.cos(Math.PI * i/55), !first);
                        addPoint(8, xvalue,
                                1 * Math.cos(Math.PI * i/60), !first);
                        addPoint(9, xvalue,
                                0.5 * Math.cos(Math.PI * i/65), !first);
                        first = false;
                    } // for
                } // synchronized
                repaint();
            } // run method
        }; // Runnable class
        _deferIfNecessary(sample);
    }

    /** Turn bars on or off (for bar charts).  Note that this is a global
     *  property, not per dataset.
     *  @param b If true, turn bars on.
     */
    public void setBars(boolean b) {
        _bars = b;
    }

    /**
     * Select whether automatic scaling is forced to put the y=0 axis
     * on scale.
     *  @param b If true, include y=0.
     */
    public void setShowZeroY(boolean b) {
        if (b != _spanZero) {
            //_xyInvalid = true;
            _spanZero = b;
        }
    }

    /** Turn bars on and set the width and offset.  Both are specified
     *  in units of the x axis.  The offset is the amount by which the
     *  i < sup>th</sup> data set is shifted to the right, so that it
     *  peeks out from behind the earlier data sets.
     *  @param width The width of the bars.
     *  @param offset The offset per data set.
     */
    public synchronized void setBars(double width, double offset) {
        ///System.out.println("setBars("+width+", "+offset+")");/*CMP*/
        _barwidth = width;
        _baroffset = offset;
        _bars = true;
    }

    /** If the argument is true, then the default is to connect
     *  subsequent points with a line.  If the argument is false, then
     *  points are not connected.  When points are by default
     *  connected, individual points can be not connected by giving the
     *  appropriate argument to addPoint().  Also, a different default
     *  can be set for each dataset, overriding this global default.
     * @param on If true, points are connected by default.
     */
    public void setConnected(boolean on) {
        _connected = on;
    }

    /** If the first argument is true, then by default for the specified
     *  dataset, points will be connected by a line.  Otherwise, the
     *  points will not be connected. When points are by default
     *  connected, individual points can be not connected by giving the
     *  appropriate argument to addPoint().
     *  @param on If true, draw lines between points.
     *  @param dataset The dataset to which this should apply.
     */
    public synchronized void setConnected(boolean on, int dataset) {
        _checkDatasetIndex(dataset);
        Format fmt = _formats.get(dataset);
        fmt.connected = on;
        fmt.connectedUseDefault = false;
    }

    /** If the argument is true, then a line will be drawn from any
     *  plotted point down to the x axis.  Otherwise, this feature is
     *  disabled.  A plot with such lines is also known as a stem plot.
     *  @param on If true, draw a stem plot.
     */
    public synchronized void setImpulses(boolean on) {
        _impulses = on;
    }

    /**
     * Set the width of all impulse stems. Default is 1 pixel.
     * Width will be forced to be a positive odd integer.
     * @param width The width in pixels.
     */
    public synchronized void setImpulseWidth(int width) {
        width = Math.max(1, width); // Must be non-negative
        width |= 1; // Must be odd
        _impulseWidth = width;
    }

    /** If the first argument is true, then a line will be drawn from any
     *  plotted point in the specified dataset down to the x axis.
     *  Otherwise, this feature is
     *  disabled.  A plot with such lines is also known as a stem plot.
     *  @param on If true, draw a stem plot.
     *  @param dataset The dataset to which this should apply.
     */
    public synchronized void setImpulses(boolean on, int dataset) {
        _checkDatasetIndex(dataset);
        Format fmt = _formats.get(dataset);
        fmt.impulses = on;
        fmt.impulsesUseDefault = false;
    }

    /** Set the marks style to "none", "points", "dots", or "various".
     *  In the last case, unique marks are used for the first ten data
     *  sets, then recycled.
     *  @param style A string specifying the style for points.
     */
    public synchronized void setMarksStyle(String style) {
        if (style.equalsIgnoreCase("none")) {
            _marks = 0;
        } else if (style.equalsIgnoreCase("points")) {
            _marks = 1;
        } else if (style.equalsIgnoreCase("dots")) {
            _marks = 2;
        } else if (style.equalsIgnoreCase("various")) {
            _marks = 3;
        } else if (style.equalsIgnoreCase("pixels")) {
            _marks = 4;
        }
        repaint();
    }

    /** Set the marks style to "none", "points", "dots", "various",
     *  or "pixels" for the specified dataset.
     *  In the last case, unique marks are used for the first ten data
     *  sets, then recycled.
     *  @param style A string specifying the style for points.
     *  @param dataset The dataset to which this should apply.
     */
    public synchronized void setMarksStyle(String style, int dataset) {
        _checkDatasetIndex(dataset);
        Format fmt = _formats.get(dataset);
        if (style.equalsIgnoreCase("none")) {
            fmt.marks = 0;
        } else if (style.equalsIgnoreCase("points")) {
            fmt.marks = 1;
        } else if (style.equalsIgnoreCase("dots")) {
            fmt.marks = 2;
        } else if (style.equalsIgnoreCase("various")) {
            fmt.marks = 3;
        } else if (style.equalsIgnoreCase("pixels")) {
            fmt.marks = 4;
        }
        fmt.marksUseDefault = false;
    }

    /** Calling this method with a positive argument sets the
     *  persistence of the plot to the given number of points.  Calling
     *  with a zero argument turns off this feature, reverting to
     *  infinite memory (unless sweeps persistence is set).  If both
     *  sweeps and points persistence are set then sweeps take
     *  precedence.
     *  <p>
     *  Setting the persistence greater than zero forces the plot to
     *  be drawn in XOR mode, which allows points to be quickly and
     *  efficiently erased.  However, there is a bug in Java (as of
     *  version 1.3), where XOR mode does not work correctly with
     *  double buffering.  Thus, if you call this with an argument
     *  greater than zero, then we turn off double buffering for this
     *  panel <i>and all of its parents</i>.  This actually happens
     *  on the next call to addPoint().
     * @param persistence The persistence value.
     */
    public void setPointsPersistence(int persistence) {
        // NOTE: No file format.  It's not clear it makes sense to have one.
        _pointsPersistence = persistence;
    }

    public void setPoints(ArrayList<ArrayList<PlotPoint>> points) {
        _points = points;
        // TODO: Initilization
    }

    public ArrayList<ArrayList<PlotPoint>> getPoints() {
        return _points;
    }

    public void setFormats(ArrayList<Format> formats) {
        _formats = formats;
    }

    public ArrayList<Format> getFormats() {
        return _formats;
    }

    public void setPrevx(ArrayList<Long> prevx) {
        _prevx = prevx;
    }

    public ArrayList<Long> getPrevx() {
        return _prevx;
    }

    public void setPrevy(ArrayList<Long> prevy) {
        _prevy = prevy;
    }

    public ArrayList<Long> getPrevy() {
        return _prevy;
    }

    /** If the argument is true, then datasets with the same name
     *  are merged into a single dataset.
     *  @param on If true, then merge datasets.
     */
    public void setReuseDatasets(boolean on) {
        _reusedatasets = on;
    }

    /** Calling this method with a positive argument sets the
     *  persistence of the plot to the given width in units of the
     *  horizontal axis. Calling
     *  with a zero argument turns off this feature, reverting to
     *  infinite memory (unless points persistence is set).  If both
     *  X and points persistence are set then both are applied,
     *  meaning that points that are old by either criterion will
     *  be erased.
     *  <p>
     *  Setting the X persistence greater than zero forces the plot to
     *  be drawn in XOR mode, which allows points to be quickly and
     *  efficiently erased.  However, there is a bug in Java (as of
     *  version 1.3), where XOR mode does not work correctly with
     *  double buffering.  Thus, if you call this with an argument
     *  greater than zero, then we turn off double buffering for this
     *  panel <i>and all of its parents</i>.  This actually happens
     *  on the next call to addPoint().
     * @param persistence The persistence value.
     */
    public void setXPersistence(double persistence) {
        // NOTE: No file format.  It's not clear it makes sense to have one.
        _xPersistence = persistence;
        _xorMode = persistence > 0;
    }

    /** Write plot data information to the specified output stream in PlotML.
     *  @param output A buffered print writer.
     */
    public synchronized void writeData(PrintWriter output) {
        super.writeData(output);
        for (int dataset = 0; dataset < _points.size(); dataset++) {

            StringBuffer options = new StringBuffer();

            Format fmt = _formats.get(dataset);

            if (!fmt.connectedUseDefault) {
                if (_isConnected(dataset)) {
                    options.append(" connected=\"yes\"");
                } else {
                    options.append(" connected=\"no\"");
                }
            }

            if (!fmt.impulsesUseDefault) {
                if (fmt.impulses) options.append(" stems=\"yes\"");
                else output.println(" stems=\"no\"");
            }

            if (!fmt.marksUseDefault) {
                switch(fmt.marks) {
                case 0:
                    options.append(" marks=\"none\"");
                    break;
                case 1:
                    options.append(" marks=\"points\"");
                    break;
                case 2:
                    options.append(" marks=\"dots\"");
                    break;
                case 3:
                    options.append(" marks=\"various\"");
                    break;
                case 4:
                    options.append(" marks=\"pixels\"");
                    break;
                }
            }

            String legend = getLegend(dataset);
            if (legend != null) {
                options.append(" name=\"" + getLegend(dataset) + "\"");
            }

            output.println("<dataset" + options.toString() + ">");

            // Write the data
            ArrayList<PlotPoint> pts = _points.get(dataset);
            for (int pointnum = 0; pointnum < pts.size(); pointnum++) {
                PlotPoint pt = pts.get(pointnum);
                if (!pt.connected) {
                    output.print("<m ");
                } else {
                    output.print("<p ");
                }
                output.print("x=\"" + pt.x + "\" y=\"" + pt.y + "\"");
                if (pt.errorBar) {
                    output.print(" lowErrorBar=\"" + pt.yLowEB
                            + "\" highErrorBar=\"" + pt.yHighEB + "\"");
                }
                output.println("/>");
            }

            output.println("</dataset>");
        }
    }

    /** Write plot format information to the specified output stream in
     *  PlotML, an XML scheme.
     *  @param output A buffered print writer.
     */
    public synchronized void writeFormat(PrintWriter output) {
        super.writeFormat(output);

        if (_reusedatasets) output.println("<reuseDatasets/>");

        StringBuffer defaults = new StringBuffer();

        if (!_connected) defaults.append(" connected=\"no\"");

        switch(_marks) {
        case 1:
            defaults.append(" marks=\"points\"");
            break;
        case 2:
            defaults.append(" marks=\"dots\"");
            break;
        case 3:
            defaults.append(" marks=\"various\"");
            break;
        case 4:
            defaults.append(" marks=\"pixels\"");
            break;
        }

        // Write the defaults for formats that can be controlled by dataset
        if (_impulses) defaults.append(" stems=\"yes\"");

        if (defaults.length() > 0) {
            output.println("<default" + defaults.toString() + "/>");
        }

        if (_bars) output.println(
                "<barGraph width=\"" + _barwidth
                + "\" offset=\"" + _baroffset + "\"/>");
    }

    ///////////////////////////////////////////////////////////////////
    ////                         protected methods                 ////


    /** Check the argument to ensure that it is a valid data set index.
     *  If it is less than zero, throw an IllegalArgumentException (which
     *  is a runtime exception).  If it does not refer to an existing
     *  data set, then fill out the _points Vector so that it does refer
     *  to an existing data set. All other dataset-related vectors are
     *  similarly filled out.
     *  @param dataset The data set index.
     */
    protected synchronized void _checkDatasetIndex(int dataset) {
        if (dataset < 0) {
            throw new IllegalArgumentException("Plot._checkDatasetIndex: Cannot"
                    + " give a negative number for the data set index.");
        }
        while (dataset >= _points.size()) {
            _points.add(new ArrayList<PlotPoint>());
            _formats.add(new Format());
            _prevx.add(new Long(0));
            _prevy.add(new Long(0));
        }
    }

    /**
     *  Draw bar from the specified point to the x axis, or, if the Y
     *  coordinate is on a log scale, to the bottom of the plot.  If
     *  the specified point is below the x axis or outside the x
     *  range, do nothing.  If the <i>clip</i> argument is true, then
     *  do not draw above the y range.
     *  Note that paintComponent() should be called before calling
     *  this method so that _xscale and _yscale are properly set.
     *  This method should be called only from the event dispatch
     *  thread.  It is not synchronized, so its caller should be.
     *  @param graphics The graphics context.
     *  @param dataset The index of the dataset.
     *  @param xpos The x position.
     *  @param ypos The y position.
     *  @param clip If true, then do not draw outside the range.
     */
    protected void _drawBar(Graphics graphics, int dataset,
            long xpos, long ypos, boolean clip) {
        if (clip) {
            if (ypos < _uly) {
                ypos = _uly;
            } if (ypos > _lry) {
                ypos = _lry;
            }
        }
        if (ypos <= _lry && xpos <= _lrx && xpos >= _ulx) {
            // left x position of bar.
            int barlx = (int)(xpos - _barwidth * _xscale/2 +
                    dataset * _baroffset * _xscale + 0.5);
            // right x position of bar -- round up
            int barrx = (int)(barlx + _barwidth * _xscale + 1.0);
            if (barlx < _ulx) barlx = _ulx;
            if (barrx > _lrx) barrx = _lrx;
            // Make sure that a bar is always at least one pixel wide.
            if (barlx >= barrx) barrx = barlx+1;
            // The y position of the zero line.
            long zeroypos;
            if (_ylog) {
                zeroypos = _lry - (long)(_yscale * (_yBottom - _yMin));
            } else {
                zeroypos = _lry - (long)(_yscale * (0 - _yMin));
            }
            if (_lry < zeroypos) zeroypos = _lry;
            if (_uly > zeroypos) zeroypos = _uly;

            if (_yMin >= 0 || ypos <= zeroypos) {
                graphics.fillRect(barlx, (int)ypos,
                        barrx - barlx, (int)(zeroypos - ypos));
            } else {
                graphics.fillRect(barlx, (int)zeroypos,
                        barrx - barlx, (int)(ypos - zeroypos));
            }
        }
    }

    /** Draw an error bar for the specified yLowEB and yHighEB values.
     *  If the specified point is below the y axis or outside the
     *  x range, do nothing.  If the <i>clip</i> argument is true,
     *  then do not draw above the y range.
     *  This method should be called only from the event dispatch thread.
     *  It is not synchronized, so its caller should be.
     *  @param graphics The graphics context.
     *  @param dataset The index of the dataset.
     *  @param xpos The x position.
     *  @param yLowEBPos The lower y position of the error bar.
     *  @param yHighEBPos The upper y position of the error bar.
     *  @param clip If true, then do not draw above the range.
     */
    protected void _drawErrorBar(Graphics graphics, int dataset,
            long xpos, long yLowEBPos, long yHighEBPos,
            boolean clip) {
        _drawLine(graphics, dataset, xpos - _ERRORBAR_LEG_LENGTH, yHighEBPos,
                xpos + _ERRORBAR_LEG_LENGTH, yHighEBPos, clip);
        _drawLine(graphics, dataset, xpos, yLowEBPos, xpos, yHighEBPos, clip);
        _drawLine(graphics, dataset, xpos - _ERRORBAR_LEG_LENGTH, yLowEBPos,
                xpos + _ERRORBAR_LEG_LENGTH, yLowEBPos, clip);
    }

    /** Draw a line from the specified point to the y axis.
     *  If the specified point is below the y axis or outside the
     *  x range, do nothing.  If the <i>clip</i> argument is true,
     *  then do not draw above the y range.
     *  This method should be called only from the event dispatch thread.
     *  It is not synchronized, so its caller should be.
     *  @param graphics The graphics context.
     *  @param xpos The x position.
     *  @param ypos The y position.
     *  @param clip If true, then do not draw outside the range.
     */
    protected void _drawImpulse(Graphics graphics,
            long xpos, long ypos, boolean clip) {
        if (clip) {
            if (ypos < _uly) {
                ypos = _uly;
            } if (ypos > _lry) {
                ypos = _lry;
            }
        }
        if (ypos <= _lry && xpos <= _lrx && xpos >= _ulx) {
            // The y position of the zero line.
            double zeroypos = _lry - (long) ((0-_yMin) * _yscale);
            if (_lry < zeroypos) zeroypos = _lry;
            if (_uly > zeroypos) zeroypos = _uly;
            int leftSide = (int)(xpos - (_impulseWidth - 1) / 2);
            graphics.fillRect(leftSide, (int)ypos, _impulseWidth,
                              (int)(zeroypos - ypos + 1));
        }
    }

    /** Draw a line from the specified starting point to the specified
     *  ending point.  The current color is used.  If the <i>clip</i> argument
     *  is true, then draw only that portion of the line that lies within the
     *  plotting rectangle.
     *  This method should be called only from the event dispatch thread.
     *  It is not synchronized, so its caller should be.
     *  @param graphics The graphics context.
     *  @param dataset The index of the dataset.
     *  @param startx The starting x position.
     *  @param starty The starting y position.
     *  @param endx The ending x position.
     *  @param endy The ending y position.
     *  @param clip If true, then do not draw outside the range.
     */
    protected void _drawLine(Graphics graphics,
            int dataset, long startx, long starty, long endx, long endy,
            boolean clip) {

        if (clip) {
            // Rule out impossible cases.
            if (!((endx <= _ulx && startx <= _ulx) ||
                    (endx >= _lrx && startx >= _lrx) ||
                    (endy <= _uly && starty <= _uly) ||
                    (endy >= _lry && starty >= _lry))) {
                // If the end point is out of x range, adjust
                // end point to boundary.
                // The integer arithmetic has to be done with longs so as
                // to not loose precision on extremely close zooms.
                if (startx != endx) {
                    if (endx < _ulx) {
                        endy = (int)(endy + ((long)(starty - endy) *
                                (_ulx - endx))/(startx - endx));
                        endx = _ulx;
                    } else if (endx > _lrx) {
                        endy = (int)(endy + ((long)(starty - endy) *
                                (_lrx - endx))/(startx - endx));
                        endx = _lrx;
                    }
                }

                // If end point is out of y range, adjust to boundary.
                // Note that y increases downward
                if (starty != endy) {
                    if (endy < _uly) {
                        endx = (int)(endx + ((long)(startx - endx) *
                                (_uly - endy))/(starty - endy));
                        endy = _uly;
                    } else if (endy > _lry) {
                        endx = (int)(endx + ((long)(startx - endx) *
                                (_lry - endy))/(starty - endy));
                        endy = _lry;
                    }
                }

                // Adjust current point to lie on the boundary.
                if (startx != endx) {
                    if (startx < _ulx) {
                        starty = (int)(starty + ((long)(endy - starty) *
                                (_ulx - startx))/(endx - startx));
                        startx = _ulx;
                    } else if (startx > _lrx) {
                        starty = (int)(starty + ((long)(endy - starty) *
                                (_lrx - startx))/(endx - startx));
                        startx = _lrx;
                    }
                }
                if (starty != endy) {
                    if (starty < _uly) {
                        startx = (int)(startx + ((long)(endx - startx) *
                                (_uly - starty))/(endy - starty));
                        starty = _uly;
                    } else if (starty > _lry) {
                        startx = (int)(startx + ((long)(endx - startx) *
                                (_lry - starty))/(endy - starty));
                        starty = _lry;
                    }
                }
            }

            // Are the new points in range?
            if (endx >= _ulx && endx <= _lrx &&
                    endy >= _uly && endy <= _lry &&
                    startx >= _ulx && startx <= _lrx &&
                    starty >= _uly && starty <= _lry) {
                graphics.drawLine((int)startx, (int)starty,
                        (int)endx, (int)endy);
            }
        } else {
            // draw unconditionally.
            graphics.drawLine((int)startx, (int)starty,
                    (int)endx, (int)endy);
        }
    }

    /** Draw the axes and then plot all points. If the second
     *  argument is true, clear the display first.
     *  This method is called by paintComponent().
     *  To cause it to be called you would normally call repaint(),
     *  which eventually causes paintComponent() to be called.
     *  <p>
     *  Note that this is synchronized so that points are not added
     *  by other threads while the drawing is occurring.  This method
     *  should be called only from the event dispatch thread, consistent
     *  with swing policy.
     *  @param graphics The graphics context.
     *  @param clearfirst If true, clear the plot before proceeding.
     *  @param drawRectangle The Rectangle to draw in.
     */
    protected synchronized void _drawPlot(Graphics graphics,
            boolean clearfirst, Rectangle drawRectangle) {
        if (graphics instanceof Graphics2D) {
            RenderingHints hints = _defaultPlotRenderingHints();
            ((Graphics2D)graphics).addRenderingHints(hints);
        }

        // We must call PlotBox._drawPlot() before calling _drawPlotPoint
        // so that _xscale and _yscale are set.
        super._drawPlot(graphics, clearfirst, drawRectangle);
        _drawCircles(graphics, 100); // Draw circles that are behind data

        // Plot the datasets in reverse order so that the first colors
        // appear on top.
        for (int dataset = _points.size() - 1; dataset >= 0 ; dataset--) {
            if (isShown(dataset)) {
                Format fmt = _formats.get(dataset);
                int marks = _marks;
                boolean impulses = fmt.impulsesUseDefault
                        ? _impulses : fmt.impulses;

                if (!fmt.marksUseDefault)
                    marks = fmt.marks;
                ArrayList<PlotPoint> data = _points.get(dataset);
                if (!_xorMode && !impulses && !_bars) {
                    _quickDraw(graphics, dataset, marks);
                } else {
                    int npts = data.size();
                    for (int pointnum = 0; pointnum < npts; pointnum++) {
                        _drawPlotPoint(graphics, dataset, pointnum);
                    }
                }
            }
        }
        _drawCircles(graphics, 0); // Draw circles that are in front of data
        _drawIcons(graphics); // Icons on top
        _showing = true;
    }

    protected synchronized void _drawIcons(Graphics graphics) {
        Graphics g = graphics.create();
        g.setClip(_ulx + 1, _uly + 1, _lrx - _ulx - 1, _lry - _uly - 1);
        int len = _icons.size();
        for (int i = 0; i < len; i++) {
            PlotIcon picon = _icons.get(i);
            picon.draw(g, this);
        }
    }

    protected synchronized void _drawCircles(Graphics graphics, int layer) {
        Graphics g = graphics.create();
        g.setClip(_ulx + 1, _uly + 1, _lrx - _ulx - 1, _lry - _uly - 1);
        //int len = _circles.size();
        try {
            for (int i = 0; true; i++) {
                Circle circle = _circles.get(i);
                if (circle.getLayer() == layer) {
                    circle.draw(g, this);
                }
            }
        } catch (IndexOutOfBoundsException eoobe) {
        }
    }

    /**
     * Draw one dataset as a polyline.  Points that are connected to
     * neither the preceding nor the following point will not be drawn.
     * @param graphics The graphics context.
     * @param dataset Which dataset to draw.
     */
    protected synchronized void _quickDraw(Graphics graphics,
                                           int dataset, int marks) {
        ArrayList<PlotPoint> data = _points.get(dataset);
        int npts = data.size();
        // Set the color
        if (_usecolor) {
            int color = dataset % _colors.length;
            graphics.setColor(_colors[color]);
        } else {
            graphics.setColor(_foreground);
        }

        graphics.setClip(_ulx + 1, _uly + 1, _lrx - _ulx - 1, _lry - _uly - 1);

//        // TODO: Only deal with points that will fall on the plot's x-range
        int ipoint = 0;
        int[] x = new int[npts];
        int[] y = new int[npts];
        while (ipoint < npts) {
            int pointnum;
            _appendPlotPoint(data, x, y, ipoint, ipoint, dataset);
            if (marks != 0) {
                _drawPoint(graphics, dataset, x[0], y[0], true);
            }
            for (pointnum = ipoint + 1; pointnum < npts; pointnum++) {
                if (!_appendPlotPoint(data, x, y, pointnum, ipoint, dataset)) {
                    break;
                }
                if (marks != 0) {
                    int n = pointnum - ipoint;
                    _drawPoint(graphics, dataset, x[n], y[n], true);
                }
            }
            if (pointnum - ipoint > 1) {
                graphics.drawPolyline(x, y, pointnum - ipoint);
            }
            pointnum++;
            // Skip points that won't affect the plot (not marked or connected):
            while (marks == 0 && pointnum < npts
                   && !(data.get(pointnum)).connected)
            {
                // NB: _appendPlotPoint() sets _prevx, _prevy
                _appendPlotPoint(data, x, y, pointnum, ipoint, dataset);
                pointnum++;
            }
            ipoint = pointnum - 1;
        }
    }

//    private int getIndexBelow(double x, ArrayList<PlotPoint> vec, int npts) {
//        //Messages.postDebug("getIndexBelow(): npts=" + npts);/*CMP*/
//        if (npts == 0) {
//            return 0;
//        }
//        int lo = -1;
//        int hi = npts;
//        int mid = (hi - lo) / 2;
//        while (hi - lo > 1) {
//            mid = (hi + lo) / 2;
//            /*Messages.postDebug("getIndexBelow: lo=" + lo
//                               + ", hi=" + hi
//                               + ", mid=" + mid);/*CMP*/
//            if (x >= (vec.get(mid)).x) {
//                lo = mid;
//            } else {
//                hi = mid;
//            }
//        }
//        return Math.max(lo, 0);
//    }

    /** Put a mark corresponding to the specified dataset at the
     *  specified x and y position. The mark is drawn in the current
     *  color. What kind of mark is drawn depends on the _marks
     *  variable and the dataset argument. If the fourth argument is
     *  true, then check the range and plot only points that
     *  are in range.
     *  This method should be called only from the event dispatch thread.
     *  It is not synchronized, so its caller should be.
     *  @param graphics The graphics context.
     *  @param dataset The index of the dataset.
     *  @param xpos The x position.
     *  @param ypos The y position.
     *  @param clip If true, then do not draw outside the range.
     */
    protected Dimension _drawPoint(Graphics graphics,
            int dataset, long xpos, long ypos,
            boolean clip) {

        Dimension dimension = new Dimension();
        // If the point is not out of range, draw it.
        boolean pointinside = ypos <= _lry && ypos >= _uly &&
            xpos <= _lrx && xpos >= _ulx;
        if (!clip || pointinside) {
            int xposi = (int)xpos;
            int yposi = (int)ypos;

            // Check to see whether the dataset has a marks directive
            Format fmt = _formats.get(dataset);
            int marks = _marks;
            if (!fmt.marksUseDefault) {
                marks = fmt.marks;
            }

            // If the point is out of range, and being drawn, then it is
            // probably a legend point.  When printing in black and white,
            // we want to use a line rather than a point for the legend.
            // (So that line patterns are visible). The only exception is
            // when the marks style uses distinct marks, or if there is
            // no line being drawn.
            // NOTE: It is unfortunate to have to test the class of graphics,
            // but there is no easy way around this that I can think of.
            /***if (!pointinside && marks != 3 && _isConnected(dataset) &&
                    (graphics instanceof EPSGraphics)) {
                graphics.drawLine(xposi-6, yposi, xposi+6, yposi);
            } else***/
            {
                // Color display.  Use normal legend.
                switch (marks) {
                case 0:
                    // If no mark style is given, draw a filled rectangle.
                    // This is used, for example, to draw the legend.
                    if (graphics != null) {
                        graphics.fillRect(xposi-6, yposi-4, 10, 2);
                    }
                    dimension.setSize(10, 2);
                    break;
                case 1:
                    // points -- use 3-pixel ovals.
                    if (graphics != null) {
                        graphics.fillOval(xposi-1, yposi-1, 3, 3);
                    }
                    dimension.setSize(3, 3);
                    break;
                case 2:
                    // dots
                    if (graphics != null) {
                        graphics.fillOval(xposi-_radius, yposi-_radius,
                                          _diameter, _diameter);
                    }
                    dimension.setSize(_diameter, _diameter);
                    break;
                case 3:
                    // various
                    dimension.setSize(_diameter, _diameter);
                    if (graphics == null) {
                        break;
                    }
                    int xpoints[], ypoints[];
                    // Points are only distinguished up to _MAX_MARKS data sets.
                    int mark = dataset % _MAX_MARKS;
                    switch (mark) {
                    case 0:
                        // filled circle
                        graphics.fillOval(xposi-_radius, yposi-_radius,
                                _diameter, _diameter);
                        break;
                    case 1:
                        // cross
                        graphics.drawLine(xposi-_radius, yposi-_radius,
                                xposi+_radius, yposi+_radius);
                        graphics.drawLine(xposi+_radius, yposi-_radius,
                                xposi-_radius, yposi+_radius);
                        break;
                    case 2:
                        // square
                        graphics.drawRect(xposi-_radius, yposi-_radius,
                                _diameter, _diameter);
                        break;
                    case 3:
                        // filled triangle
                        xpoints = new int[4];
                        ypoints = new int[4];
                        xpoints[0] = xposi; ypoints[0] = yposi-_radius;
                        xpoints[1] = xposi+_radius; ypoints[1] = yposi+_radius;
                        xpoints[2] = xposi-_radius; ypoints[2] = yposi+_radius;
                        xpoints[3] = xposi; ypoints[3] = yposi-_radius;
                        graphics.fillPolygon(xpoints, ypoints, 4);
                        break;
                    case 4:
                        // diamond
                        xpoints = new int[5];
                        ypoints = new int[5];
                        xpoints[0] = xposi; ypoints[0] = yposi-_radius;
                        xpoints[1] = xposi+_radius; ypoints[1] = yposi;
                        xpoints[2] = xposi; ypoints[2] = yposi+_radius;
                        xpoints[3] = xposi-_radius; ypoints[3] = yposi;
                        xpoints[4] = xposi; ypoints[4] = yposi-_radius;
                        graphics.drawPolygon(xpoints, ypoints, 5);
                        break;
                    case 5:
                        // circle
                        graphics.drawOval(xposi-_radius, yposi-_radius,
                                _diameter, _diameter);
                        break;
                    case 6:
                        // plus sign
                        graphics.drawLine(xposi, yposi-_radius, xposi,
                                yposi+_radius);
                        graphics.drawLine(xposi-_radius, yposi, xposi+_radius,
                                yposi);
                        break;
                    case 7:
                        // filled square
                        graphics.fillRect(xposi-_radius, yposi-_radius,
                                _diameter, _diameter);
                        break;
                    case 8:
                        // triangle
                        xpoints = new int[4];
                        ypoints = new int[4];
                        xpoints[0] = xposi; ypoints[0] = yposi-_radius;
                        xpoints[1] = xposi+_radius; ypoints[1] = yposi+_radius;
                        xpoints[2] = xposi-_radius; ypoints[2] = yposi+_radius;
                        xpoints[3] = xposi; ypoints[3] = yposi-_radius;
                        graphics.drawPolygon(xpoints, ypoints, 4);
                        break;
                    case 9:
                        // filled diamond
                        xpoints = new int[5];
                        ypoints = new int[5];
                        xpoints[0] = xposi; ypoints[0] = yposi-_radius;
                        xpoints[1] = xposi+_radius; ypoints[1] = yposi;
                        xpoints[2] = xposi; ypoints[2] = yposi+_radius;
                        xpoints[3] = xposi-_radius; ypoints[3] = yposi;
                        xpoints[4] = xposi; ypoints[4] = yposi-_radius;
                        graphics.fillPolygon(xpoints, ypoints, 5);
                        break;
                    }
                    break;
                case 4:
                    // If the mark style is pixels, draw a filled rectangle.
                    if (graphics != null) {
                        graphics.fillRect(xposi, yposi, 1, 1);
                    }
                    dimension.setSize(1, 1);
                    break;
                default:
                    // none
                }
            }
        }
        return dimension;
    }

    /** Parse a line that gives plotting information. Return true if
     *  the line is recognized.  Lines with syntax errors are ignored.
     *  It is not synchronized, so its caller should be.
     *  @param line A command line.
     *  @return True if the line is recognized.
     */
    protected boolean _parseLine(String line) {
        boolean connected = false;
        if (_isConnected(_currentdataset)) {
            connected = true;
        }
        // parse only if the super class does not recognize the line.
        if (super._parseLine(line)) {
            return true;
        } else {
            // We convert the line to lower case so that the command
            // names are case insensitive
            String lcLine = new String(line.toLowerCase());
            if (lcLine.startsWith("marks:")) {
                // If we have seen a dataset directive, then apply the
                // request to the current dataset only.
                String style = (line.substring(6)).trim();
                if (_sawfirstdataset) {
                    setMarksStyle(style, _currentdataset);
                } else {
                    setMarksStyle(style);
                }
                return true;
            } else if (lcLine.startsWith("numsets:")) {
                // Ignore.  No longer relevant.
                return true;
            } else if (lcLine.startsWith("reusedatasets:")) {
                if (lcLine.indexOf("off", 16) >= 0) {
                    setReuseDatasets(false);
                } else {
                    setReuseDatasets(true);
                }
                return true;
            } else if (lcLine.startsWith("dataset:")) {
                if (_reusedatasets && lcLine.length() > 0) {
                    String tlegend = (line.substring(8)).trim();
                    _currentdataset = -1;
                    int i;
                    for ( i = 0; i <= _maxdataset; i++) {
                        if (getLegend(i).compareTo(tlegend) == 0) {
                            _currentdataset = i;
                        }
                    }
                    if (_currentdataset != -1) {
                        return true;
                    } else {
                        _currentdataset = _maxdataset;
                    }
                }

                // new data set
                _firstinset = true;
                _sawfirstdataset = true;
                _currentdataset++;
                if (lcLine.length() > 0) {
                    String legend = (line.substring(8)).trim();
                    if (legend != null && legend.length() > 0) {
                        addLegend(_currentdataset, legend);
                    }
                }
                _maxdataset = _currentdataset;
                return true;
            } else if (lcLine.startsWith("lines:")) {
                if (lcLine.indexOf("off", 6) >= 0) {
                    setConnected(false);
                } else {
                    setConnected(true);
                }
                return true;
            } else if (lcLine.startsWith("impulses:")) {
                // If we have not yet seen a dataset, then this is interpreted
                // as the global default.  Otherwise, it is assumed to apply
                // only to the current dataset.
                if (_sawfirstdataset) {
                    if (lcLine.indexOf("off", 9) >= 0) {
                        setImpulses(false, _currentdataset);
                    } else {
                        setImpulses(true, _currentdataset);
                    }
                } else {
                    if (lcLine.indexOf("off", 9) >= 0) {
                        setImpulses(false);
                    } else {
                        setImpulses(true);
                    }
                }
                return true;
            } else if (lcLine.startsWith("bars:")) {
                if (lcLine.indexOf("off", 5) >= 0) {
                    setBars(false);
                } else {
                    setBars(true);
                    int comma = line.indexOf(",", 5);
                    String barwidth;
                    String baroffset = null;
                    if (comma > 0) {
                        barwidth = (line.substring(5, comma)).trim();
                        baroffset = (line.substring(comma+1)).trim();
                    } else {
                        barwidth = (line.substring(5)).trim();
                    }
                    try {
                        Double bwidth = new Double(barwidth);
                        double boffset = _baroffset;
                        if (baroffset != null) {
                            boffset = (new Double(baroffset)).
                                doubleValue();
                        }
                        setBars(bwidth.doubleValue(), boffset);
                    } catch (NumberFormatException e) {
                        // ignore if format is bogus.
                    }
                }
                return true;
            } else if (line.startsWith("move:")) {
                // a disconnected point
                connected = false;
                // deal with 'move: 1 2' and 'move:2 2'
                line = line.substring(5, line.length()).trim();
            } else if (line.startsWith("move")) {
                // a disconnected point
                connected = false;
                // deal with 'move 1 2' and 'move2 2'
                line = line.substring(4, line.length()).trim();
            } else if (line.startsWith("draw:")) {
                // a connected point, if connect is enabled.
                line = line.substring(5, line.length()).trim();
            } else if (line.startsWith("draw")) {
                // a connected point, if connect is enabled.
                line = line.substring(4, line.length()).trim();
            }
            line = line.trim();

            // We can't use StreamTokenizer here because it can't
            // process numbers like 1E-01.
            // This code is somewhat optimized for speed, since
            // most data consists of two data points, we want
            // to handle that case as efficiently as possible.

            int fieldsplit = line.indexOf(",");
            if (fieldsplit == -1) {
                fieldsplit = line.indexOf(" ");
            }
            if (fieldsplit == -1) {
                fieldsplit = line.indexOf("\t");  // a tab
            }

            if (fieldsplit > 0) {
                String x = (line.substring(0, fieldsplit)).trim();
                String y = (line.substring(fieldsplit+1)).trim();
                // Any more separators?
                int fieldsplit2 = y.indexOf(",");
                if (fieldsplit2 == -1) {
                    fieldsplit2 = y.indexOf(" ");
                }
                if (fieldsplit2 == -1) {
                    fieldsplit2 = y.indexOf("\t");  // a tab
                }
                if (fieldsplit2 > 0) {
                    line = (y.substring(fieldsplit2+1)).trim();
                    y = (y.substring(0, fieldsplit2)).trim();
                }
                try {
                    Double xpt = new Double(x);
                    Double ypt = new Double(y);
                    if (fieldsplit2 > 0) {
                        // There was one separator after the y value, now
                        // look for another separator.
                        int fieldsplit3 = line.indexOf(",");
                        if (fieldsplit3 == -1) {
                            fieldsplit3 = line.indexOf(" ");
                        }
                        if (fieldsplit3 == -1) {
                            fieldsplit2 = line.indexOf("\t");  // a tab
                        }

                        if (fieldsplit3 > 0) {
                            // We have more numbers, assume that this is
                            // an error bar
                            String yl = (line.substring(0,
                                    fieldsplit3)).trim();
                            String yh = (line.substring(fieldsplit3+1)).trim();
                            Double yLowEB = new Double(yl);
                            Double yHighEB = new Double(yh);
                            connected = _addLegendIfNecessary(connected);
                            addPointWithErrorBars(_currentdataset,
                                    xpt.doubleValue(),
                                    ypt.doubleValue(),
                                    yLowEB.doubleValue(),
                                    yHighEB.doubleValue(),
                                    connected);
                            return true;
                        } else {
                            // It is unlikely that we have a fieldsplit2 >0
                            // but not fieldsplit3 >0, but just in case:

                            connected = _addLegendIfNecessary(connected);
                            addPoint(_currentdataset, xpt.doubleValue(),
                                    ypt.doubleValue(), connected);
                            return true;
                        }
                    } else {
                        // There were no more fields, so this is
                        // a regular pt.
                        connected = _addLegendIfNecessary(connected);
                        addPoint(_currentdataset, xpt.doubleValue(),
                                ypt.doubleValue(), connected);
                        return true;
                    }
                } catch (NumberFormatException e) {
                    // ignore if format is bogus.
                }
            }
        }
        return false;
    }


    ///////////////////////////////////////////////////////////////////
    ////                         protected variables               ////

    /** @serial The current dataset. */
    protected int _currentdataset = -1;

    /** @serial Which datasets are used for autoscaling. */
    protected int[] m_scalingDataSets = null;

    /** @serial A vector of datasets. */
    protected ArrayList<ArrayList<PlotPoint>> _points
            = new ArrayList<ArrayList<PlotPoint>>();

    /** @serial An indicator of the marks style.  See _parseLine method for
     * interpretation.
     */
    protected int _marks;

    ///////////////////////////////////////////////////////////////////
    ////                         private methods                   ////

    /* Add a legend if necessary, return the value of the connected flag.
     */
    private boolean _addLegendIfNecessary(boolean connected) {
        if ((! _sawfirstdataset  || _currentdataset < 0) &&
                ! _reusedatasets) {
            // We did not set a DataSet line, but
            // we did get called with -<digit> args and
            // we did not see reusedatasets: yes
            _sawfirstdataset = true;
            _currentdataset++;
        }
        if (! _sawfirstdataset && getLegend(_currentdataset) == null) {
            // We did not see a "DataSet" string yet,
            // nor did we call addLegend().
            _firstinset = true;
            _sawfirstdataset = true;
            addLegend(_currentdataset,
                    new String("Set "+ _currentdataset));
        }
        if (_firstinset && ! _reusedatasets) {
            connected = false;
            _firstinset = false;
        }
        return connected;
    }

    /*
     * The point is drawn on the screen only if is visible.
     * Otherwise, it is drawn the next time paintComponent() is called.
     *
     * This is not synchronized, so the caller should be.  Moreover, this
     * should only be called in the event dispatch thread. It should only
     * be called via _deferIfNecessary().
     */
    protected void _setPoints(int dataset,
                              double[] xvals, double[] yvals,
                              double[] yLowEB, double[] yHighEB,
                              int npts, boolean connected,
                              boolean errorBar, boolean resetScale)  {
        ///System.out.println("_setPoints()");/*CMP*/
        _checkDatasetIndex(dataset);

        // Remember x-coord of last point for walking window
        double oldX = 0;
        ArrayList<PlotPoint> oldPts = _points.get(dataset);
        int len = oldPts.size();
        if (len > 0) {
            PlotPoint oldPt = oldPts.get(len - 1);
            oldX = oldPt.originalx;
        }

        ArrayList<PlotPoint> pts = new ArrayList<PlotPoint>(npts); // New list of points
        _points.set(dataset, pts);

        double x = 0;
        for (int i=0; i<npts; ++i) {
            PlotPoint pt = new PlotPoint();
            x = xvals[i];
            double y = yvals[i];

            if (_xlog) {
                if (x <= 0.0) {
                    if (_xlogNoNegVals) {
                        System.err.println("Can't plot non-positive X values "+
                                           "when the logarithmic X axis "+
                                           "value is specified: " + x);
                        return;
                    } else {
                        x = _xlogNegDefault;
                    }
                } else {
                    x = Math.log(x)*_LOG10SCALE;
                }
            }

            // Modify x if wrapping.
            pt.originalx = x;   // Original value of x before wrapping.
            if(_wrap) {
                double width = _wrapHigh - _wrapLow;
                if (x < _wrapLow) {
                    x += width*Math.floor(1.0 + (_wrapLow-x)/width);
                } else if (x > _wrapHigh) {
                    x -= width*Math.floor(1.0 + (x-_wrapHigh)/width);
                    // NOTE: Could quantization errors be a problem here?
                    if (x == _wrapLow) x = _wrapHigh;
                }
            }

            if (_ylog) {
                if (y <= 0.0) {
                    if (_ylogNoNegVals) {
                        System.err.println("Can't plot non-positive Y values "+
                                           "when the logarithmic Y axis "+
                                           "value is specified: " + y);
                        return;
                    } else {
                        y = _ylogNegDefault;
                    }
                } else {
                    y = Math.log(y)*_LOG10SCALE;
                }
                if (errorBar && yLowEB[i] <= 0.0) {
                     if (_ylogNoNegVals) {
                         System.err.println("Can't plot non-positive Y values "+
                                            "when the logarithmic Y axis "+
                                            "value is specified: " + y);
                         return;
                     } else {
                         yLowEB[i] = _ylogNegDefault;
                     }
                } else if (errorBar) {
                    yLowEB[i] = Math.log(yLowEB[i])*_LOG10SCALE;
                }
                if (errorBar && yHighEB[i] <= 0.0) {
                     if (_ylogNoNegVals) {
                         System.err.println("Can't plot non-positive Y values "+
                                            "when the logarithmic Y axis "+
                                            "value is specified: " + y);
                         return;
                     } else {
                         yHighEB[i] = _ylogNegDefault;
                     }
                } else if (errorBar) {
                    yHighEB[i] = Math.log(yHighEB[i])*_LOG10SCALE;
                }
            }
            ////System.out.println("y["+i+"]="+y);/*CMP*/

            pt.x = x;
            pt.y = y;
            pt.connected = connected && _isConnected(dataset);

            // For auto-ranging, keep track of min and max.
            if (isScalingDataSet(dataset)) {
                if (i == 0) {
                    pt.connected = false;
                    if (resetScale) {
                        _xBottom = x;
                        _xTop = x;
                        _yBottom = y;
                        _yTop = y;
                        if (_spanZero) {
                            if (_yBottom > 0.0) _yBottom = 0.0;
                            if (_yTop < 0.0) _yTop = 0.0;
                        }
                    } else {
                        if (x < _xBottom) {
                            _xBottom = x;
                        } else if (x > _xTop) {
                            _xTop = x;
                        }
                        if (y < _yBottom) {
                            _yBottom = y;
                        } else if (y > _yTop) {
                            _yTop = y;
                        }
                    }
                } else {
                    if (x < _xBottom) {
                        _xBottom = x;
                    } else if (x > _xTop) {
                        _xTop = x;
                    }
                    if (y < _yBottom) {
                        _yBottom = y;
                    } else if (y > _yTop) {
                        _yTop = y;
                    }
                }
                _xyInvalid = false;

                if (errorBar) {
                    if (yLowEB[i] < _yBottom) _yBottom = yLowEB[i];
                    if (yLowEB[i] > _yTop) _yTop = yLowEB[i];
                    if (yHighEB[i] < _yBottom) _yBottom = yHighEB[i];
                    if (yHighEB[i] > _yTop) _yTop = yHighEB[i];
                    pt.yLowEB = yLowEB[i];
                    pt.yHighEB = yHighEB[i];
                    pt.errorBar = true;
                }
            }

            if (i > 0 && _wrap) {
                // Do not connect points if wrapping...
                PlotPoint old = (pts.get(i-1));
                if (old.x > x) pt.connected = false;
            }
            pts.add(pt);
        }

        if (getWalking()) {
            double dx;
            double[] xr = getXRange();
            boolean oldInRange = oldX >= xr[0] && oldX <= xr[1];
            if (oldInRange && ((dx = x - xr[0]) < 0 || (dx = x - xr[1]) > 0)) {
                setXRange(xr[0] + dx, xr[1] + dx);
            }
        }

        // Draw the point on the screen only if the plot is showing.
        Graphics graphics = getGraphics();
        // Need to check that graphics is not null because plot may have
        // been dismissed.
        if (_showing && graphics != null) {

            if (_xorMode && isDoubleBuffered()) {
                // NOTE: Double buffering has a bug in Java (in at least
                // version 1.3) where there is a one pixel alignment problem
                // that prevents XOR drawing from working correctly.
                // XOR drawing is used for live plots, and if double buffering
                // is turned on, then cruft is left on the screen whenever the
                // fill or zoom functions are used.
                // Here, if it hasn't been done already, we turn off double
                // buffering on this panel and all its parents for which this
                // is possible.  Note that we could do this globally using
                //
                // RepaintManager repaintManager
                //        = RepaintManager.currentManager(this);
                // repaintManager.setDoubleBufferingEnabled(false);
                //
                // However, that turns off double buffering in all windows
                // of the application, which means that other windows that only
                // work properly with double buffering (such as vergil windows)
                // will not work.
                //
                // NOTE: This fix creates another problem...
                // If there are other widgets besides the plotter in the
                // same top-level window, and they implement double
                // buffering (which they will by default), then they
                // need to be opaque or drawing artifacts will appear
                // upon exposure events.  The workaround is simple:
                // Make these other objects opaque, and set their
                // background color appropriately.
                //
                // See:
                // <pre>
                // http://developer.java.sun.com/developer/bugParade/bugs/
                //     4188795.html
                //     4204551.html
                //     4295712.htm
                // </pre>
                //
                // Since we are assured of being in the event dispatch thread,
                // we can simply execute this.
                setDoubleBuffered(false);
                Component parent = getParent();
                while (parent != null) {
                    if (parent instanceof JComponent) {
                        ((JComponent)parent).setDoubleBuffered(false);
                    }
                    parent = parent.getParent();
                }
            }
            repaint();
        }
    }

    /* In the specified data set, add the specified x, y point to the
     * plot.  Data set indices begin with zero.  If the dataset
     * argument is less than zero, throw an IllegalArgumentException
     * (a runtime exception).  If it refers to a data set that does
     * not exist, create the data set.  The fourth argument indicates
     * whether the point should be connected by a line to the previous
     * point.  However, this argument is ignored if setConnected() has
     * been called with a false argument.  In that case, a point is never
     * connected to the previous point.  That argument is also ignored
     * if the point is the first in the specified dataset.
     * The point is drawn on the screen only if is visible.
     * Otherwise, it is drawn the next time paintComponent() is called.
     *
     * This is not synchronized, so the caller should be.  Moreover, this
     * should only be called in the event dispatch thread. It should only
     * be called via _deferIfNecessary().
     */
    protected void _addPoint(
            int dataset, double x, double y, double yLowEB, double yHighEB,
            boolean connected, boolean errorBar, boolean drawit, boolean fix)  {
        _checkDatasetIndex(dataset);

        if (_xlog) {
            if (x <= 0.0) {
                System.err.println("Can't plot non-positive X values "+
                        "when the logarithmic X axis value is specified: " +
                        x);
                return;
            }
            x = Math.log(x)*_LOG10SCALE;
        }
        if (_ylog) {
            if (y <= 0.0) {
                System.err.println("Can't plot non-positive Y values "+
                        "when the logarithmic Y axis value is specified: " +
                        y);
                return;
            }
            y = Math.log(y)*_LOG10SCALE;
            if (errorBar) {
                if (yLowEB <= 0.0 || yHighEB <= 0.0) {
                    System.err.println("Can't plot non-positive Y values "+
                            "when the logarithmic Y axis value is specified: " +
                            y);
                    return;
                }
                yLowEB = Math.log(yLowEB)*_LOG10SCALE;
                yHighEB = Math.log(yHighEB)*_LOG10SCALE;
            }
        }

        ArrayList<PlotPoint> pts = _points.get(dataset);

        // If X persistence has been set, then delete any old points.
        if (_xPersistence > 0.0) {
            int numToDelete = 0;
            while(numToDelete < pts.size()) {
                PlotPoint old = (pts.get(numToDelete));
                if (x - old.originalx <= _xPersistence) break;
                numToDelete++;
            }
            for (int i=0; i < numToDelete; i++) {
                erasePoint(dataset, 0);
            }
        }

        // Get the new size after deletions.
        int size = pts.size();

        PlotPoint pt = new PlotPoint();
        // Original value of x before wrapping.
        pt.originalx = x;

        // Modify x if wrapping.
        if(_wrap) {
            double width = _wrapHigh - _wrapLow;
            if (x < _wrapLow) {
                x += width*Math.floor(1.0 + (_wrapLow-x)/width);
            } else if (x > _wrapHigh) {
                x -= width*Math.floor(1.0 + (x-_wrapHigh)/width);
                // NOTE: Could quantization errors be a problem here?
                if (x == _wrapLow) x = _wrapHigh;
            }
        }

        // For auto-ranging, keep track of min and max.
        if (_xyInvalid) {
            calcMinMax();
        }
        if (isScalingDataSet(dataset)) {
            if (x < _xBottom) _xBottom = x;
            if (x > _xTop) _xTop = x;
            if (y < _yBottom) _yBottom = y;
            if (y > _yTop) _yTop = y;

            if (errorBar) {
                if (yLowEB < _yBottom) _yBottom = yLowEB;
                if (yLowEB > _yTop) _yTop = yLowEB;
                if (yHighEB < _yBottom) _yBottom = yHighEB;
                if (yHighEB > _yTop) _yTop = yHighEB;
                pt.yLowEB = yLowEB;
                pt.yHighEB = yHighEB;
                pt.errorBar = true;
            }
        }

        pt.x = x;
        pt.y = y;
        pt.connected = connected && _isConnected(dataset);

        // Maybe change the x-range if we're in walking window mode
        boolean walked = false;
        if (!fix &&  getWalking()) {
            double dx;
            double[] xr = getXRange();
            boolean oldInRange = true;
            if (size > 0) {
                PlotPoint old = (pts.get(size - 1));
                oldInRange = old.originalx >= _xMin && old.originalx <= _xMax;
            }
            if (oldInRange && ((dx = x - xr[0]) < 0 || (dx = x - xr[1]) > 0)) {
                setXRange(xr[0] + dx, xr[1] + dx);
                walked = true;
            }
        }

        // If this is the first point in the dataset, clear the connected bit.
        if (size == 0) {
            pt.connected = false;
        } else if(_wrap) {
            // Do not connect points if wrapping...
            PlotPoint old = (pts.get(size-1));
            if (old.x > x) {
                pt.connected = false;
            }
        }
        pts.add(pt);
        // If points persistence has been set, then delete one old point.
        if (_pointsPersistence > 0) {
            if (size > _pointsPersistence) {
                if (_xorMode) {
                    erasePoint(dataset, 0);
                } else {
                    removePoint(dataset, 0);
                }
            }
        }

        // Draw the point on the screen only if the plot is showing.
        Graphics graphics = getGraphics();
        // Need to check that graphics is not null because plot may have
        // been dismissed.
        if (drawit && _showing && graphics != null) {

            if (_xorMode && isDoubleBuffered()) {
                // NOTE: Double buffering has a bug in Java (in at least
                // version 1.3) where there is a one pixel alignment problem
                // that prevents XOR drawing from working correctly.
                // XOR drawing is used for live plots, and if double buffering
                // is turned on, then cruft is left on the screen whenever the
                // fill or zoom functions are used.
                // Here, if it hasn't been done already, we turn off double
                // buffering on this panel and all its parents for which this
                // is possible.  Note that we could do this globally using
                //
                // RepaintManager repaintManager
                //        = RepaintManager.currentManager(this);
                // repaintManager.setDoubleBufferingEnabled(false);
                //
                // However, that turns off double buffering in all windows
                // of the application, which means that other windows that only
                // work properly with double buffering (such as vergil windows)
                // will not work.
                //
                // NOTE: This fix creates another problem...
                // If there are other widgets besides the plotter in the
                // same top-level window, and they implement double
                // buffering (which they will by default), then they
                // need to be opaque or drawing artifacts will appear
                // upon exposure events.  The workaround is simple:
                // Make these other objects opaque, and set their
                // background color appropriately.
                //
                // See:
                // <pre>
                // http://developer.java.sun.com/developer/bugParade/bugs/
                //     4188795.html
                //     4204551.html
                //     4295712.htm
                // </pre>
                //
                // Since we are assured of being in the event dispatch thread,
                // we can simply execute this.
                setDoubleBuffered(false);
                Component parent = getParent();
                while (parent != null) {
                    if (parent instanceof JComponent) {
                        ((JComponent)parent).setDoubleBuffered(false);
                    }
                    parent = parent.getParent();
                }
            }

            // Again, we are in the event thread, so this is safe...
            if (walked) {
                repaint();      // TODO: is this the best way?
            } else {
                _drawPlotPoint(graphics, dataset, pts.size() - 1);
            }
        }

        if(_wrap && x == _wrapHigh) {
            // Plot a second point at the low end of the range.
            _addPoint(dataset, _wrapLow, y, yLowEB, yHighEB, false, errorBar,
                      drawit, false);
        }
    }

    /* Clear the plot of all data points.  If the argument is true, then
     * reset all parameters to their initial conditions, including
     * the persistence, plotting format, and axes formats.
     * For the change to take effect, you must call repaint().
     *
     * This is not synchronized, so the caller should be.  Moreover, this
     * should only be called in the event dispatch thread. It should only
     * be called via _deferIfNecessary().
     */
    protected void _clear(boolean format) {
        super.clear(format);
        _currentdataset = -1;
        _points = new ArrayList<ArrayList<PlotPoint>>();
        _prevx = new ArrayList<Long>();
        _prevy = new ArrayList<Long>();
        _maxdataset = -1;
        _firstinset = true;
        _sawfirstdataset = false;
        _xyInvalid = true;
        if (format) {
            _showing = false;
            // Reset format controls
            _formats = new ArrayList<Format>();
            _marks = 0;
            _pointsPersistence = 0;
            _xPersistence = 0;
            _xorMode = false;
            _bars = false;
            _spanZero = false;
            _barwidth = 0.5;
            _baroffset = 0.05;
            _connected = true;
            _impulses = false;
            _reusedatasets = false;
        }
    }

    /** Clear the plot of data points in the specified dataset.
     *  This calls repaint() to request an update of the display.
     *
     * This is not synchronized, so the caller should be.  Moreover, this
     * should only be called in the event dispatch thread. It should only
     * be called via _deferIfNecessary().
     */
    private void _clear(int dataset) {
        _checkDatasetIndex(dataset);
        _xyInvalid = true;
        // Vector.clear() is new in JDK1.2, so we use just
        // create a new Vector here so that we can compile
        // this with JDK1.1 for use in JDK1.1 browsers
        _points.set(dataset, new ArrayList<PlotPoint>());
        repaint();
    }

    public void ensureCapacity(int dataset, int minCapacity) {
        _checkDatasetIndex(dataset);
        ArrayList<PlotPoint> points = _points.get(dataset);
        points.ensureCapacity(minCapacity);
    }

    /* Draw the specified point and associated lines, if any.
     * Note that paintComponent() should be called before
     * calling this method so that it calls _drawPlot(), which sets
     * _xscale and _yscale. Note that this does not check the dataset
     * index.  It is up to the caller to do that.
     *
     * Note that this method is not synchronized, so the caller should be.
     * Moreover this method should always be called from the event thread
     * when being used to write to the screen.
     */
    private void _drawPlotPoint(Graphics graphics,
                                int dataset, int index) {
        // NB: Turning on anti-aliasing here slows things down considerably
        //     when using addPoint() for a large data set.
        //if (graphics instanceof Graphics2D) {
        //    RenderingHints hints = _defaultPlotRenderingHints();
        //    ((Graphics2D)graphics).addRenderingHints(hints);
        //}
        if (graphics instanceof Graphics2D) {
            Graphics2D g2 = (Graphics2D)graphics;
            g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
                                RenderingHints.VALUE_ANTIALIAS_ON);
        }
        if (_xorMode) {
            // To allow erasing to work by just redrawing the points.
            if (_background == null) {
                // java.awt.Component.setBackground(color) says that
                // if the color "parameter is null then this component
                // will inherit the  background color of its parent."
                graphics.setXORMode(getBackground());
            } else {
                graphics.setXORMode(_background);
            }
        }
        // Set the color
        if (_usecolor) {
            int color = dataset % _colors.length;
            graphics.setColor(_colors[color]);
        } else {
            graphics.setColor(_foreground);
        }

        ArrayList<PlotPoint> pts = _points.get(dataset);
        PlotPoint pt = pts.get(index);
        // Use long here because these numbers can be quite large
        // (when we are zoomed out a lot).
        long ypos = _lry - (long)((pt.y - _yMin) * _yscale);
        long xpos = _ulx + (long)((pt.x - _xMin) * _xscale);

        // Draw the line to the previous point.
        long prevx = _prevx.get(dataset);
        long prevy = _prevy.get(dataset);
        // MIN_VALUE is a flag that there has been no previous x or y.
        if (pt.connected) {
            _drawLine(graphics, dataset, xpos, ypos, prevx, prevy, true);
        }

        // Save the current point as the "previous" point for future
        // line drawing.
        _prevx.set(dataset, xpos);
        _prevy.set(dataset, ypos);

        // Draw decorations that may be specified on a per-dataset basis
        Format fmt = _formats.get(dataset);
        if (fmt.impulsesUseDefault) {
            if (_impulses) _drawImpulse(graphics, xpos, ypos, true);
        } else {
            if (fmt.impulses) _drawImpulse(graphics, xpos, ypos, true);
        }

        // Check to see whether the dataset has a marks directive
        int marks = _marks;
        if (!fmt.marksUseDefault) marks = fmt.marks;
        if (marks != 0) _drawPoint(graphics, dataset, xpos, ypos, true);

        if (_bars) _drawBar(graphics, dataset, xpos, ypos, true);
        if (pt.errorBar)
            _drawErrorBar(graphics, dataset, xpos,
                    _lry - (long)((pt.yLowEB - _yMin) * _yscale),
                    _lry - (long)((pt.yHighEB - _yMin) * _yscale), true);

        // Restore the color, in case the box gets redrawn.
        graphics.setColor(_foreground);
        if (_xorMode) {
            // Restore paint mode in case axes get redrawn.
            graphics.setPaintMode();
        }
    }

    /**
     * Add x,y coords to plot to given arrays.
     * @param pts ArrayList of "PlotPoint"s to get data from.
     * @param x Array of X plot coords.
     * @param y Array of Y plot coords.
     * @param index Index into "PlotPoint" vector.
     * @param offset Is subtracted from "index" to get index into X
     * and Y arrays.
     * @param dataset Which plot dataset we are dealing with.
     * @return Returns true iff this point is connected to previous one.
     */
    private boolean _appendPlotPoint(ArrayList<PlotPoint> pts, int[] x, int[] y,
                                     int index, int offset, int dataset) {

        PlotPoint pt = pts.get(index);
        // Use long here because these numbers can be quite large
        // (when we are zoomed out a lot).
        long ypos = _lry - (long)((pt.y - _yMin) * _yscale);
        ypos = Math.min(Integer.MAX_VALUE, Math.max(Integer.MIN_VALUE, ypos));
        y[index - offset] = (int)ypos;
        _prevy.set(dataset, ypos);

        long xpos = _ulx + (long)((pt.x - _xMin) * _xscale);
        xpos = Math.min(Integer.MAX_VALUE, Math.max(Integer.MIN_VALUE, xpos));
        x[index - offset] = (int)xpos;
        _prevx.set(dataset, xpos);

        return pt.connected;
    }

    /* Erase the point at the given index in the given dataset.  If
     * lines are being drawn, also erase the line to the next points
     * (note: not to the previous point).
     *
     * This is not synchronized, so the caller should be.  Moreover, this
     * should only be called in the event dispatch thread. It should only
     * be called via _deferIfNecessary().
     */
    protected void _erasePoint(int dataset, int index) {
        _checkDatasetIndex(dataset);
        // Plot has probably been dismissed.  Return.
        Graphics graphics = getGraphics();
        // Need to check that graphics is not null because plot may have
        // been dismissed.
        if (_showing && graphics != null) {
            // Set the color
            if (_xorMode) {
                // To allow erasing to work by just redrawing the points.
                if (_background == null) {
                    graphics.setXORMode(getBackground());
                } else {
                    graphics.setXORMode(_background);
                }
            }
            if (_usecolor) {
                int color = dataset % _colors.length;
                graphics.setColor(_colors[color]);
            } else {
                graphics.setColor(_foreground);
            }

            ArrayList<PlotPoint> pts = _points.get(dataset);
            PlotPoint pt = pts.get(index);
            long ypos = _lry - (long) ((pt.y - _yMin) * _yscale);
            long xpos = _ulx + (long) ((pt.x - _xMin) * _xscale);

            // Erase line to the next point, if appropriate.
            if (index < pts.size() - 1) {
                PlotPoint nextp = pts.get(index+1);
                int nextx = _ulx + (int) ((nextp.x - _xMin) * _xscale);
                int nexty = _lry - (int) ((nextp.y - _yMin) * _yscale);
                // NOTE: I have no idea why I have to give this point backwards.
                if (nextp.connected) _drawLine(graphics, dataset,
                        nextx, nexty,  xpos, ypos, true);
                nextp.connected = false;
            }

            // Draw decorations that may be specified on a per-dataset basis
            Format fmt = _formats.get(dataset);
            if (fmt.impulsesUseDefault) {
                if (_impulses) _drawImpulse(graphics, xpos, ypos, true);
            } else {
                if (fmt.impulses) _drawImpulse(graphics, xpos, ypos, true);
            }

            // Check to see whether the dataset has a marks directive
            int marks = _marks;
            if (!fmt.marksUseDefault) marks = fmt.marks;
            if (marks != 0) _drawPoint(graphics, dataset, xpos, ypos, true);

            if (_bars) _drawBar(graphics, dataset, xpos, ypos, true);
            if (pt.errorBar)
                _drawErrorBar(graphics, dataset, xpos,
                        _lry - (long)((pt.yLowEB - _yMin) * _yscale),
                        _lry - (long)((pt.yHighEB - _yMin) * _yscale), true);

            // Restore the color, in case the box gets redrawn.
            graphics.setColor(_foreground);
            if (_xorMode) {
                // Restore paint mode in case axes get redrawn.
                graphics.setPaintMode();
            }
        }
        removePoint(dataset, index);
    }

    private void removePoint(int dataset, int index) {
        // The following is executed whether the plot is showing or not.
        // Remove the point from the model.
        ArrayList<PlotPoint> points = _points.get(dataset);
        if (points != null) {
            // If this point is at the maximum or minimum x or y boundary,
            // then flag that boundary needs to be recalculated next time
            // fillPlot() is called.
            PlotPoint pt = points.get(index);
            if (pt != null) {
                if (pt.x == _xBottom || pt.x == _xTop ||
                        pt.y == _yBottom || pt.y == _yTop) {
                    _xyInvalid = true;
                }
                points.remove(index);
            }
        }
    }

    protected void calcMinMax() {
        // Recalculate the boundaries based on currently visible data
        _xBottom = Double.MAX_VALUE;
        _xTop = Double.MIN_VALUE;
        _yBottom = Double.MAX_VALUE;
        _yTop = Double.MIN_VALUE;
        for (int dataset = 0; dataset < _points.size(); dataset++) {
            if (isScalingDataSet(dataset)) {
                ArrayList<PlotPoint> points = _points.get(dataset);
                for (int index = 0; index < points.size(); index++) {
                    PlotPoint pt = points.get(index);
                    if (pt.x < _xBottom)
                        _xBottom = pt.x;
                    if (pt.x > _xTop)
                        _xTop = pt.x;
                    if (pt.y < _yBottom)
                        _yBottom = pt.y;
                    if (pt.y > _yTop)
                        _yTop = pt.y;
                }
            }
        }
        // If this is a bar graph, then make sure the Y range includes 0
        if (_spanZero || _bars) {
            if (_yBottom > 0.0) _yBottom = 0.0;
            if (_yTop < 0.0) _yTop = 0.0;
        }
        _xyInvalid = false;
    }


    /* Rescale so that the data that is currently plotted just fits.
     * This overrides the base class method to ensure that the protected
     * variables _xBottom, _xTop, _yBottom, and _yTop are valid.
     * This method calls repaint(), which causes the display
     * to be updated.
     *
     * This is not synchronized, so the caller should be.  Moreover, this
     * should only be called in the event dispatch thread. It should only
     * be called via _deferIfNecessary().
     */
    protected void _fillPlot() {
        if (_xyInvalid) {
            calcMinMax();
        }
        super.fillPlot();
    }

    // Return true if the specified dataset is connected by default.
    private boolean _isConnected(int dataset) {
        if (dataset < 0) return _connected;
        _checkDatasetIndex(dataset);
        Format fmt = _formats.get(dataset);
        if (fmt.connectedUseDefault) {
            return _connected;
        } else {
            return fmt.connected;
        }
    }

    /**
     * Construct a Color that is a brighter or darker version of a given
     * color.
     * Positive percent changes make the color brighter, if it
     * is already as bright as it can get without changing the hue,
     * the saturation is decreased.
     * A percent change of -100 will always give black.
     *
     * This is a copy of the version in vnmr.util.Util; that class has
     * many dependencies of VnmrJ-specific stuff.
     *
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

    ///////////////////////////////////////////////////////////////////
    ////                         private variables                 ////

    /** @serial Number of points to persist for. */
    private int _pointsPersistence = 0;

    /** @serial Persistence in units of the horizontal axis. */
    private double _xPersistence = 0.0;

    /** @serial Whether we draw in XOR mode. */
    private boolean _xorMode = false;

    /** @serial True if this is a bar plot. */
    private boolean _bars = false;

    /** @serial True if we're forced to show y=0 on autoscale. */
    private boolean _spanZero = false;

    /** @serial Width of a bar in x axis units. */
    private double _barwidth = 0.5;

    /** @serial Offset per dataset in x axis units. */
    private double _baroffset = 0.05;

    /** @serial True if the points are connected. */
    private boolean _connected = true;

    /** @serial True if this is an impulse plot. */
    private boolean _impulses = false;

    /** @serial The width of impulses in pixels. */
    private int _impulseWidth = 1;

    /** @serial The highest data set used. */
    private int _maxdataset = -1;

    /** @serial True if we saw 'reusedatasets: on' in the file. */
    private boolean _reusedatasets = false;

    /** @serial Is this the first datapoint in a set? */
    private boolean _firstinset = true;

    /** @serial Have we seen a DataSet line in the current data file? */
    private boolean _sawfirstdataset = false;

    /** @serial Give the radius of a point for efficiency. */
    private int _radius = 3;

    /** @serial Give the diameter of a point for efficiency. */
    private int _diameter = 6;

    /** @serial Information about the previously plotted point. */
    private ArrayList<Long> _prevx = new ArrayList<Long>();
    private ArrayList<Long> _prevy = new ArrayList<Long>();

    // Half of the length of the error bar horizontal leg length;
    private static final int _ERRORBAR_LEG_LENGTH = 5;

    // Maximum number of different marks
    // NOTE: There are 11 colors in the base class.  Combined with 10
    // marks, that makes 110 unique signal identities.
    private static final int _MAX_MARKS = 10;

    /** @serial Flag indicating validity of _xBottom, _xTop,
     *  _yBottom, and _yTop.
     */
    private boolean _xyInvalid = true;

    /** @serial Set by _drawPlot(), and reset by clear(). */
    private boolean _showing = false;

    /** @serial Format information on a per data set basis. */
    private ArrayList<Format> _formats = new ArrayList<Format>();

    /** List of icons to draw on the plot, if they are within the bounds */
    private List<PlotIcon> _icons = new ArrayList<PlotIcon>();

    /** List of circles to draw on the plot, if they are within the bounds */
    private List<Circle> _circles = new ArrayList<Circle>();

    ///////////////////////////////////////////////////////////////////
    ////                         inner classes                     ////

    private class Format implements Serializable {
        // Indicate whether the current dataset is connected.
        public boolean connected;

        // Indicate whether the above variable should be ignored.
        public boolean connectedUseDefault = true;

        // Indicate whether a stem plot should be drawn for this data set.
        // This is ignored unless the following variable is set to false.
        public boolean impulses;

        // Indicate whether the above variable should be ignored.
        public boolean impulsesUseDefault = true;

        // Indicate what type of mark to use.
        // This is ignored unless the following variable is set to false.
        public int marks;

        // Indicate whether the above variable should be ignored.
        public boolean marksUseDefault = true;
    }


    private class PlotIcon {
        private Icon _icon;
        private double _x;
        private double _y;

        /**
         * Specify an icon to appear on the plot,
         * centered at the given coordinates.
         * @param icon The icon.
         * @param x X location of the icon in data coordinates.
         * @param y Y location of the icon in data coordinates.
         */
        public PlotIcon(Icon icon, double x, double y) {
            _icon = icon;
            _x = x;
            _y = y;
        }

        /**
         * Draw this icon.
         * @param graphics The graphics to draw on.
         * @param plot The plot to draw on.
         */
        public void draw(Graphics graphics, Plot plot) {
            if (_icon == null) {
                return;
            }
            double[] rx = getFullXRange();
            double[] ry = getFullYRange();
            if (!(_x >= rx[0] && _x <= rx[1] && _y >= ry[0] && _y <= ry[1])) {
                // Coords out of visible range (or NaN)
                return;
            }
            int w = _icon.getIconWidth();
            int h = _icon.getIconHeight();
            int x = xDataToPix(_x) - w / 2;
            int y = yDataToPix(_y) - h / 2;
            _icon.paintIcon(plot, graphics, x, y);

            // Draw registration mark (for testing)
            //graphics.setColor(Color.RED);
            //x = xDataToPix(_x);
            //y = yDataToPix(_y);
            //graphics.drawLine(x, y - h / 2, x, y + h / 2);
            //graphics.drawLine(x - w / 2, y, x + w / 2, y);
        }
    }


    public class Circle {
        private double _x;
        private double _y;
        private double _r;
        private double _theta0 = 0;
        private double _deltaTheta = 2 * Math.PI;
        private Color _color;
        private int _layer;

        /**
         * Create a circle to be drawn on the plot behind the data.
         * @param x The x coordinate of the center in data units.
         * @param y The y coordinate of the center in data units.
         * @param r The radius of the arc in data units.
         * @param color The color of the arc.
         * are not drawn.)
         */
        public Circle(double x, double y, double r, Color color) {
            this(x, y, r, color, 100);
        }

        /**
         * Create a circle to be drawn on the plot.
         * @param x The x coordinate of the center in data units.
         * @param y The y coordinate of the center in data units.
         * @param r The radius of the arc in data units.
         * @param color The color of the arc.
         * @param layer 0 => on top of data, 100 => behind data. (Other values
         * are not drawn.)
         */
        public Circle(double x, double y, double r, Color color, int layer) {
            _x = x;
            _y = y;
            _r = r;
            _color = color;
            _layer = layer;
        }

        /**
         * Create an arc to be drawn on the plot behind the data.
         * @param x The x coordinate of the center in data units.
         * @param y The y coordinate of the center in data units.
         * @param r The radius of the arc in data units.
         * @param theta0 The angle from the center to the beginning of the arc.
         * @param deltaTheta The length of the arc in radians.
         * @param color The color of the arc.
         * are not drawn.)
         */
        public Circle(double x, double y, double r,
                      double theta0, double deltaTheta, Color color) {
            this(x, y, r, theta0, deltaTheta, color, 100);
        }

        /**
         * Specify an arc to be drawn on the plot.
         * @param x The x coordinate of the center in data units.
         * @param y The y coordinate of the center in data units.
         * @param r The radius of the arc in data units.
         * @param theta0 The angle from the center to the beginning of the arc.
         * @param deltaTheta The length of the arc in radians.
         * @param color The color of the arc.
         * @param layer 0 => on top of data, 100 => behind data. (Other values
         * are not drawn.)
         */
        public Circle(double x, double y, double r,
                      double theta0, double deltaTheta, Color color, int layer) {
            this(x, y, r, color);
            _theta0 = theta0;
            _deltaTheta = deltaTheta;
            _layer = layer;
        }

        public int getLayer() {
            return _layer;
        }

        public void draw(Graphics graphics, Plot plot) {
            Graphics2D g2 = (Graphics2D)graphics.create();
            g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
                                RenderingHints.VALUE_ANTIALIAS_ON);
            double[] rx = getFullXRange();
            double[] ry = getFullYRange();
            if (_x + _r < rx[0] || _x - _r > rx[1]
                || _y + _r < ry[0] || _y - _r > ry[1]) {
                return;
            }
            int x = xDataToPix(_x - _r);
            int dx = (int)Math.round(_xscale * _r * 2);
            int y = yDataToPix(_y + _r);
            int dy = (int)Math.round(_yscale * _r * 2);
            g2.setColor(_color);

            // Calculate distance from center of circle to farthest corner
            // of plot area.
            int iXCor = (_x - rx[0] > rx[1] - _x) ? 0 : 1;
            int iYCor = (_y - ry[0] > ry[1] - _y) ? 0 : 1;
            double xCor = _x - rx[iXCor];
            double yCor = _y - ry[iYCor];
            // Distance to farthest corner:
            double rCor = Math.hypot(xCor, yCor);
            if (_r > rCor) {
                // None of circle is visible
                return;
            //} else if (_x > rx[0] && _x < rx[1] && _y > ry[0] && _y < ry[1]) {
            } else if (true) {
                // Center of circle is in plot area.
                // Just draw the whole arc.
                double theta_deg = _theta0 * 180 / Math.PI;
                int iTheta = (int)Math.round(theta_deg);
                double dtheta_deg = _deltaTheta * 180 / Math.PI;
                int idTheta = (int)Math.round(dtheta_deg);
                g2.drawArc(x, y, dx, dy, iTheta, idTheta);
            } else {
                // Center is outside plot area.
                // Distance to closest part of plot area:
                double rEdge = 0;
                if (_x > rx[0] && _x < rx[1]) {
                    // Center is within X range of plot.
                    // Get distance to closest X edge.
                    int iXEdge = _y < ry[0] ? 0 : 1;
                    rEdge = Math.abs(ry[iXEdge] - _y);
                } else if (_y > ry[0] && _y < ry[1]) {
                    // Center is within Y range of plot.
                    // Get distance to closest Y edge.
                    int iYEdge = _x < rx[0] ? 0 : 1;
                    rEdge = Math.abs(rx[iYEdge] - _x);
                } else {
                    // We're off of one corner of the plot region.
                    iXCor = (Math.abs(rx[0] - _x) < Math.abs(_x - rx[1]))
                        ? 0 : 1;
                    iYCor = (Math.abs(ry[0] - _y) < Math.abs(_y - ry[1]))
                         ? 0 : 1;
                    xCor = _x - rx[iXCor];
                    yCor = _y - ry[iYCor];
                    // Distance to closest corner:
                    rEdge = Math.hypot(xCor, yCor);
                }
                if (rEdge < _r) {
                    // Part of circle is visible.
                    // Just draw an arc that passes through the plot area.
                    // Consider a test circle that circumscribes the plot area.
                    // Tangents to the test circle that pass through the
                    // center of the drawn arc, define angle limits of arc.
                    double xradius = (rx[1] - rx[0]) / 2;
                    double yradius = (ry[1] - ry[0]) / 2;
                    double pRadius = Math.hypot(xradius, yradius);
                    // Position of plot center wrt test circle center:
                    double px = (rx[1] + rx[0]) / 2 - _x;
                    double py = (ry[1] + ry[0]) / 2 - _y;
                    double theta = Math.atan2(py, px) * 180 / Math.PI;

                    double pDist = Math.hypot(px, py);
                    double dTheta = Math.asin(pRadius / pDist) * 180 / Math.PI;
                    int iTheta = (int)Math.floor(theta - dTheta);
                    int iThetaRange = (int)Math.ceil(theta + dTheta - iTheta);
                    // TODO: If dTheta << 1, just draw a straight line?
                    // NB: If the arc extends way outside the clip rect,
                    // Java exceptions are generated.
                    g2.drawArc(x, y, dx, dy, iTheta, iThetaRange);
                }
            }
            /*if (_x != 0 || _y != 0) {
                // Diagnostic: print radius and closest point to origin
                double theta = Math.atan2(_y, _x);
                double xmin = _x - _r * Math.cos(theta);
                double ymin = _y - _r * Math.sin(theta);
                System.out.println("CIRCLE: R=" + _r
                                   + ", xmin=" + xmin
                                   + ", ymin=" + ymin);
            }/*CMP*/
        }
    }
}
