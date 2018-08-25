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

/* A labeled box for signal plots.

@Author: Edward A. Lee and Christopher Hylands

@Contributors:  William Wu, Robert Kroeger

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

import java.awt.Color;
import java.awt.Component;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.EventQueue;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Insets;
import java.awt.LayoutManager;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.event.*;
import java.awt.image.BufferedImage;
import java.awt.Image;
import java.awt.print.PageFormat;
import java.awt.print.Printable;
import java.awt.print.PrinterException;
import java.awt.print.PrinterJob;
import java.awt.RenderingHints;
import java.io.*;
import java.net.URL;
import java.text.*;
import java.util.*;
import javax.swing.*;

import vnmr.util.VButtonBorder;


//// PlotBox
/**
This class provides a labeled box within which to place a data plot.
A title, X and Y axis labels, tick marks, and a legend are all supported.
Zooming in and out is supported.  To zoom in, drag the mouse
downwards to draw a box.  To zoom out, drag the mouse upward.
<p>
The box can be configured either through a file with commands or
through direct invocation of the public methods of the class.
<p>
When calling the methods, in most cases the changes will not
be visible until paintComponent() has been called.  To request that this
be done, call repaint().
<p>
At this time, the two export commands produce encapsulated postscript
tuned for black-and-white printers.  In the future, more formats may
supported.

@author Edward A. Lee, Christopher Hylands;
contributor Jun Wu (jwu@inin.com.au)
 */
public class PlotBox extends JPanel implements Printable {

    ///////////////////////////////////////////////////////////////////
    ////                         constructor                       ////

    /** Construct a plot box with a default configuration. */
    public PlotBox() {
        // If we make this transparent, the background shows through.
        // However, we assume that the user will set the background.
        // NOTE: A component is transparent by default (?).
        // setOpaque(false);
        setOpaque(true);

        setLayout(new PlotLayout());
        addMouseListener(new ZoomListener());
        ////addKeyListener(new CommandListener());
        addMouseMotionListener(new DragListener());
        // This is something we want to do only once...
        _measureFonts();
        // Request the focus so that key events are heard.
        // NOTE: no longer needed?
        // requestFocus();

        _buttonListener = new ButtonListener();
        _legendListener = new LegendListener();
    }

    ///////////////////////////////////////////////////////////////////
    ////                         public methods                    ////

    /** Add a legend (displayed at the upper right) for the specified
     *  data set with the specified string.  Short strings generally
     *  fit better than long strings.  If the string is empty, or the
     *  argument is null, then no legend is added.
     *  @param dataset The dataset index.
     *  @param legend The label for the dataset.
     */
    public synchronized void addLegend(int dataset, String legend) {
        if (legend == null || legend.equals("")) return;
        _legendStrings.addElement(legend);
        _legendDatasets.addElement(dataset);
    }

    /** Specify a tick mark for the X axis.  The label given is placed
     *  on the axis at the position given by <i>position</i>. If this
     *  is called once or more, automatic generation of tick marks is
     *  disabled.  The tick mark will appear only if it is within the X
     *  range.
     *  @param label The label for the tick mark.
     *  @param position The position on the X axis.
     */
    public synchronized void addXTick(String label, double position) {
        if (_xticks == null) {
            _xticks = new Vector<Double>();
            _xticklabels = new Vector<String>();
        }
        _xticks.addElement(new Double(position));
        _xticklabels.addElement(label);
    }

    /** Specify a tick mark for the Y axis.  The label given is placed
     *  on the axis at the position given by <i>position</i>. If this
     *  is called once or more, automatic generation of tick marks is
     *  disabled.  The tick mark will appear only if it is within the Y
     *  range.
     *  @param label The label for the tick mark.
     *  @param position The position on the Y axis.
     */
    public synchronized void addYTick(String label, double position) {
        if (_yticks == null) {
            _yticks = new Vector<Double>();
            _yticklabels = new Vector<String>();
        }
        _yticks.addElement(new Double(position));
        _yticklabels.addElement(label);
    }

    /** If the argument is true, clear the axes.  I.e., set all parameters
     *  controlling the axes to their initial conditions.
     *  For the change to take effect, call repaint().  If the argument
     *  is false, do nothing.
     *  @param axes If true, clear the axes parameters.
     */
    public synchronized void clear(boolean axes) {
        _xBottom = Double.MAX_VALUE;
        _xTop = - Double.MAX_VALUE;
        _yBottom = Double.MAX_VALUE;
        _yTop = - Double.MAX_VALUE;
        if (axes) {
            // Protected members first.
            _yMax = 0;
            _yMin = 0;
            _xMax = 0;
            _xMin = 0;
            _xRangeGiven = false;
            _yRangeGiven = false;
            _originalXRangeGiven = false;
            _originalYRangeGiven = false;
            _rangesGivenByZooming = false;
            _xlog = false;
            _ylog = false;
            _grid = true;
            _wrap = false;
            _usecolor = true;

            _xlabel = null;
            _ylabel = null;
            _title = null;
            _legendStrings = new Vector<String>();
            _legendDatasets = new Vector<Integer>();
            _xticks = null;
            _xticklabels = null;
            _yticks = null;
            _yticklabels = null;
        }
    }

    /** Clear all legends.  This will show up on the next redraw.
     */
    public synchronized void clearLegends() {
        _legendStrings = new Vector<String>();
        _legendDatasets = new Vector<Integer>();
    }

    /** Export a description of the plot.
     *  Currently, only EPS is supported.  But in the future, this
     *  may cause a dialog box to open to allow the user to select
     *  a format.  If the argument is null, then the description goes
     *  to the clipboard.  Otherwise, it goes to the specified file.
     *  To send it to standard output, use
     *  <code>System.out</code> as an argument.
     *  @param out An OutputStream to which to send the description.
     */
    public synchronized void export(OutputStream out,
                                    int x, int y,
                                    int ptWidth, int ptHeight,
                                    String creator, String date, String title) {
        try {
            if (this instanceof Plot) {
                Plot pplot = new Plot((Plot)this);
                pplot.setPlotBackground(Color.WHITE);
                pplot.setGridColor(Color.LIGHT_GRAY);
                pplot.setTickColor(Color.BLACK);
                //pplot.setSize(ptWidth, ptHeight);
                //pplot.setBounds(0, 0, ptWidth, ptHeight);
                EPSGraphics g = new EPSGraphics(out, x, y,
                                                ptWidth, ptHeight,
                                                _lrx, _height,
                                                creator,
                                                date,
                                                title);
                //pplot._drawPlot(g, false,
                //                new Rectangle(0, 0, ptWidth, ptHeight));
                pplot._drawPlot(g, false);
                g.showpage();
            }
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

    // CONTRIBUTED CODE.
    // I wanted the ability to use the Plot object in a servlet and to
    // write out the resultant images. The following routines,
    // particularly exportImage(), permit this. I also had to make some
    // minor changes elsewhere. Rob Kroeger, May 2001.

    // NOTE: This code has been modified by EAL to conform with Ptolemy II
    // coding style.

    /** Create a BufferedImage and draw this plot to it.
     *  The size of the returned image matches the current size of the plot.
     *  This method can be used, for
     *  example, by a servelet to produce an image, rather than
     *  requiring an applet to instantiate a PlotBox.
     *  @return An image filled by the plot.
     */
    public synchronized BufferedImage exportImage() {
        Rectangle rectangle = new Rectangle(_preferredWidth, _preferredHeight);
        return exportImage(
                new BufferedImage(
                    rectangle.width,
                    rectangle.height,
                    BufferedImage.TYPE_INT_ARGB),
                rectangle,
                _defaultImageRenderingHints(),
                false);
    }

    /** Create a BufferedImage the size of the given rectangle and draw
     *  this plot to it at the position specified by the rectangle.
     *  The plot is rendered using anti-aliasing.
     *  @param rectangle The size of the plot. This method can be used, for
     *  example, by a servelet to produce an image, rather than
     *  requiring an applet to instantiate a PlotBox.
     *  @return An image containing the plot.
     */
    public synchronized BufferedImage exportImage(Rectangle rectangle) {
        return exportImage(
                new BufferedImage(
                    rectangle.width,
                    rectangle.height,
                    BufferedImage.TYPE_INT_ARGB),
                rectangle,
                _defaultImageRenderingHints(),
                false);
    }

    /** Draw this plot onto the specified image at the position of the
     *  specified rectangle with the size of the specified rectangle.
     *  The plot is rendered using anti-aliasing.
     *  This can be used to paint a number of different
     *  plots onto a single buffered image.  This method can be used, for
     *  example, by a servelet to produce an image, rather than
     *  requiring an applet to instantiate a PlotBox.
     *  @param bufferedImage Image onto which the plot is drawn.
     *  @param rectangle The size and position of the plot in the image.
     *  @param hints Rendering hints for this plot.
     *  @param transparent Indicator that the background of the plot
     *   should not be painted.
     *  @return The modified bufferedImage.
     */
    public synchronized BufferedImage exportImage(
            BufferedImage bufferedImage,
            Rectangle rectangle,
            RenderingHints hints,
            boolean transparent) {
        Graphics2D graphics = bufferedImage.createGraphics();
        graphics.addRenderingHints(_defaultImageRenderingHints());
        if( !transparent ) {
            graphics.setColor(Color.white);     // set the background color
            graphics.fill(rectangle);
        }
        _drawPlot(graphics, false , rectangle);
        return bufferedImage;
    }

    /** Draw this plot onto the provided image.
     *  This method does not paint the background, so the plot is
     *  transparent.  The plot fills the image, and is rendered
     *  using anti-aliasing.  This method can be used to overlay
     *  multiple plots on the same image, although you must use care
     *  to ensure that the axes and other labels are identical.
     *  Hence, it is usually better to simply combine data sets into
     *  a single plot.
     *  @param bufferedImage The image onto which to render the plot.
     *  @return The modified bufferedImage.
     */
    public synchronized BufferedImage exportImage(BufferedImage bufferedImage) {
        return exportImage(
                bufferedImage,
                new Rectangle(
                    bufferedImage.getWidth(),
                    bufferedImage.getHeight()),
                _defaultImageRenderingHints(),
                true);
    }

    /** Rescale so that the data that is currently plotted just fits.
     *  This is done based on the protected variables _xBottom, _xTop,
     *  _yBottom, and _yTop.  It is up to derived classes to ensure that
     *  variables are valid.
     *  This method calls repaint(), which eventually causes the display
     *  to be updated.
     */
    public synchronized void fillPlot() {
        // NOTE: These used to be _setXRange() and _setYRange() to avoid
        // confusing this with user-specified ranges.  But we want to treat
        // a fill command as a user specified range.
        // EAL, 6/12/00.
        setXRange(_xBottom, _xTop);
        setYRange(_yBottom, _yTop);
        repaint();
        // Reacquire the focus so that key bindings work.
        // NOTE: no longer needed?
        // requestFocus();
    }

    /** Return whether the plot uses color.
     *  @return True if the plot uses color.
     */
    public boolean getColor() {
        return _usecolor;
    }

    /** Convert a color name into a Color. Currently, only a very limited
     *  set of color names is supported: black, white, red, green, and blue.
     *  @param name A color name, or null if not found.
     *  @return An instance of Color.
     */
    public static Color getColorByName(String name) {
        try {
            // Check to see if it is a hexadecimal
            if(name.startsWith("#")) {
                name = name.substring(1);
            }
            Color col = new Color(Integer.parseInt(name, 16));
            return col;
        } catch (NumberFormatException e) {}
        // FIXME: This is a poor excuse for a list of colors and values.
        // We should use a hash table here.
        // Note that Color decode() wants the values to start with 0x.
        String names[][] = {
            {"black", "00000"}, {"white", "ffffff"},
            {"red", "ff0000"}, {"green", "00ff00"}, {"blue", "0000ff"}
        };
        for(int i = 0;i< names.length; i++) {
            if(name.equals(names[i][0])) {
                try {
                    Color col = new Color(Integer.parseInt(names[i][1], 16));
                    return col;
                } catch (NumberFormatException e) {}
            }
        }
        return null;
    }

    /** Return whether the grid is drawn.
     *  @return True if a grid is drawn.
     */
    public boolean getGrid() {
        return _grid;
    }

    /** Return the color of the grid (whether it is drawn or not).
     *  @return The grid color.
     */
    public Color getGridColor() {
        return _gridColor;
    }

    /** Return the color of the axis tick marks (whether they are drawn or not).
     *  @return The tick color.
     */
    public Color getTickColor() {
        return _tickColor;
    }

    /** Return whether the X-Axis is drawn.
     *  @return True if X-Axis is drawn.
     */
    public boolean getXAxis() {
        return _showXAxis;
    }

    /** Return whether the Y-Axis is drawn.
     *  @return True if Y-Axis is drawn.
     */
    public boolean getYAxis() {
        return _showYAxis;
    }

    /** Get the legend for a dataset, or null if there is none.
     *  The legend would have been set by addLegend().
     *  @param dataset The dataset index.
     *  @return The legend label, or null if there is none.
     */
    public synchronized String getLegend(int dataset) {
        int idx = _legendDatasets.indexOf(new Integer(dataset), 0);
        if (idx != -1) {
            return _legendStrings.elementAt(idx);
        } else {
            return null;
        }
    }

    /** If the size of the plot has been set by setSize(),
     *  then return that size.  Otherwise, return what the superclass
     *  returns (which is undocumented, but apparently imposes no maximum size).
     *  Currently (JDK 1.3), only BoxLayout pays any attention to this.
     *  @return The maximum desired size.
     */
    public synchronized Dimension getMaximumSize() {
        if (_sizeHasBeenSet) {
            return new Dimension(_preferredWidth, _preferredHeight);
        } else {
            return super.getMaximumSize();
        }
    }

    /** Get the minimum size of this component.
     *  This is simply the dimensions specified by setSize(),
     *  if this has been called.  Otherwise, return whatever the base
     *  class returns, which is undocumented.
     *  @return The minimum size.
     */
    public synchronized Dimension getMinimumSize() {
        if (_sizeHasBeenSet) {
            return new Dimension(_preferredWidth, _preferredHeight);
        } else {
            return super.getMinimumSize();
        }
    }

    /** Get the preferred size of this component.
     *  This is simply the dimensions specified by setSize(),
     *  if this has been called, or the default width and height
     *  otherwise (500 by 300).
     *  @return The preferred size.
     */
    public synchronized Dimension getPreferredSize() {
        return new Dimension(_preferredWidth, _preferredHeight);
    }

    /** Get the title of the graph, or an empty string if there is none.
     *  @return The title.
     */
    public synchronized String getTitle() {
        if (_title == null) return "";
        return _title;
    }

    public synchronized Color getTitleColor() {
        return _titleColor;
    }

    /** Get the label for the X (horizontal) axis, or null if none has
     *  been set.
     *  @return The X label.
     */
    public synchronized String getXLabel() {
        return _xlabel;
    }

    /** Return whether the X axis is drawn with a logarithmic scale.
     *  @return True if the X axis is logarithmic.
     */
    public boolean getXLog() {
        return _xlog;
    }

    /** Get the X range.  The returned value is an array where the first
     *  element is the minimum and the second element is the maximum.
     *  return The current X range.
     */
    public synchronized double[] getXRange() {
        double[] result = new double[2];
        if (_xRangeGiven) {
            result[0] = _xlowgiven;
            result[1] = _xhighgiven;
        } else {
            // Have to first correct for the padding.
            result[0] = _xMin + ((_xMax - _xMin) * _padding);
            result[1] = _xMax - ((_xMax - _xMin) * _padding);
        }
        return result;
    }

    /**
     *  Get the full X range, including any padding.  The returned
     *  value is an array where the first element is the minimum and
     *  the second element is the maximum.
     * @return The current X  range.
     */
    public synchronized double[] getFullXRange() {
        double[] result = new double[2];
        result[0] = _xMin;
        result[1] = _xMax;
        return result;
    }

    /**
     * Returns the X range of pixel coordinates for the data area. The
     * returned value is an array where the first element is the
     * minimum and the second element is the maximum.
     * @see #getXCanvasRange
     * @return The current X range in pixels.
     */
    public int[] getXPixelRange() {
        int[] rtn = new int[2];
        rtn[0] = (int)(_ulx + (_lrx - _ulx) * _padding);
        rtn[1] = (int)(_lrx - (_lrx - _ulx) * _padding);
        return rtn;
    }

    /**
     * Returns the X range of pixel coordinates for the canvas
     * area. The returned value is an array where the first element is
     * the minimum and the second element is the maximum.
     * @see PlotBox#getXPixelRange
     * @return The current X range in pixels.
     */
    public int[] getXCanvasRange() {
        int[] rtn = new int[2];
        rtn[0] = _ulx;
        rtn[1] = _lrx;
        return rtn;
    }

    /**
     * Converts a data X value to a pixel coordinate.
     */
    public int xDataToPix(double xval) {
        // NB: pixel is not rounded
        return (int)(_ulx + (xval  - _xMin) * _xscale /*+ 0.5*/);
    }

    /**
     * Converts an X location in pixels into a data coordinate.
     */
    public double xPixToData(int xpix) {
        // NB: round this because of no rounding in xDataToPix().
        return _xMin + (xpix - _ulx + 0.5) / _xscale;
    }

    /**
     * Set the desired aspect ratio of the user unit sizes as displayed
     * on the screen. That is, if aspectRatio is set to 0.5, one X-unit
     * will be displayed half the size of one Y-unit. If the aspect ratio
     * is set non-positive, control of the aspect ratio will be turned off.
     * @param aspectRatio The desired aspect ratio, or 0 to turn off
     * control of the aspect ratio.
     */
    public void setAspectRatio(double aspectRatio) {
        if (aspectRatio <= 0) {
             _aspectRatioGiven = false;
        } else {
            _aspectRatio = aspectRatio;
            _aspectRatioGiven = true;
        }
    }

    /** Get the X ticks that have been specified, or null if none.
     *  The return value is an array with two vectors, the first of
     *  which specifies the X tick locations (as instances of Double),
     *  and the second of which specifies the corresponding labels.
     *  @return The X ticks.
     */
    /*
    public synchronized Vector[] getXTicks() {
        if (_xticks == null) return null;
        Vector[] result = new Vector[2];
        result[0] = _xticks;
        result[1] = _xticklabels;
        return result;
    }
    */

    /** Get the label for the Y (vertical) axis, or null if none has
     *  been set.
     *  @return The Y label.
     */
    public String getYLabel() {
        return _ylabel;
    }

    /** Return whether the Y axis is drawn with a logarithmic scale.
     *  @return True if the Y axis is logarithmic.
     */
    public boolean getYLog() {
        return _ylog;
    }

    /** Get the Y range.  The returned value is an array where the first
     *  element is the minimum and the second element is the maximum.
     *  @return The current Y range.
     */
    public synchronized double[] getYRange() {
        double[] result = new double[2];
        if (_yRangeGiven) {
            result[0] = _ylowgiven;
            result[1] = _yhighgiven;
        } else {
            // Have to first correct for the padding.
            result[0] = _yMin + ((_yMax - _yMin) * _padding);
            result[1] = _yMax - ((_yMax - _yMin) * _padding);
        }
        return result;
    }

    /**
     *  Get the full Y range, including any padding.  The returned
     *  value is an array where the first element is the minimum and
     *  the second element is the maximum.
     * @return The current Y  range.
     */
    public synchronized double[] getFullYRange() {
        double[] result = new double[2];
        result[0] = _yMin;
        result[1] = _yMax;
        return result;
    }

    /**
     * Returns the Y range of pixel coordinates. The returned value is
     * an array where the first element is the minimum and the second
     * element is the maximum.
     * @see PlotBox#getXCanvasRange
     * @return The current Y range in pixels.
     */
    public int[] getYPixelRange() {
        int[] rtn = new int[2];
        rtn[0] = (int)(_uly + (_lry - _uly) * _padding);
        rtn[1] = (int)(_lry - (_lry - _uly) * _padding);
        return rtn;
    }

    /**
     * Returns the Y range of pixel coordinates for the canvas
     * area. The returned value is an array where the first element is
     * the minimum and the second element is the maximum.
     * @see PlotBox#getYPixelRange
     * @return The current Y range in pixels.
     */
    public int[] getYCanvasRange() {
        int[] rtn = new int[2];
        rtn[0] = _uly;
        rtn[1] = _lry;
        return rtn;
    }

    /**
     * Converts a data Y value to a pixel coordinate.
     */
    public int yDataToPix(double yval) {
        // NB: pixel is not rounded
        return (int)(_lry - (yval  - _yMin) * _yscale /*+ 0.5*/);
    }

    /**
     * Converts a Y location in pixels into a data coordinate.
     */
    public double yPixToData(int ypix) {
        // NB: round this because of no rounding in yDataToPix().
        return _yMin + (_lry - ypix + 0.5) / _yscale;
    }

    /** Get the Y ticks that have been specified, or null if none.
     *  The return value is an array with two vectors, the first of
     *  which specifies the Y tick locations (as instances of Double),
     *  and the second of which specifies the corresponding labels.
     *  @return The Y ticks.
     */
    /*
    public synchronized Vector[] getYTicks() {
        if (_yticks == null) return null;
        Vector[] result = new Vector[2];
        result[0] = _yticks;
        result[1] = _yticklabels;
        return result;
    }
    */

    /**
     * Whether the given dataset should be shown.
     * @param dataset The index of the dataset (from 0).
     * @return True if the data should be shown.
     */
    public boolean isShown(int dataset) {
        return (dataset >= 0 &&
                (_datasetPlotFlags.size() <= dataset
                || _datasetPlotFlags.get(dataset).booleanValue()));
    }

    /**
     * Sets whether a given dataset should be shown.
     * @param dataset The index of the dataset (from 0).
     * @param flag True to show the dataset, false to suppress it.
     */
    public void setShown(int dataset, boolean flag) {
        boolean prev = isShown(dataset);
        while (_datasetPlotFlags.size() <= dataset) {
            _datasetPlotFlags.add(new Boolean(true));
        }
        _datasetPlotFlags.set(dataset, new Boolean(flag));
        if (prev != flag) {
            repaint();
        }
    }

    /**
     * Dummy method should be overridden by derived classes
     * Set false to cause all data to be rechecked for max/min values
     * before auto-scaling.
     */
    public void setScalingValid(boolean b) {}

    /**
     * Dummy method should be overridden by derived classes
     * @return Return false if all data should be rechecked for max/min values
     * before auto-scaling.
     */
    public boolean isScalingValid(boolean b) { return true; }

    /**
     * Dummy method should be overridden by derived classes
     */
    protected void calcMinMax() {}

    /** Paint the component contents, which in this base class is
     *  only the axes.
     *  @param graphics The graphics context.
     */
    public void paintComponent(Graphics graphics) {
        super.paintComponent(graphics);
        _drawPlot(graphics.create(), true);

        // Acquire the focus so that key bindings work.
        // NOTE: no longer needed?
        // requestFocus();
        // TODO: Call this to draw to file on request.
        /*
        try {
            export(new FileOutputStream("/tmp/VjEPS"));
        } catch (FileNotFoundException fnfe) {
        }/*CMP*/
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
        export(out, x, y, ptWidth, ptHeight, creator, date, title);
    }

    /** Print the plot to a printer, represented by the specified graphics
     *  object.
     *  @param graphics The context into which the page is drawn.
     *  @param format The size and orientation of the page being drawn.
     *  @param index The zero based index of the page to be drawn.
     *  @return PAGE_EXISTS if the page is rendered successfully, or
     *   NO_SUCH_PAGE if pageIndex specifies a non-existent page.
     *  @exception PrinterException If the print job is terminated.
     */
    public synchronized int print(Graphics graphics, PageFormat format,
            int index) throws PrinterException {
        if (graphics == null) return Printable.NO_SUCH_PAGE;
        // We only print on one page.
        if (index >= 1) {
            return Printable.NO_SUCH_PAGE;
        }
        graphics.translate((int)format.getImageableX(),
                (int)format.getImageableY());
        _drawPlot(graphics, true);
        return Printable.PAGE_EXISTS;
    }

    /** Read commands and/or plot data from an input stream in the old
     *  (non-XML) file syntax.
     *  To update the display, call repaint(), or make the plot visible with
     *  setVisible(true).
     *  <p>
     *  To read from standard input, use:
     *  <pre>
     *     read(System.in);
     *  </pre>
     *  To read from a url, use:
     *  <pre>
     *     read(url.openStream());
     *  </pre>
     *  To read a URL from within an applet, use:
     *  <pre>
     *     URL url = new URL(getDocumentBase(), urlSpec);
     *     read(url.openStream());
     *  </pre>
     *  Within an application, if you have an absolute URL, use:
     *  <pre>
     *     URL url = new URL(urlSpec);
     *     read(url.openStream());
     *  </pre>
     *  To read from a file, use:
     *  <pre>
     *     read(new FileInputStream(filename));
     *  </pre>
     *  @param in The input stream.
     *  @exception IOException If the stream cannot be read.
     */
    public synchronized void read(InputStream in) throws IOException {
        try {
            // NOTE: I tried to use exclusively the jdk 1.1 Reader classes,
            // but they provide no support like DataInputStream, nor
            // support for URL accesses.  So I use the older classes
            // here in a strange mixture.

            BufferedReader din = new BufferedReader(
                    new InputStreamReader(in));

            try {
                String line = din.readLine();
                while (line != null) {
                    _parseLine(line);
                    line = din.readLine();
                }
            } finally {
                din.close();
            }
        } catch (IOException e) {
            _errorMsg = new String [2];
            _errorMsg[0] = "Failure reading input data.";
            _errorMsg[1] = e.getMessage();
            throw e;
        }
    }

    /** Read a single line command provided as a string.
     *  The commands can be any of those in the ASCII file format.
     *  @param command A command.
     */
    public synchronized void read(String command) {
        _parseLine(command);
    }

    /** Reset the X and Y axes to the ranges that were first specified
     *  using setXRange() and setYRange(). If these methods have not been
     *  called, then reset to the default ranges.
     *  This method calls repaint(), which eventually causes the display
     *  to be updated.
     */
    public synchronized void resetAxes() {
        setXRange(_originalXlow, _originalXhigh);
        setYRange(_originalYlow, _originalYhigh);
        repaint();
    }

    /** Do nothing in this base class. Derived classes might want to override
     *  this class to give an example of their use.
     */
    public void samplePlot() {
        // Empty default implementation.
    }

    /** Set the background color.
     *  @param background The background color.
     */
    public void setBackground(Color background) {
        _background = background;
        super.setBackground(_background);
    }

    /** Move and resize this component. The new location of the top-left
     *  corner is specified by x and y, and the new size is specified by
     *  width and height. This overrides the base class method to make
     *  a record of the new size.
     *  @param x The new x-coordinate of this component.
     *  @param y The new y-coordinate of this component.
     *  @param width The new width of this component.
     *  @param height The new height of this component.
     */
    public /*synchronized*/ void setBounds(int x, int y, int width, int height) {
        _width = width;
        _height = height;
        super.setBounds(x, y, _width, _height);
    }

    public void removeLegendButtons() {
        int n = _legendButtonList.size();
        for (int i = 0; i < n; i++) {
            AbstractButton button = _legendButtonList.get(i);
            if (button != null) {
                remove(button);
                _legendButtonList.set(i, (AbstractButton)null);
            }
        }
    }

    public void addLegendButton(int idx, int dataset, String legend) {
        addLegendButton(idx, dataset, legend, null);
    }

    public void addLegendButton(int idx, int dataset,
                                String legend, String tooltip) {
        while (_legendButtonList.size() <= idx) {
            _legendButtonList.add(null);
        }
        AbstractButton button = _legendButtonList.get(idx);
        LegendIcon icon = null;
        if (legend != null) {
            icon = new LegendIcon(dataset);
        }
        if (button != null) {
            if (legend != null) {
                button.setText(legend);
                button.setIcon(icon);
            }
            Set<Integer> datasetList;
            datasetList = (Set<Integer>)button.getClientProperty("datasets");
            datasetList.add(new Integer(dataset));
        } else {
            button = new JToggleButton(legend, icon, true);
            button.setFont(_labelFont);
            button.addActionListener(_legendListener);
            Set<Integer> datasetList = new TreeSet<Integer>();
            datasetList.add(new Integer(dataset));
            button.putClientProperty("datasets", datasetList);
            button.setHorizontalTextPosition(SwingConstants.LEFT);
            button.setMargin(new Insets(0, 2, 0, 4));
            add(button);
            _legendButtonList.set(idx, button);
            setShown(dataset, true);
            invalidate();
            validate();
        }
        if (tooltip != null) {
            button.setToolTipText(tooltip);
        }
    }

    /**
     * Get an Icon from the JAR file.
     * @param f The name of the icon file.
     * @return The Icon, or null.
     */
    public Icon getIcon(String f) {
        Icon imageIcon = null;
        try {
            //Class<?> c = getClass();
            Class<?> c = PlotBox.class;
            URL imageURL = c.getResource("/vnmr/images/" + f);
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
     * If the argument is true, make a fill button visible.
     * This button auto-scales the plot.
     * @param visible If true, show the button.
     */
    public synchronized void setFillButton(boolean visible) {
        if (visible && _fillButton == null) {
            Icon icon = getIcon("FullScale16.png");
            if (icon != null) {
                _fillButton = new JButton(icon);
            } else {
                _fillButton = new JButton(" Full ");
            }
            //_fillButton = new JButton(" Full ");
            _fillButton.setBackground(null);
            _fillButton.setFont(_labelFont);
            VButtonBorder border = new VButtonBorder();
            border.setBorderInsets(new Insets(5, 4, 3, 4));
            _fillButton.setBorder(border);
            _fillButton.addActionListener(_buttonListener);
            _fillButton.setToolTipText("Show all the data");
        }
        if (_fillButton != null) {
            if (visible) {
                add(_fillButton);
            } else {
                remove(_fillButton);
                _fillButton = null;
            }
        }
    }

    /**
     * Action to take when _fillButton is clicked.
     */
    protected void fillButtonClick() {
        if (_xFillRange == null) {
            unsetXRange();
        } else {
            setXRange(_xFillRange.min, _xFillRange.max);
        }
        if (_yFillRange == null) {
            unsetYRange();
        } else {
            setYRange(_yFillRange.min, _yFillRange.max);
        }
        repaint();
    }

    /**
     * Action to take when _tailButton is clicked.
     */
    protected void tailButtonClick() {
        double xMax = getMaxX();
        double dx = getDefaultDataWidth();
        setXRange(xMax - dx, xMax);
        repaint();
    }

    /**
     * Create a new button
     */
    protected synchronized JButton makeButton(String text, String icon_name) {
    	Icon icon=null;
    	JButton button=null;
    	if(icon_name !=null)
    		icon=getIcon(icon_name);
    	if(icon !=null)
        	button=new JButton(icon) ;
    	else
    		button=new JButton(text) ;
    	button.setBackground(null);
    	button.setFont(_labelFont);
        VButtonBorder border = new VButtonBorder();
        border.setBorderInsets(new Insets(5, 4, 3, 4));
        button.setBorder(border);
        button.addActionListener(_buttonListener);
        return button;
    }

    /**
     * If the argument is true, make a print button visible.
     * @param visible If true, show the button.
     */
    public synchronized void setPrintButton(boolean visible) {
        if (visible && _printButton == null) {
        	_printButton=makeButton("PRNT","print.gif");
            _printButton.setToolTipText("Print chart");
        }
        if (_printButton != null) {
            if (visible) {
                add(_printButton);
            } else {
                remove(_printButton);
                _printButton = null;
            }
        }
    }

    /**
     * Action to take when _printButton is clicked.
     */
	protected void printButtonClick() {
		PrinterJob job = PrinterJob.getPrinterJob();
		job.setPrintable(PlotBox.this);
		if (job.printDialog()) {
			try {
				job.print();
			} catch (Exception ex) {
				Component ancestor = getTopLevelAncestor();
				JOptionPane.showMessageDialog(ancestor, "Printing failed:\n"
						+ ex.toString(), "Print Error",
						JOptionPane.WARNING_MESSAGE);
				// Messages.writeStackTrace(ex);
			}
		}
	}

    /**
     * Action to take when _formatButton is clicked.
     */
    protected void formatButtonClick() {
        //PlotFormatter fmt = new PlotFormatter(PlotBox.this);
        //fmt.openModal();
    }

    /**
     * Show or hide the "Tail" button.
     * This button scales the plot to show the right end of the data.
     * @param visible If true, the button is shown.
     */
    public synchronized void setTailButton(boolean visible) {
        if (visible && _tailButton == null) {
        	_tailButton=makeButton(" Tail ","eastarrow.gif");
        	_tailButton.setToolTipText("Show the most recent data");
        }
        if (_tailButton != null) {
            if (visible) {
                add(_tailButton);
            } else {
                remove(_tailButton);
                _tailButton = null;
            }
        }
    }

   /**
     * Show or hide the "Format" button.
     * This button selects an axis format option
     * @param visible If true, the button is shown.
     */
    public synchronized void setFormatButton(boolean visible) {
        if (visible && _formatButton == null) {
        	_formatButton=makeButton("TM","busy.gif");
        }
        if (_formatButton != null) {
            if (visible) {
                add(_formatButton);
            } else {
                remove(_formatButton);
                _formatButton = null;
            }
        }
    }

    /** If the argument is false, draw the plot without using color
     *  (in black and white).  Otherwise, draw it in color (the default).
     *  @param useColor False to draw in back and white.
     */
    public void setColor(boolean useColor) {
        _usecolor = useColor;
    }

    /**
     * Get the mark color for a dataset.
     * Dataset indices start at 0 and wrap around.
     *  @param dataset The index of the data set.
     */
    public Color getMarkColor(int dataset) {
        return _colors[dataset % _colors.length];
    }

    /**
     * Set the mark color for a dataset.
     * Dataset indices start at 0 and wrap around.
     * Will not set a color to null.
     *  @param color The color to set.
     *  @param dataset The index of the data set.
     */
    public void setMarkColor(Color color, int dataset) {
        if (color != null) {
            _colors[dataset % _colors.length] = color;
        }
    }

    /** Set the foreground color.
     *  @param foreground The foreground color.
     */
    public void setForeground(Color foreground) {
        _foreground = foreground;
        super.setForeground(_foreground);
    }

    /** Set the color of the background on the plot rectangle.
     *  @param color The background color.
     */
    public void setPlotBackground(Color color) {
        _canvasColor = color;
    }

    public void setColors(Color[] colors) {
        _colors = colors;
    }

    public Color[] getColors() {
        return _colors;
    }

    /** Control whether the grid is drawn.
     *  @param grid If true, a grid is drawn.
     */
    public void setGrid(boolean grid) {
        _grid = grid;
    }

    /** Set the color of the grid.
     *  @param c The new grid color.
     */
    public void setGridColor(Color c) {
        _gridColor = c;
    }

    /** Set the color of the axis tick marks.
     *  @param c The new tick color.
     */
    public void setTickColor(Color c) {
        _tickColor = c;
    }

    /** Control whether the X-Axis is drawn.
     *  @param b If true, the axis is drawn.
     */
    public void setXAxis(boolean b) {
        _showXAxis = b;
    }

    /** Control whether the Y-Axis is drawn.
     *  @param b If true, the axis is drawn.
     */
    public void setYAxis(boolean b) {
        _showYAxis = b;
    }

    /** Set the label font, which is used for axis labels and legend labels.
     *  The font names understood are those understood by
     *  java.awt.Font.decode().
     *  @param name A font name.
     */
    public void setLabelFont(String name) {
        _labelFont = Font.decode(name);
        _labelFontMetrics = getFontMetrics(_labelFont);
        _superscriptFont = _labelFont.deriveFont(_labelFont.getSize2D() * 0.8f);
        _superscriptFontMetrics = getFontMetrics(_superscriptFont);
    }

    /** Set the label font, which is used for axis labels and legend labels.
     *  @param font A font.
     */
    public void setLabelFont(Font font) {
        _labelFont = font;
        _labelFontMetrics = getFontMetrics(_labelFont);
        _superscriptFont = _labelFont.deriveFont(_labelFont.getSize2D() * 0.8f);
        _superscriptFontMetrics = getFontMetrics(_superscriptFont);
    }

    public Font getLabelFont() {
        return _labelFont;
    }

    /** Set the size of the plot.  This overrides the base class to make
     *  it work.  In particular, it records the specified size so that
     *  getMinimumSize() and getPreferredSize() return the specified value.
     *  However, it only works if the plot is placed in its own JPanel.
     *  This is because the JPanel asks the contained component for
     *  its preferred size before determining the size of the panel.
     *  If the plot is placed directly in the content pane of a JApplet,
     *  then, mysteriously, this method has no effect.
     *  @param width The width, in pixels.
     *  @param height The height, in pixels.
     */
    public void setSize(int width, int height) {
        _width = width;
        _height = height;
        _preferredWidth = width;
        _preferredHeight = height;
        _sizeHasBeenSet = true;
        super.setSize(width, height);
    }

    public Dimension getSize() {
        return new Dimension(_width, _height);
    }

    /** Set the title of the graph.
     *  @param title The title.
     */
    public void setTitle(String title) {
        _title = title;
    }

    public void setTitleColor(Color color) {
        _titleColor = color;
    }

    /** Set the title font.
     *  The font names understood are those understood by
     *  java.awt.Font.decode().
     *  @param name A font name.
     */
    public void setTitleFont(String name) {
        _titleFont = Font.decode(name);
        _titleFontMetrics = getFontMetrics(_titleFont);
    }

    /** Set the title font.
     *  @param font A font.
     */
    public void setTitleFont(Font font) {
        _titleFont = font;
        _titleFontMetrics = getFontMetrics(_titleFont);
        /*Messages.postDebug("title height=" + _titleFontMetrics.getHeight()
                           + ", or " + (_titleFontMetrics.getAscent()
                                        +_titleFontMetrics.getDescent()));/*CMP*/
    }

    public Font getTitleFont() {
        return _titleFont;
    }

    /** Specify whether the X axis is wrapped.
     *  If it is, then X values that are out of range are remapped
     *  to be in range using modulo arithmetic. The X range is determined
     *  by the most recent call to setXRange() (or the most recent zoom).
     *  If the X range has not been set, then use the default X range,
     *  or if data has been plotted, then the current fill range.
     *  @param wrap If true, wrapping of the X axis is enabled.
     */
    public void setWrap(boolean wrap) {
        _wrap = wrap;
        if (!_xRangeGiven) {
            if (_xBottom > _xTop) {
                // have nothing to go on.
                setXRange(0, 0);
            } else {
                setXRange(_xBottom, _xTop);
            }
        }
        _wrapLow = _xlowgiven;
        _wrapHigh = _xhighgiven;
    }

    /** Set the label for the X (horizontal) axis.
     *  @param label The label.
     */
    public void setXLabel(String label) {
        _xlabel = label;
    }

    /**
     * Specify whether the X axis is drawn with a time-of-day scale.
     * If true, X values are the number of ms of the Unix era
     * (ms since Jan 0, 1970, 0:00).
     * @param b If true, time/date is plotted on the X-axis.
     */
    public void setXTimeOfDay(boolean b) {
        _xtime = b;
    }

    /** Specify whether the X axis is drawn with a logarithmic scale.
     *  Trying to plot non-positive values is an error.
     *  @param xlog If true, logarithmic axis is used.
     */
    public void setXLog(boolean xlog) {
        _xlog = xlog;
        _xlogNoNegVals = true;
    }

    /** Specify whether the X axis is drawn with a logarithmic scale.
     *  If it is, set the value to plot for out-of-range (non-positive)
     *  X values.
     *  @param xlog If true, logarithmic axis is used.
     *  @param xdefault The value to use for non-positive X values.
     */
    public void setXLog(boolean xlog, double xdefault) {
        _xlog = xlog;
        _xlogNoNegVals = false;
        _xlogNegDefault = xdefault;
    }

    /**
     * The largest X value we have.
     * This is overridden by subclasses that actually have the data.
     */
    public double getMaxX() {
        return 0;
    }

    public double getDefaultDataWidth() {
        if (_walkingWidth > 0) {
            return _walkingWidth;
        } else {
            // Not set, just return current width
            double[] xr = getXRange();
            return xr[1] - xr[0];
        }
    }

    /** Set whether the window will "walk".
     *  If true, when addPoint() is called to add a point outside of
     *  the XRange, and the previous point was inside the XRange,
     *  the range is moved just enough to keep the new point in range.
     */
    public void setWalking(boolean b) {
        _walkingWindow = b;
        double[] xr = getXRange();
        _walkingWidth = xr[1] - xr[0];
        return;
    }

    /** Set whether the window will "walk".
     *  If true, when addPoint() is called to add a point outside of
     *  the XRange, and the previous point was inside the XRange,
     *  the range is moved just enough to keep the new point in range.
     * @param b If true, then walking will be enabled.
     * @param width The default width of the walking window in X units.
     */
    public void setWalking(boolean b, double width) {
        _walkingWindow = b;
        _walkingWidth = width;
        return;
    }

    /** Return whether the window will "walk".
     *  If true, when a point is added outside of
     *  the XRange, and the previous point was inside the XRange,
     *  the range is moved just enough to keep the new point in range.
     */
    public boolean getWalking() {
        return _walkingWindow;
    }

    public void setXFillRange(double min, double max) {
        if (min >= max) {
            _xFillRange = null;
        } else {
            _xFillRange = new Range(min, max);
        }
    }

    public void setYFillRange(double min, double max) {
        if (min >= max) {
            _yFillRange = null;
        } else {
            _yFillRange = new Range(min, max);
        }
    }

    /** Set the X (horizontal) range of the plot.  If this is not done
     *  explicitly, then the range is computed automatically from data
     *  available when the plot is drawn.  If min and max
     *  are identical, then the range is arbitrarily spread by 1.
     *  @param min The left extent of the range.
     *  @param max The right extent of the range.
     */
    public synchronized void setXRange(double min, double max) {
        _xRangeGiven = true;
        _xlowgiven = min;
        _xhighgiven = max;
        if (!_originalXRangeGiven) {
            _originalXlow = min;
            _originalXhigh = max;
            _originalXRangeGiven = true;
        }
        _setXRange(min, max);
    }
    /** Sets XRange the same as above, but with no padding*/
     public synchronized void setXFullRange(double min, double max) {
        _xRangeGiven = true;
        _xlowgiven = min;
        _xhighgiven = max;
        if (!_originalXRangeGiven) {
            _originalXlow = min;
            _originalXhigh = max;
            _originalXRangeGiven = true;
        }
        _setPadding(0);
        _setXRange(min, max);
    }

    public boolean isXRangeGiven() {
        return _xRangeGiven;
    }

    public boolean isYRangeGiven() {
        return _yRangeGiven;
    }

    /**
     *  Unset the X (horizontal) range of the plot.  The range is
     *  computed automatically from data available when the plot is
     *  drawn.
     */
    public void unsetXRange() {
        _xRangeGiven = false;
    }

    /** Set the label for the Y (vertical) axis.
     *  @param label The label.
     */
    public void setYLabel(String label) {
        _ylabel = label;
    }

    /** Specify whether the Y axis is drawn with a logarithmic scale.
     *  Trying to plot non-positive values is an error.
     *  @param ylog If true, logarithmic axis is used.
     */
    public void setYLog(boolean ylog) {
        _ylog = ylog;
        _ylogNoNegVals = true;
    }

    /** Specify whether the Y axis is drawn with a logarithmic scale.
     *  If it is, set the value to plot for out-of-range (non-positive)
     *  Y values.
     *  @param ylog If true, logarithmic axis is used.
     *  @param ydefault The value to use for non-logable Y values.
     */
    public void setYLog(boolean ylog, double ydefault) {
        _ylog = ylog;
        _ylogNoNegVals = false;
        _ylogNegDefault = ydefault;
    }

    /** Set the Y (vertical) range of the plot.  If this is not done
     *  explicitly, then the range is computed automatically from data
     *  available when the plot is drawn.  If min and max are identical,
     *  then the range is arbitrarily spread by 0.1.
     *  @param min The bottom extent of the range.
     *  @param max The top extent of the range.
     */
    public synchronized void setYRange(double min, double max) {
        _yRangeGiven = true;
        _ylowgiven = min;
        _yhighgiven = max;
        if (!_originalYRangeGiven) {
            _originalYlow = min;
            _originalYhigh = max;
            _originalYRangeGiven = true;
        }
        _setYRange(min, max);
    }
     /** Sets YRange the same as above, but with no padding*/
     public synchronized void setYFullRange(double min, double max) {
        _yRangeGiven = true;
        _ylowgiven = min;
        _yhighgiven = max;
        if (!_originalYRangeGiven) {
            _originalYlow = min;
            _originalYhigh = max;
            _originalYRangeGiven = true;
        }
        _setPadding(0);
        _setYRange(min, max);
    }

    /**
     *  Unset the Y (vertical) range of the plot.  The range is
     *  computed automatically from data available when the plot is
     *  drawn.
     */
    public void unsetYRange() {
        _yRangeGiven = false;
    }

    /** Write the current data and plot configuration to the
     *  specified stream in PlotML syntax.  PlotML is an XML
     *  extension for plot data.  The written information is
     *  standalone, in that it includes the DTD (document type
     *  definition).  This makes is somewhat verbose.  To get
     *  smaller files, use the two argument version of write().
     *  The output is buffered, and is flushed and
     *  closed before exiting.  Derived classes should override
     *  writeFormat and writeData rather than this method.
     *  @param out An output stream.
     */
    public void write(OutputStream out) {
        write(out, null);
    }

    /** Write the current data and plot configuration to the
     *  specified stream in PlotML syntax.  PlotML is an XML
     *  scheme for plot data. The URL (relative or absolute) for the DTD is
     *  given as the second argument.  If that argument is null,
     *  then the PlotML PUBLIC DTD is referenced, resulting in a file
     *  that can be read by a PlotML parser without any external file
     *  references, as long as that parser has local access to the DTD.
     *  The output is buffered, and is flushed and
     *  closed before exiting.  Derived classes should override
     *  writeFormat and writeData rather than this method.
     *  @param out An output stream.
     *  @param dtd The reference (URL) for the DTD, or null to use the
     *   PUBLIC DTD.
     */
    public synchronized void write(OutputStream out, String dtd) {
        write(new OutputStreamWriter(out), dtd);
    }

    /** Write the current data and plot configuration to the
     *  specified stream in PlotML syntax.  PlotML is an XML
     *  scheme for plot data. The URL (relative or absolute) for the DTD is
     *  given as the second argument.  If that argument is null,
     *  then the PlotML PUBLIC DTD is referenced, resulting in a file
     *  that can be read by a PlotML parser without any external file
     *  references, as long as that parser has local access to the DTD.
     *  The output is buffered, and is flushed and
     *  closed before exiting.
     *  @param out An output writer.
     *  @param dtd The reference (URL) for the DTD, or null to use the
     *   PUBLIC DTD.
     */
    public synchronized void write(Writer out, String dtd) {
        // Auto-flush is disabled.
        PrintWriter output = new PrintWriter(new BufferedWriter(out), false);
        if (dtd == null) {
            output.println("<?xml version=\"1.0\" standalone=\"yes\"?>");
            output.println(
                    "<!DOCTYPE plot PUBLIC \"-//UC Berkeley//DTD PlotML 1//EN\"");
            output.println(
                    "    \"http://ptolemy.eecs.berkeley.edu/xml/dtd/PlotML_1.dtd\">");
        } else {
            output.println("<?xml version=\"1.0\" standalone=\"no\"?>");
            output.println("<!DOCTYPE plot SYSTEM \"" + dtd + "\">");
        }
        output.println("<plot>");
        output.println("<!-- Ptolemy plot, version " + PTPLOT_RELEASE
                + " , PlotML format. -->");
        writeFormat(output);
        writeData(output);
        output.println("</plot>");
        output.flush();
        // NOTE: We used to close the stream, but if this is part
        // of an exportMoML operation, that is the wrong thing to do.
        // if(out != System.out) {
        //    output.close();
        // }
    }

    /** Write plot data information to the specified output stream in PlotML.
     *  In this base class, there is no data to write, so this method
     *  returns without doing anything.
     *  @param output A buffered print writer.
     */
    public synchronized void writeData(PrintWriter output) {
    }

    /** Write plot format information to the specified output stream in PlotML.
     *  Derived classes should override this method to first call
     *  the parent class method, then add whatever additional format
     *  information they wish to add to the stream.
     *  @param output A buffered print writer.
     */
    public synchronized void writeFormat(PrintWriter output) {
        // NOTE: If you modify this, you should change the _DTD variable
        // accordingly.
        if (_title != null) output.println(
                "<title>" + _title + "</title>");
        if (_xlabel != null) output.println(
                "<xLabel>" + _xlabel + "</xLabel>");
        if (_ylabel != null) output.println(
                "<yLabel>" + _ylabel + "</yLabel>");
        if (_xRangeGiven) output.println(
                "<xRange min=\"" + _xlowgiven + "\" max=\""
                + _xhighgiven + "\"/>");
        if (_yRangeGiven) output.println(
                "<yRange min=\"" + _ylowgiven + "\" max=\""
                + _yhighgiven + "\"/>");
        if (_xticks != null && _xticks.size() > 0) {
            output.println("<xTicks>");
            int last = _xticks.size() - 1;
            for (int i = 0; i <= last; i++) {
                output.println("  <tick label=\""
                        + (String)_xticklabels.elementAt(i) + "\" position=\""
                        + _xticks.elementAt(i) + "\"/>");
            }
            output.println("</xTicks>");
        }
        if (_yticks != null && _yticks.size() > 0) {
            output.println("<yTicks>");
            int last = _yticks.size() - 1;
            for (int i = 0; i <= last; i++) {
                output.println("  <tick label=\""
                        + (String)_yticklabels.elementAt(i) + "\" position=\""
                        + (Double)_yticks.elementAt(i) + "\"/>");
            }
            output.println("</yTicks>");
        }
        if (_xlog) output.println("<xLog/>");
        if (_ylog) output.println("<yLog/>");
        if (!_grid) output.println("<noGrid/>");
        if (_wrap) output.println("<wrap/>");
        if (!_usecolor) output.println("<noColor/>");
    }

    /** Zoom in or out to the specified rectangle.
     *  This method calls repaint().
     *  @param lowx The low end of the new X range.
     *  @param lowy The low end of the new Y range.
     *  @param highx The high end of the new X range.
     *  @param highy The high end of the new Y range.
     */
    public synchronized void zoom(double lowx, double lowy,
            double highx, double highy) {
        _setXRange(lowx, highx);
        _setYRange(lowy, highy);
        repaint();
    }

    public boolean zoomin(){
    	return _zoomin;
    }
    public void setImage(Image image){
        _img= image;
        _imgSize = null;
    }

    public void setImage(Image image, int width, int height) {
        _img= image;
        _imgSize = new Rectangle(width, height);
    }

    public Rectangle getLegendRectangle(){
        return _legendRectangle;
    }
        

    ///////////////////////////////////////////////////////////////////
    ////                         public variables                  ////

    public static final String PTPLOT_RELEASE = "5.1p2";

    ///////////////////////////////////////////////////////////////////
    ////                         protected methods                 ////

    /** If this method is called in the event thread, then simply
     * execute the specified action.  Otherwise,
     * if there are already deferred actions, then add the specified
     * one to the list.  Otherwise, create a list of deferred actions,
     * if necessary, and request that the list be processed in the
     * event dispatch thread.
     *
     * Note that it does not work nearly as well to simply schedule
     * the action yourself on the event thread because if there are a
     * large number of actions, then the event thread will not be able
     * to keep up.  By grouping these actions, we avoid this problem.
     *
     * This method is not synchronized, so the caller should be.
     * @param action The Runnable object to execute.
     */
    protected void _deferIfNecessary(Runnable action) {
        // In swing, updates to showing graphics must be done in the
        // event thread.  If we are in the event thread, then proceed.
        // Otherwise, queue a request or add to a pending request.

        if(EventQueue.isDispatchThread()) {
            action.run();
        } else {

            if (_deferredActions == null) {
                _deferredActions = new LinkedList<Runnable>();
            }
            // Add the specified action to the list of actions to perform.
            _deferredActions.add(action);

            // If it hasn't already been requested, request that actions
            // be performed in the event dispatch thread.
            if (!_actionsDeferred) {
                Runnable doActions = new Runnable() {
                    public void run() {
                        _executeDeferredActions();
                    }
                };
                try {
                    // NOTE: Using invokeAndWait() here risks causing
                    // deadlock.  Don't do it!
                    SwingUtilities.invokeLater(doActions);
                } catch (Exception ex) {
                    // Ignore InterruptedException.
                    // Other exceptions should not occur.
                    //Messages.writeStackTrace(ex);
                }
                _actionsDeferred = true;
            }
        }
    }

    /**
     * Draw the background for the plotting rectangle.
     * Derived classes may override this to put up a special
     * background for the plot.
     */
    protected void _drawCanvas(Graphics g, int x, int y, int wd, int ht) {
        // NOTE: The dataset colors were designed for a white background.
        if (wd < 0 || ht < 0) {
            return;
        }
        if (_img != null) {
            int ppdx = 0;       // pixels per data point
            int ppdy = 0;
            Graphics gclip = g;
            if (_imgSize != null) {
                // Stretch the image to line up the center of the first/last
                // data boxes with the edges of the drawing rectangle.
                gclip = g.create();
                gclip.setClip(x, y, wd, ht);
                int imgWd = _imgSize.width;
                if (imgWd > 1) {
                    ppdx = wd / (imgWd - 1);
                }
                int imgHt = _imgSize.height;
                if (imgHt > 1) {
                    ppdy = ht / (imgHt - 1);
                }
            }
            gclip.drawImage(_img, x - ppdx / 2, y - ppdy / 2,
                            wd + ppdx, ht + ppdy, this);
        } else {
            g.setColor(_canvasColor);
            g.fillRect(x, y, wd, ht);
        }
        g.setColor(_foreground); // _foreground is the font color
        g.drawRect(x, y, wd, ht);
    }

    /** Draw the axes using the current range, label, and title information.
     *  If the second argument is true, clear the display before redrawing.
     *  This method is called by paintComponent().  To cause it to be called
     *  you would normally call repaint(), which eventually causes
     *  paintComponent() to be called.
     *  <p>
     *  Note that this is synchronized so that points are not added
     *  by other threads while the drawing is occurring.  This method
     *  should be called only from the event dispatch thread, consistent
     *  with swing policy.
     *  @param graphics The graphics context.
     *  @param clearfirst If true, clear the plot before proceeding.
     */
    protected synchronized void _drawPlot(Graphics graphics,
                                          boolean clearfirst) {
        Rectangle bounds = getBounds();
        _drawPlot(graphics, clearfirst, bounds);
    }


    protected synchronized void _drawPlot(Graphics graphics,Rectangle bounds) {
    	_drawPlot(graphics, true, bounds);
	}

    /** Draw the axes using the current range, label, and title information,
     *  at the size of the specified rectangle.
     *  If the second argument is true, clear the display before redrawing.
     *  This method is called by paintComponent().  To cause it to be called
     *  you would normally call repaint(), which eventually causes
     *  paintComponent() to be called.
     *  <p>
     *  Note that this is synchronized so that points are not added
     *  by other threads while the drawing is occurring.  This method
     *  should be called only from the event dispatch thread, consistent
     *  with swing policy.
     *  @param graphics The graphics context.
     *  @param clearfirst If true, clear the plot before proceeding.
     *  @param drawRect A specification of the size.
     */
    protected synchronized void _drawPlot(
            Graphics graphics, boolean clearfirst, Rectangle drawRect) {
        // Ignore if there is no graphics object to draw on.
        if (graphics == null) {
            return;
        }

        graphics.setPaintMode();

        /* NOTE: The following seems to be unnecessary with Swing...
           if (clearfirst) {
           // NOTE: calling clearRect() here permits the background
           // color to show through, but it messes up printing.
           // Printing results in black-on-black title and axis labels.
           graphics.setColor(_background);
           graphics.drawRect(0, 0, drawRect.width, drawRect.height);
           graphics.setColor(Color.black);
           }
        */

        // If an error message has been set, display it and return.
        if (_errorMsg != null) {
            int fheight = _labelFontMetrics.getHeight() + 2;
            int msgy = fheight;
            graphics.setColor(Color.red);
            for(int i = 0; i < _errorMsg.length;i++) {
                graphics.drawString(_errorMsg[i], 10, msgy);
                msgy += fheight;
                System.err.println(_errorMsg[i]);
            }
            return;
        }

        // Make sure we have an x and y range
        if (!_xRangeGiven) {
            if (_xBottom > _xTop) {
                // have nothing to go on.
                _setXRange(0, 0);
            } else {
                _setXRange(_xBottom, _xTop);
            }
        }
        if (!_yRangeGiven) {
            if (_yBottom > _yTop) {
                // have nothing to go on.
                _setYRange(0, 0);
            } else {
                _setYRange(_yBottom, _yTop);
            }
        }

        // Various plot parameters that get set in the following "do" loop
        int titley;
        int ind;
        int labelheight;
        int halflabelheight;
        double yStart; 
        double yStep;
        int ny;
        String ylabels[];
        int ylabwidth[];
        int ySPos;
        int xSPos;
        Font previousFont;
        int width;
        int height;
        int prevWd;
        int prevHt;
        do {
            prevWd = _lrx - _ulx;
            prevHt = _lry - _uly;
            if (_aspectRatioGiven) {
                double[] xrange = getFullXRange();
                double xunits = xrange[1] - xrange[0];
                int[] xPrange = getXCanvasRange();
                double xpixels = (xPrange[1] - xPrange[0]);
                double xscale = xunits / xpixels;

                double[] yrange = getFullYRange();
                double yunits = yrange[1] - yrange[0];
                int[] yPrange = getYCanvasRange();
                double ypixels = (yPrange[1] - yPrange[0]);
                double yscale = yunits / ypixels;

                /*
            System.out.println("_xMin=" + _xMin + ", _xMax=" + _xMax
                               + ", _yMin=" + _yMin + ", _yMax=" + _yMax);
            System.out.println("xpixels=" + xpixels
                               + ", ypixels=" + ypixels
                               + ", ratio=" + (xpixels / ypixels));
            System.out.println("xunits=" + xunits
                               + ", yunits=" + yunits);
            System.out.println("xscale=" + xscale
                               + ", yscale=" + yscale);
                 */

                double padFactor = 1 - 2 * _padding;
                /*System.out.println("padFactor=" + padFactor);*/
                if (xscale / yscale < _aspectRatio) {
                    // Need to show more x -- desired width of range is xunitsNew
                    double xunitsNew = padFactor * xpixels * yscale * _aspectRatio;
                    // Need additional "delta" units at each end of old range
                    double delta = (xunitsNew - xunits) / 2;
                    _setXRange(xrange[0] - delta, xrange[1] + delta);
                    /*System.out.println("setXRange=" + (xrange[0] - delta)
                  + ", " + (xrange[1] + delta));*/
                } else {
                    // Need to show more y -- desired width of range is yunitsNew
                    double yunitsNew = padFactor * ypixels * xscale / _aspectRatio;
                    // Need additional "delta" units at each end of old range
                    double delta = (yunitsNew - yunits) / 2;
                    _setYRange(yrange[0] - delta, yrange[1] + delta);
                    /*System.out.println("setYRange=" + (yrange[0] - delta)
                  + ", " + (yrange[1] + delta));*/
                }
            }

            // Vertical space for title, if appropriate.
            // NOTE: We assume a one-line title.
            titley = 0;
            labelheight = 0;
            halflabelheight = 0;
            int titlefontheight = _titleFontMetrics.getHeight();

            if (_title == null) {
                // NOTE: If the _title is null, then set it to the empty
                // string to solve the problem where the fill button overlaps
                // the legend if there is no title.  The fix here would
                // be to modify the legend printing text so that it takes
                // into account the case where there is no title by offsetting
                // just enough for the button.
                ////_title = "";
            }
            if ((_title != null && _title.length() != 0) || _yExp != 0) {
                titley = titlefontheight + _topPadding;
            }

            // Number of vertical tick marks depends on the height of the font
            // for labeling ticks and the height of the window.
            previousFont = graphics.getFont();
            graphics.setFont(_labelFont);
            graphics.setColor(_foreground); // foreground color not set here  --Rob.
            //int labelheight = _labelFontMetrics.getHeight();
            labelheight = _labelFontMetrics.getAscent();
            halflabelheight = labelheight/2;

            // Draw scaling annotation for x axis.
            // NOTE: 5 pixel padding on bottom.
            ySPos = drawRect.height - 5;
            xSPos = drawRect.width - _rightPadding;
            if (_showXAxis) {
                if (_xlog)
                    _xExp = (int)Math.floor(_xtickMin);
                if (_xExp != 0 && _xticks == null) {
                    String superscript = Integer.toString(_xExp);
                    xSPos -= _superscriptFontMetrics.stringWidth(superscript);
                    graphics.setFont(_superscriptFont);
                    if (!_xlog) {
                        graphics.setColor(_foreground);
                        graphics.drawString(superscript, xSPos,
                                            ySPos - halflabelheight);
                        xSPos -= _labelFontMetrics.stringWidth("x10");
                        graphics.setFont(_labelFont);
                        graphics.drawString("x10", xSPos, ySPos);
                    }
                    // NOTE: 5 pixel padding on bottom
                    _bottomPadding = (3 * labelheight)/2 + 5;
                }
            }

            // NOTE: 5 pixel padding on the bottom.
            if (_xlabel != null && _bottomPadding < labelheight + 5) {
                _bottomPadding = labelheight + 5;
            }

            // Compute the space needed around the plot, starting with vertical.
            // NOTE: padding of 5 pixels below title.
            _uly = titley + 5;
            // NOTE: 3 pixels above bottom labels.
            int old_lry = _lry;
            _lry = drawRect.height-labelheight-_bottomPadding-3;
            if (old_lry != _lry) {
                // Need to re-layout any buttons
                // TODO: Only do this if there are buttons?
                invalidate();
                validate();
            }
            height = _lry-_uly;
            _yscale = height/(_yMax - _yMin);
            _ytickscale = height/(_ytickMax - _ytickMin);

            ////////////////// vertical axis

            // Number of y tick marks.
            // NOTE: subjective spacing factor.
            ny = 2 + height / (labelheight + 20);
            // Compute y increment.
            yStep = _roundUp((_ytickMax - _ytickMin) / (double)ny);

            // Compute y starting point so it is a multiple of yStep.
            yStart = yStep*Math.ceil(_ytickMin/yStep);

            // NOTE: Following disables first tick.  Not a good idea?
            // if (yStart == _ytickMin) yStart += yStep;

            // Define the strings that will label the y axis.
            // Meanwhile, find the width of the widest label.
            // The labels are quantized so that they don't have excess resolution.
            // Start with a minimum width, so that the graph position doesn't
            // change so much.
            int widesty = _labelFontMetrics.stringWidth("-1.0");

            // These do not get used unless ticks are automatic, but the
            // compiler is not smart enough to allow us to reference them
            // in two distinct conditional clauses unless they are
            // allocated outside the clauses.
            ylabels = new String[ny];
            ylabwidth = new int[ny];

            ind = 0;
            if (height > 0 && _showYAxis) {
                if (_yticks == null) {
                    Vector<Double> ygrid = null;
                    if (_ylog) {
                        ygrid = _gridInit(yStart, yStep, true, null);
                    }

                    // automatic ticks
                    // First, figure out how many digits after the decimal point
                    // will be used.
                    int numfracdigits = _numFracDigits(yStep);

                    // NOTE: Test cases kept in case they are needed again.
                    // System.out.println("0.1 with 3 digits: " +
                    //                    _formatNum(0.1, 3));
                    // System.out.println("0.0995 with 3 digits: " +
                    //                    _formatNum(0.0995, 3));
                    // System.out.println("0.9995 with 3 digits: " +
                    //                    _formatNum(0.9995, 3));
                    // System.out.println("1.9995 with 0 digits: " +
                    //                    _formatNum(1.9995, 0));
                    // System.out.println("1 with 3 digits: " + _formatNum(1, 3));
                    // System.out.println("10 with 0 digits: " + _formatNum(10, 0));
                    // System.out.println("997 with 3 digits: " +
                    //                    _formatNum(997, 3));
                    // System.out.println("0.005 needs: " + _numFracDigits(0.005));
                    // System.out.println("1 needs: " + _numFracDigits(1));
                    // System.out.println("999 needs: " + _numFracDigits(999));
                    // System.out.println("999.0001 needs: "+
                    //                    _numFracDigits(999.0001));
                    // System.out.println("0.005 integer digits: " +
                    //                    _numIntDigits(0.005));
                    // System.out.println("1 integer digits: " + _numIntDigits(1));
                    // System.out.println("999 integer digits: " +
                    //                    _numIntDigits(999));
                    // System.out.println("-999.0001 integer digits: " +
                    //                    _numIntDigits(999.0001));

                    double yTmpStart = yStart;
                    if (_ylog)
                        yTmpStart = _gridStep(ygrid, yStart, yStep, _ylog);

                    for (double ypos = yTmpStart; ypos <= _ytickMax;
                    ypos = _gridStep(ygrid, ypos, yStep, _ylog)) {
                        // Prevent out of bounds exceptions
                        if (ind >= ny) break;
                        String yticklabel;
                        if (_ylog) {
                            yticklabel = _formatLogNum(ypos, numfracdigits);
                        } else {
                            yticklabel = _formatNum(ypos, numfracdigits);
                        }
                        ylabels[ind] = yticklabel;
                        int lw = _labelFontMetrics.stringWidth(yticklabel);
                        ylabwidth[ind++] = lw;
                        if (lw > widesty) {widesty = lw;}
                    }
                } else {
                    // explicitly specified ticks
                    Enumeration<String> nl = _yticklabels.elements();
                    while (nl.hasMoreElements()) {
                        String label = (String) nl.nextElement();
                        int lw = _labelFontMetrics.stringWidth(label);
                        if (lw > widesty) {widesty = lw;}
                    }
                }
            }

            // Next we do the horizontal spacing.
            if (_ylabel != null) {
                _ulx = widesty + labelheight + _leftPadding;
            } else {
                _ulx = widesty + _leftPadding;
            }
            int legendwidth = _drawLegend(graphics,
                                          drawRect.width-_rightPadding, _uly);
            _lrx = drawRect.width-legendwidth-_rightPadding;
            _lrx = Math.min(_lrx, drawRect.width - _buttonWidths);
            width = _lrx - _ulx;

            // If plot range was adjusted for given aspect ratio, and plot
            // shape changed during this calculation, we redo the adjustment
            // using the new plot shape.
        //} while (_aspectRatioGiven && (width != prevWd || height != prevHt));
        } while (_aspectRatioGiven && (width * prevHt != height * prevWd));

        //////////////////// Draw title now.
        // Center the title over the plotting region, not
        // the window.
        graphics.setColor(_foreground);

        if (_title != null) {
            graphics.setFont(_titleFont);
            int titlex = _ulx +
                (width - _titleFontMetrics.stringWidth(_title))/2;
            Color oldColor = graphics.getColor();
            graphics.setColor(_titleColor);
            graphics.drawString(_title, titlex, titley);
            graphics.setColor(oldColor);

        }

        if (height <= 0 || width <= 0) {
            return;
        }

        _xscale = width/(_xMax - _xMin);
        _xtickscale = width/(_xtickMax - _xtickMin);

        _drawCanvas(graphics, _ulx, _uly, width, height);

        // NOTE: subjective tick length.
        int tickLength = 5;
        int xCoord1 = _ulx+tickLength;
        int xCoord2 = _lrx-tickLength;

        if (_showYAxis) {
            graphics.setFont(_labelFont);
            if (_yticks == null) {
                // auto-ticks
                Vector<Double> ygrid = null;
                double yTmpStart = yStart;
                if (_ylog) {
                    ygrid = _gridInit(yStart, yStep, true, null);
                    yTmpStart = _gridStep(ygrid, yStart, yStep, _ylog);
                    ny = ind;
                }
                ind = 0;
                // Set to false if we don't need the exponent
                boolean needExponent = _ylog;

                for (double ypos = yTmpStart; ypos <= _ytickMax;
                     ++ind, ypos = _gridStep(ygrid, ypos, yStep, _ylog))
                {
                    // Prevent out of bounds exceptions
                    if (ind >= ny) break;
                    int yCoord1 = _lry - (int)((ypos-_ytickMin)*_ytickscale);

                    // Don't try to draw off-of-plot stuff
                    if (yCoord1 < _uly || yCoord1 > _lry) continue;

                    // NB: Don't let label go past top or bottom of canvas
                    int ylabel = Math.min(yCoord1 + halflabelheight, _lry);
                    ylabel = Math.max(ylabel, _uly + labelheight);
                    graphics.setColor(_tickColor);
                    graphics.drawLine(_ulx+1, yCoord1, xCoord1, yCoord1);
                    graphics.drawLine(_lrx-1, yCoord1, xCoord2, yCoord1);
                    if (_grid && yCoord1 != _uly && yCoord1 != _lry) {
                        graphics.setColor(_gridColor);
                        graphics.drawLine(xCoord1, yCoord1, xCoord2, yCoord1);
                        graphics.setColor(_foreground);
                    }
                    // Check to see if any of the labels printed contain
                    // the exponent.  If we don't see an exponent, then print it.
                    if (_ylog && ylabels[ind] != null && ylabels[ind].indexOf('e') != -1 )
                        needExponent = false;

                    // NOTE: 4 pixel spacing between axis and labels.
                    graphics.setColor(_foreground);
                    if (ylabels[ind] != null) {
                        graphics.drawString(ylabels[ind],
                                            _ulx-ylabwidth[ind]-4, ylabel);
                    }
                }

                if (_ylog) {
                    // Draw in grid lines that don't have labels.
                    Vector<Double> unlabeledgrid  = _gridInit(yStart, yStep, false, ygrid);
                    if (unlabeledgrid.size() > 0) {
                        // If the step is greater than 1, clamp it to 1 so that
                        // we draw the unlabeled grid lines for each
                        //integer interval.
                        double tmpStep = (yStep > 1.0)? 1.0 : yStep;

                        for (double ypos = _gridStep(unlabeledgrid , yStart,
                                tmpStep, _ylog);
                             ypos <= _ytickMax;
                             ypos = _gridStep(unlabeledgrid, ypos,
                                     tmpStep, _ylog)) {
                            int yCoord1 = _lry -
                                (int)((ypos-_ytickMin)*_ytickscale);
                            if (_grid && yCoord1 != _uly && yCoord1 != _lry) {
                                graphics.setColor(_gridColor);
                                graphics.drawLine(_ulx+1, yCoord1,
                                                  _lrx-1, yCoord1);
                                graphics.setColor(_foreground);
                            }
                        }
                    }

                    if (needExponent) {
                        // We zoomed in, so we need the exponent
                        _yExp = (int)Math.floor(yTmpStart);
                    } else {
                        _yExp = 0;
                    }
                }

                // Draw scaling annotation for y axis.
                if (_yExp != 0) {
                    graphics.setColor(_foreground);
                    graphics.drawString("x10", 2, titley);
                    graphics.setFont(_superscriptFont);
                    graphics.drawString(Integer.toString(_yExp),
                            _labelFontMetrics.stringWidth("x10") + 2,
                            titley-halflabelheight);
                    graphics.setFont(_labelFont);
                }
            } else {
                // ticks have been explicitly specified
                Enumeration<Double> nt = _yticks.elements();
                Enumeration<String> nl = _yticklabels.elements();

                while (nl.hasMoreElements()) {
                    String label = (String) nl.nextElement();
                    double ypos = ((Double)(nt.nextElement())).doubleValue();
                    if (ypos > _yMax || ypos < _yMin) continue;
                    int yCoord1 = _lry - (int)((ypos-_yMin)*_yscale);
                    // NB: Don't let label go past top or bottom of canvas
                    int ylabel = Math.min(yCoord1 + halflabelheight, _lry);
                    ylabel = Math.max(ylabel, _uly + labelheight);

                    graphics.setColor(_tickColor);
                    graphics.drawLine(_ulx+1, yCoord1, xCoord1, yCoord1);
                    graphics.drawLine(_lrx-1, yCoord1, xCoord2, yCoord1);
                    if (_grid && yCoord1 != _uly && yCoord1 != _lry) {
                        graphics.setColor(_gridColor);
                        graphics.drawLine(xCoord1, yCoord1, xCoord2, yCoord1);
                    }
                    // NOTE: 3 pixel spacing between axis and labels.
                    graphics.setColor(_foreground);
                    graphics.drawString(label,
                            _ulx - _labelFontMetrics.stringWidth(label) - 3,
                            ylabel);
                }
            }
        }

        //////////////////// horizontal axis
        int yCoord1 = _uly+tickLength;
        int yCoord2 = _lry-tickLength;
        if (_showXAxis) {
            if (_xticks == null) {
                // auto-ticks

                // Number of x tick marks.
                // Need to start with a guess and converge on a solution here.
                int nx = 10;
                double xStep = 0.0;
                int numfracdigits = 0;
                int charwidth = _labelFontMetrics.stringWidth("8");
                if (_xlog) {
                    // X axes log labels will be at most 6 chars: -1E-02
                    nx = 2 + width/((charwidth * 6) + 10);
                } else {
                    // Limit to 10 iterations
                    int count = 0;
                    while (count++ <= 10) {
                        if (_xtime) {
                            xStep = _roundUpDate((_xtickMax - _xtickMin)
                                                 / (double)nx);
                        } else {
                            xStep = _roundUp((_xtickMax - _xtickMin)
                                             / (double)nx);
                        }
                        // Compute the width of a label for this xStep
                        numfracdigits = _numFracDigits(xStep);
                        // Number of integer digits is the maximum of two endpoints
                        int intdigits = _numIntDigits(_xtickMax);
                        int inttemp = _numIntDigits(_xtickMin);
                        if (intdigits < inttemp) {
                            intdigits = inttemp;
                        }
                        // Allow two extra digits (decimal point and sign).
                        int maxlabelwidth = charwidth *
                            (numfracdigits + 2 + intdigits);
                        // Compute new estimate of number of ticks.
                        int savenx = nx;
                        // NOTE: Subjective additional pixels between labels.
                        // NOTE: Try to ensure at least two tick marks.
                        nx = 2 + width / (maxlabelwidth + 20);
                        if (nx - savenx <= 1 || savenx - nx <= 1) break;
                    }
                }
                if (_xtime) {
                    xStep = _roundUpDate((_xtickMax - _xtickMin)
                                         / (double)nx);
                } else {
                    xStep = _roundUp((_xtickMax - _xtickMin)
                                     / (double)nx);
                }
                numfracdigits = _numFracDigits(xStep);

                // Compute x starting point so it is a multiple of xStep.
                double xStart = xStep*Math.ceil(_xtickMin/xStep);

                // NOTE: Following disables first tick.  Not a good idea?
                // if (xStart == _xMin) xStart += xStep;

                Vector<Double> xgrid = null;
                double xTmpStart = xStart;
                if (_xlog) {
                    xgrid = _gridInit(xStart, xStep, true, null);
                    //xgrid = _gridInit(xStart, xStep);
                    xTmpStart = _gridRoundUp(xgrid, xStart);
                } else if (_xtime) {
                    xTmpStart = _roundUpDate(_xtickMin, xStep);
                }

                // Set to false if we don't need the exponent
                boolean needExponent = _xlog;

                // Label the x axis.  The labels are quantized so that
                // they don't have excess resolution.
                for (double xpos = xTmpStart;
                     xpos <= _xtickMax; // TODO: ?????
                     xpos = _gridStep(xgrid, xpos, xStep, _xlog)) {
                    String xticklabel;
                    if (_xlog) {
                        xticklabel = _formatLogNum(xpos, numfracdigits);
                        if (xticklabel.indexOf('e') != -1 )
                            needExponent = false;
                    } else if (_xtime) {
                        xticklabel = _formatTime(xpos, xStep);
                        needExponent = false;
                    } else {
                        xticklabel = _formatNum(xpos, numfracdigits);
                    }
                    xCoord1 = _ulx + (int)((xpos-_xtickMin)*_xtickscale);
                    graphics.setColor(_tickColor);
                    graphics.drawLine(xCoord1, _uly+1, xCoord1, yCoord1);
                    graphics.drawLine(xCoord1, _lry-1, xCoord1, yCoord2);
                    if (_grid && xCoord1 != _ulx && xCoord1 != _lrx) {
                        graphics.setColor(_gridColor);
                        graphics.drawLine(xCoord1, yCoord1, xCoord1, yCoord2);
                        graphics.setColor(_foreground);
                    }
                    int labxpos = xCoord1 -
                        _labelFontMetrics.stringWidth(xticklabel)/2;
                    // NOTE: 3 pixel spacing between axis and labels.
                    graphics.setColor(_foreground);
                    graphics.drawString(xticklabel, labxpos,
                            _lry + 3 + labelheight);
                }

                if (_xlog) {
                    // Draw in grid lines that don't have labels.

                    // If the step is greater than 1, clamp it to 1 so that
                    // we draw the unlabeled grid lines for each
                    // integer interval.
                    double tmpStep = (xStep > 1.0)? 1.0 : xStep;

                    // Recalculate the start using the new step.
                    xTmpStart = tmpStep*Math.ceil(_xtickMin/tmpStep);

                    Vector<Double> unlabeledgrid  = _gridInit(xTmpStart, tmpStep,
                            false, xgrid);
                    if (unlabeledgrid.size() > 0 ) {
                        for (double xpos = _gridStep(unlabeledgrid, xTmpStart,
                                tmpStep, _xlog);
                             xpos <= _xtickMax;
                             xpos = _gridStep(unlabeledgrid, xpos,
                                     tmpStep, _xlog)) {
                            xCoord1 = _ulx + (int)((xpos-_xtickMin)*_xtickscale);
                            if (_grid && xCoord1 != _ulx && xCoord1 != _lrx) {
                                graphics.setColor(_gridColor);
                                graphics.drawLine(xCoord1, _uly+1,
                                        xCoord1, _lry-1);
                                graphics.setColor(_foreground);
                            }
                        }
                    }

                    if (needExponent) {
                        _xExp = (int)Math.floor(xTmpStart);
                        graphics.setFont(_superscriptFont);
                        graphics.setColor(_foreground);
                        graphics.drawString(Integer.toString(_xExp), xSPos,
                                ySPos - halflabelheight);
                        xSPos -= _labelFontMetrics.stringWidth("x10");
                        graphics.setFont(_labelFont);
                        graphics.drawString("x10", xSPos, ySPos);
                    } else {
                        _xExp = 0;
                    }
                }


            } else {
                // ticks have been explicitly specified
                Enumeration<Double> nt = _xticks.elements();
                Enumeration<String> nl = _xticklabels.elements();
                // Code contributed by Jun Wu (jwu@inin.com.au)
                double preLength = 0.0;
                while (nl.hasMoreElements()) {
                    String label = (String) nl.nextElement();
                    double xpos = ((Double)(nt.nextElement())).doubleValue();
                    // If xpos is out of range, ignore.
                    if (xpos > _xMax || xpos < _xMin) continue;

                    // Find the center position of the label.
                    xCoord1 = _ulx + (int)((xpos-_xMin)*_xscale);

                    // Find  the start position of x label.
                    int labxpos = xCoord1 - _labelFontMetrics.stringWidth(label)/2;

                    // If the labels are not overlapped, proceed.
                    if (labxpos > preLength) {
                        // calculate the length of the label
                        preLength = xCoord1 + 10
                                + _labelFontMetrics.stringWidth(label) / 2;

                        // Draw the label.
                        // NOTE: 3 pixel spacing between axis and labels.
                        graphics.setColor(_foreground);
                        graphics.drawString(label,
                                            labxpos, _lry + 3 + labelheight);

                        // Draw the label mark on the axis
                        graphics.setColor(_tickColor);
                        graphics.drawLine(xCoord1, _uly+1, xCoord1, yCoord1);
                        graphics.drawLine(xCoord1, _lry-1, xCoord1, yCoord2);

                        // Draw the grid line
                        if (_grid && xCoord1 != _ulx && xCoord1 != _lrx) {
                            graphics.setColor(_gridColor);
                            graphics.drawLine(xCoord1, yCoord1,
                                              xCoord1, yCoord2);
                            graphics.setColor(_foreground);
                        }
                    }
                }
            }
        }

        //////////////////// Draw axis labels now.
        // Center the  X label over the plotting region, not
        // the window.
        graphics.setColor(_foreground);
        graphics.setFont(_labelFont);
        if (_xlabel != null) {
            String xlabel = _xlog ? "log ("+_xlabel+")" : _xlabel;
            int labelx = _ulx +
                (width - _labelFontMetrics.stringWidth(xlabel))/2;
            graphics.drawString(xlabel, labelx, ySPos);
        }

        if (graphics instanceof Graphics2D && _ylabel != null) {
            Graphics2D g2 = (Graphics2D)graphics;
            int x = labelheight;
            String ylabel = _ylog ? "log ("+_ylabel+")" : _ylabel;
            int y = _uly +
                (height + _labelFontMetrics.stringWidth(ylabel)) / 2;
            g2.rotate(Math.toRadians(-90), x, y);
            g2.drawString(ylabel, x, y);
            g2.rotate(Math.toRadians(90), x, y);
        }
        graphics.setFont(previousFont);
    }

    /** Put a mark corresponding to the specified dataset at the
     *  specified x and y position.   The mark is drawn in the
     *  current color.  In this base class, a point is a
     *  filled rectangle 6 pixels across.  Note that marks greater than
     *  about 6 pixels in size will not look very good since they will
     *  overlap axis labels and may not fit well in the legend.   The
     *  <i>clip</i> argument, if <code>true</code>, states
     *  that the point should not be drawn if
     *  it is out of range.
     *
     *  Note that this method is not synchronized, so the caller should be.
     *  Moreover this method should always be called from the event thread
     *  when being used to write to the screen.
     *
     *  @param graphics The graphics context.
     *  @param dataset The index of the data set.
     *  @param xpos The X position.
     *  @param ypos The Y position.
     *  @param clip If true, do not draw if out of range.
     */
    protected Dimension _drawPoint(Graphics graphics,
            int dataset, long xpos, long ypos, boolean clip) {
        // Ignore if there is no graphics object to draw on.
        if (graphics == null) {
            return new Dimension(0,0);
        }
        boolean pointinside = (ypos <= _lry && ypos >= _uly &&
                               xpos <= _lrx && xpos >= _ulx);
        if (!pointinside && clip) {
            return new Dimension(0,0);
        }
        graphics.fillRect((int)xpos-6, (int)ypos-6, 6, 6);
        return new Dimension(6, 6);
    }

    /** Display basic information in its own window.
     */
    protected void _help() {
        String message =
            "Ptolemy plot package\n" +
            "By: Edward A. Lee, eal@eecs.berkeley.edu\n" +
            "and Christopher Hylands, cxh@eecs.berkeley.edu\n" +
            "Version " + PTPLOT_RELEASE +
            "Key bindings:\n" +
            "   Cntr-c:  copy plot to clipboard (EPS format), if permitted\n" +
            "   D: dump plot data to standard out\n" +
            "   E: export plot to standard out (EPS format)\n" +
            "   F: fill plot\n" +
            "   H or ?: print help message (this message)\n" +
            "   Cntr-D or Q: quit\n" +
            "For more information, see\n" +
            "http://ptolemy.eecs.berkeley.edu/java/ptplot\n";
        JOptionPane.showMessageDialog(this, message,
                "Ptolemy Plot Help Window", JOptionPane.INFORMATION_MESSAGE);
    }

    /** Parse a line that gives plotting information.  In this base
     *  class, only lines pertaining to the title and labels are processed.
     *  Everything else is ignored. Return true if the line is recognized.
     *  It is not synchronized, so its caller should be.
     *  @param line A line of text.
     */
    protected boolean _parseLine(String line) {
        // If you modify this method, you should also modify write()

        // We convert the line to lower case so that the command
        // names are case insensitive.
        String lcLine = new String(line.toLowerCase());
        if (lcLine.startsWith("#")) {
            // comment character
            return true;
        } else if (lcLine.startsWith("titletext:")) {
            setTitle((line.substring(10)).trim());
            return true;
        } else if (lcLine.startsWith("title:")) {
            // Tolerate alternative tag.
            setTitle((line.substring(10)).trim());
            return true;
        } else if (lcLine.startsWith("xlabel:")) {
            setXLabel((line.substring(7)).trim());
            return true;
        } else if (lcLine.startsWith("ylabel:")) {
            setYLabel((line.substring(7)).trim());
            return true;
        } else if (lcLine.startsWith("xrange:")) {
            int comma = line.indexOf(",", 7);
            if (comma > 0) {
                String min = (line.substring(7, comma)).trim();
                String max = (line.substring(comma+1)).trim();
                try {
                    Double dmin = new Double(min);
                    Double dmax = new Double(max);
                    setXRange(dmin.doubleValue(), dmax.doubleValue());
                } catch (NumberFormatException e) {
                    // ignore if format is bogus.
                }
            }
            return true;
        } else if (lcLine.startsWith("yrange:")) {
            int comma = line.indexOf(",", 7);
            if (comma > 0) {
                String min = (line.substring(7, comma)).trim();
                String max = (line.substring(comma+1)).trim();
                try {
                    Double dmin = new Double(min);
                    Double dmax = new Double(max);
                    setYRange(dmin.doubleValue(), dmax.doubleValue());
                } catch (NumberFormatException e) {
                    // ignore if format is bogus.
                }
            }
            return true;
        } else if (lcLine.startsWith("xticks:")) {
            // example:
            // XTicks "label" 0, "label" 1, "label" 3
            _parsePairs(line.substring(7), true);
            return true;
        } else if (lcLine.startsWith("yticks:")) {
            // example:
            // YTicks "label" 0, "label" 1, "label" 3
            _parsePairs(line.substring(7), false);
            return true;
        } else if (lcLine.startsWith("xlog:")) {
            if (lcLine.indexOf("off",5) >= 0) {
                _xlog = false;
            } else {
                _xlog = true;
            }
            return true;
        } else if (lcLine.startsWith("ylog:")) {
            if (lcLine.indexOf("off",5) >= 0) {
                _ylog = false;
            } else {
                _ylog = true;
            }
            return true;
        } else if (lcLine.startsWith("grid:")) {
            if (lcLine.indexOf("off",5) >= 0) {
                _grid = false;
            } else {
                _grid = true;
            }
            return true;
        } else if (lcLine.startsWith("wrap:")) {
            if (lcLine.indexOf("off",5) >= 0) {
                _wrap = false;
            } else {
                _wrap = true;
            }
            return true;
        } else if (lcLine.startsWith("color:")) {
            if (lcLine.indexOf("off",6) >= 0) {
                _usecolor = false;
            } else {
                _usecolor = true;
            }
            return true;
        }
        return false;
    }

    /** Set the padding multiple.
     *  The plot rectangle can be "padded" in each direction -x, +x, -y, and
     *  +y.  If the padding is set to 0.05 (and the padding is used), then
     *  there is 10% more length on each axis than set by the setXRange() and
     *  setYRange() methods, 5% in each direction.
     *  @param padding The padding multiple.
     */
    protected void _setPadding(double padding) {
        _padding = padding;
    }

    ///////////////////////////////////////////////////////////////////
    ////                         protected variables               ////

    // The range of the data to be plotted.
    protected transient double _yMax = 0, _yMin = 0, _xMax = 0, _xMin = 0;

    /** The factor we pad by so that we don't plot points on the axes.
     */
    protected double _padding = 0.025;

    // Whether the ranges have been given.
    protected transient boolean _xRangeGiven = false;
    protected transient boolean _yRangeGiven = false;
    protected transient boolean _rangesGivenByZooming = false;

    /** @serial The given X and Y ranges.
     * If they have been given the top and bottom of the x and y ranges.
     * This is different from _xMin and _xMax, which actually represent
     * the range of data that is plotted.  This represents the range
     * specified (which may be different due to zooming).
     */
    protected double _xlowgiven, _xhighgiven, _ylowgiven, _yhighgiven;

    protected Range _xFillRange = null;
    protected Range _yFillRange = null;

    /** @serial The minimum X value registered so for, for auto ranging. */
    protected double _xBottom = Double.MAX_VALUE;

    /** @serial The maximum X value registered so for, for auto ranging. */
    protected double _xTop = - Double.MAX_VALUE;

    /** @serial The minimum Y value registered so for, for auto ranging. */
    protected double _yBottom = Double.MAX_VALUE;

    /** @serial The maximum Y value registered so for, for auto ranging. */
    protected double _yTop = - Double.MAX_VALUE;

    /** @serial Whether to draw the axes using a logarithmic scale. */
    protected boolean _xlog = false, _ylog = false;

    /** @serial Whether to draw the x-axis with a time-of-day scale. */
    protected boolean _xtime = false;

    /** @serial The date formatter for x-axis time labels. */
    protected SimpleDateFormat _dateFormat = null;

    /** @serial Whether log plot of non-positive values produces an error. */
    protected boolean _ylogNoNegVals = true, _xlogNoNegVals = true;

    /** @serial Values to plot for log plot of non-positive values. */
    protected double _ylogNegDefault = 0, _xlogNegDefault = 0;

    // For use in calculating log base 10. A log times this is a log base 10.
    protected static final double _LOG10SCALE = 1/Math.log(10);

    /** @serial Whether to draw a background grid. */
    private boolean _grid = true;

    /** @serial Color of the background grid. */
    private Color _gridColor = Color.lightGray;

    /** @serial Color of the background grid. */
    private Color _tickColor = Color.black;

    /** @serial Whether to wrap the X axis. */
    protected boolean _wrap = false;

    /** @serial The high range of the X axis for wrapping. */
    protected double _wrapHigh;

    /** @serial The low range of the X axis for wrapping. */
    protected double _wrapLow;

    /** @serial Color of the background, settable from HTML. */
    protected Color _background = Color.white;

    /** @serial Color of the foreground, settable from HTML. */
    protected Color _foreground = Color.black;

    /** @serial Color of the plot canvas background. */
    private Color _canvasColor = Color.white;

    /** @serial Top padding.
     *  Derived classes can increment these to make space around the plot.
     */
    protected int _topPadding = 0;

    /** @serial Bottom padding.
     *  Derived classes can increment these to make space around the plot.
     */
    protected int _bottomPadding = 5;

    /** @serial Right padding.
     *  Derived classes can increment these to make space around the plot.
     */
    protected int _rightPadding = 10;

    /** @serial Left padding.
     *  Derived classes can increment these to make space around the plot.
     */
    protected int _leftPadding = 10;

    // The naming convention is: "_ulx" = "upper left x", where "x" is
    // the horizontal dimension.

    /** The x value of the upper left corner of the plot rectangle in pixels. */
    protected int _ulx = 1;

    /** The y value of the upper left corner of the plot rectangle in pixels. */
    protected int _uly = 1;

    /** The x value of the lower right corner of
     * the plot rectangle in pixels. */
    protected int _lrx = 100;

    /** The y value of the lower right corner of
     * the plot rectangle in pixels. */
    protected int _lry = 100;

    /**
     * The width of any buttons to the right of the plot.
     */
    protected int _buttonWidths = 0;

    /** Scaling used for the vertical axis in plotting points.
     *  The units are pixels/unit, where unit is the units of the Y axis.
     */
    protected double _yscale = 1.0;

    /** Scaling used for the horizontal axis in plotting points.
     *  The units are pixels/unit, where unit is the units of the X axis.
     */
    protected double _xscale = 1.0;

    /**
     * Set to true if the user has specified a fixed aspect ratio.
     */
    protected boolean _aspectRatioGiven = false;

    /**
     * The desired aspect ratio of the user unit sizes as displayed
     * on the screen.
     */
    protected double _aspectRatio = 1;

    /** @serial Indicator whether to use _colors. */
    protected boolean _usecolor = true;

    // Default _colors, by data set.
    // There are 11 colors so that combined with the
    // 10 marks of the Plot class, we can distinguish 110
    // distinct data sets.
    protected Color[] _colors = {
        new Color(0xff0000),   // red
        new Color(0x0000ff),   // blue
        new Color(0x00aaaa),   // cyan-ish
        new Color(0x000000),   // black
        new Color(0xffa500),   // orange
        new Color(0x53868b),   // cadetblue4
        new Color(0xff7f50),   // coral
        new Color(0x45ab1f),   // dark green-ish
        new Color(0x90422d),   // sienna-ish
        new Color(0xa0a0a0),   // grey-ish
        new Color(0x14ff14),   // green-ish
    };

    /** @serial Width and height of component in pixels. */
    protected int _width = 500, _height = 300,
        _preferredWidth = 500, _preferredHeight = 300;

    /** @serial Indicator that size has been set. */
    protected boolean _sizeHasBeenSet = false;

    private Image _img = null;
    private Rectangle _imgSize = null;
    protected Rectangle _legendRectangle;


    ///////////////////////////////////////////////////////////////////
    ////                         private methods                   ////

    /*
     * Draw the legend in the upper right corner and return the width
     * (in pixels)  used up.  The arguments give the upper right corner
     * of the region where the legend should be placed.
     */
    private int _drawLegend(Graphics graphics, int urx, int ury) {
        // Ignore if there is no graphics object to draw on.
        if (graphics == null) return 0;

        // FIXME: consolidate all these for efficiency
        Font previousFont = graphics.getFont();
        graphics.setFont(_labelFont);
        int spacing = _labelFontMetrics.getHeight();

        Enumeration<String> v = _legendStrings.elements();
        Enumeration<Integer> i = _legendDatasets.elements();
        int ypos = ury + spacing;
        int maxwidth = 0;
        while (v.hasMoreElements()) {
            String legend = (String) v.nextElement();
            // NOTE: relies on _legendDatasets having the same num. of entries.
            int dataset = ((Integer) i.nextElement()).intValue();
            if (dataset >= 0) {
                if (_usecolor) {
                    // Points are only distinguished up to the number of colors
                    int color = dataset % _colors.length;
                    graphics.setColor(_colors[color].brighter());
                }
                _drawPoint(graphics, dataset, urx-3, ypos-3, false);

                graphics.setColor(_foreground);
                int width = _labelFontMetrics.stringWidth(legend);
                if (width > maxwidth) maxwidth = width;
                graphics.drawString(legend, urx - 15 - width, ypos);
                ypos += spacing;
            }
        }
        graphics.setFont(previousFont);
        return 22 + maxwidth;  // NOTE: subjective spacing parameter.
    }

    // Execute all actions pending on the deferred action list.
    // The list is cleared and the _actionsDeferred variable is set
    // to false, even if one of the deferred actions fails.
    // This method should only be invoked in the event dispatch thread.
    // It is synchronized, so the integrity of the deferred actions list
    // is ensured, since modifications to that list occur only in other
    // synchronized methods.
    protected synchronized void _executeDeferredActions() {
        try {
            Iterator<Runnable> actions = _deferredActions.iterator();
            while (actions.hasNext()) {
                Runnable action = actions.next();
                action.run();
            }
        } finally {
            _actionsDeferred = false;
            _deferredActions.clear();
        }
    }

    /*
     * Return the number as a String for use as a label on a
     * logarithmic axis.
     * Since this is a log plot, number passed in will not have too many
     * digits to cause problems.
     * If the number is an integer, then we print 1e<num>.
     * If the number is not an integer, then print only the fractional
     * components.
     */
    private String _formatLogNum(double num, int numfracdigits) {
        String results;
        int exponent = (int)num;

        // Determine the exponent, prepending 0 or -0 if necessary.
        if (exponent >= 0 && exponent < 10) {
            results = "0" + exponent;
        } else {
            if (exponent < 0 && exponent > -10) {
                results = "-0" + (-exponent);
            } else {
                results = Integer.toString(exponent);
            }
        }

        // Handle the mantissa.
        if (num >= 0.0 ) {
            if (num - (int)(num) < 0.001) {
                results = "1e" + results;
            } else {
                results = _formatNum(Math.pow(10.0, (num - (int)num)),
                        numfracdigits);
            }
        } else {
            if (-num - (int)(-num) < 0.001) {
                results = "1e" + results;
            } else {
                results = _formatNum(Math.pow(10.0, (num - (int)num))*10,
                        numfracdigits);
            }
        }
        return results;
    }

    protected String _formatTime(double dtime, double step_ms) {
        if (_dateFormat == null) {
            _dateFormat = new SimpleDateFormat();
        }
        if (step_ms < 1000) { // Step < 1 s
            _dateFormat.applyPattern("H:mm:ss.S");
        } else if (step_ms < 60 * 1000) { // Step < 1 min
            _dateFormat.applyPattern("H:mm:ss");
        } else if (step_ms < 4 * 60 * 60 * 1000) { // Step < 4 hr
            _dateFormat.applyPattern("H:mm");
        } else if (step_ms < 24 * 60 * 60 * 1000) { // Step < 1 day
            //_dateFormat.applyPattern("yyyy/MM/dd:HH");
            _dateFormat.applyPattern("H:mm");// Same as previous
        } else {                // Step >= 1 day
            _dateFormat.applyPattern("yyyy-MM-dd");
        }
        String str = _dateFormat.format(new Date((long)dtime));
        return str;
    }

    /*
     * Return a string for displaying the specified number
     * using the specified number of digits after the decimal point.
     * NOTE: java.text.NumberFormat in Netscape 4.61 has a bug
     * where it fails to round numbers instead it truncates them.
     * As a result, we don't use java.text.NumberFormat, instead
     * We use the method from Ptplot1.3
     */
    private String _formatNum(double num, int numfracdigits) {
        // When java.text.NumberFormat works under Netscape,
        // uncomment the next block of code and remove
        // the code after it.
        // Ptplot developers at UCB can access a test case at:
        // http://ptolemy.eecs.berkeley.edu/~ptII/ptIItree/ptolemy/plot/adm/trunc/trunc-jdk11.html
        // The plot will show two 0.7 values on the x axis if the bug
        // continues to exist.

        //if (_numberFormat == null) {
        //   // Cache the number format so that we don't have to get
        //    // info about local language etc. from the OS each time.
        //    _numberFormat = NumberFormat.getInstance();
        //}
        //_numberFormat.setMinimumFractionDigits(numfracdigits);
        //_numberFormat.setMaximumFractionDigits(numfracdigits);
        //return _numberFormat.format(num);

        // The section below is from Ptplot1.3

        // First, round the number.
        double fudge = (num < 0.0) ? -0.5 : 0.5;
        fudge *= Math.pow(10.0, -numfracdigits);
        if (Math.abs(num) < Math.abs(fudge)) {
            return "0";
        }
        String numString = Double.toString(num + fudge);
        // Next, find the decimal point.
        int dpt = numString.lastIndexOf(".");
        StringBuffer result = new StringBuffer();
        if (dpt < 0) {
            // The number we are given is an integer.
            if (numfracdigits <= 0) {
                // The desired result is an integer.
                result.append(numString);
                return result.toString();
            }
            // Append a decimal point and some zeros.
            result.append(".");
            for (int i = 0; i < numfracdigits; i++) {
                result.append("0");
            }
            return result.toString();
        } else {
            // There are two cases.  First, there may be enough digits.
            int shortby = numfracdigits - (numString.length() - dpt -1);
            if (shortby <= 0) {
                int numtocopy = dpt + numfracdigits + 1;
                if (numfracdigits == 0) {
                    // Avoid copying over a trailing decimal point.
                    numtocopy -= 1;
                }
                result.append(numString.substring(0, numtocopy));
                return result.toString();
            } else {
                result.append(numString);
                for (int i = 0; i < shortby; i++) {
                    result.append("0");
                }
                return result.toString();
            }
        }
    }

    /*
     * Determine what values to use for log axes.
     * Based on initGrid() from xgraph.c by David Harrison.
     */
    private Vector<Double> _gridInit(double low, double step, boolean labeled,
            Vector<Double> oldgrid) {

        // How log axes work:
        // _gridInit() creates a vector with the values to use for the
        // log axes.  For example, the vector might contain
        // {0.0 0.301 0.698}, which could correspond to
        // axis labels {1 1.2 1.5 10 12 15 100 120 150}
        //
        // _gridStep() gets the proper value.  _gridInit is cycled through
        // for each integer log value.
        //
        // Bugs in log axes:
        // * Sometimes not enough grid lines are displayed because the
        // region is small.  This bug is present in the original xgraph
        // binary, which is the basis of this code.  The problem is that
        // as ratio gets closer to 1.0, we need to add more and more
        // grid marks.

        Vector<Double> grid = new Vector<Double>(10);
        //grid.addElement(new Double(0.0));
        double ratio = Math.pow(10.0, step);
        int ngrid = 1;
        if (labeled) {
            // Set up the number of grid lines that will be labeled
            if (ratio <= 3.5) {
                if (ratio > 2.0)
                    ngrid = 2;
                else if (ratio > 1.26)
                    ngrid = 5;
                else if (ratio > 1.125)
                    ngrid = 10;
                else
                    ngrid = (int)Math.rint(1.0/step);

            }
        } else {
            // Set up the number of grid lines that will not be labeled
            if (ratio > 10.0)
                ngrid = 1;
            else if (ratio > 3.0)
                ngrid = 2;
            else if (ratio > 2.0)
                ngrid = 5;
            else if (ratio > 1.125)
                ngrid = 10;
            else
                ngrid = 100;
            // Note: we should keep going here, but this increases the
            // size of the grid array and slows everything down.
        }

        int oldgridi = 0;
        for (int i = 0; i < ngrid; i++) {
            double gridval = i * 1.0/ngrid * 10;
            double logval = _LOG10SCALE*Math.log(gridval);
            if (logval == Double.NEGATIVE_INFINITY)
                logval = 0.0;

            // If oldgrid is not null, then do not draw lines that
            // were already drawn in oldgrid.  This is necessary
            // so we avoid obliterating the tick marks on the plot borders.
            if (oldgrid != null && oldgridi < oldgrid.size() ) {

                // Cycle through the oldgrid until we find an element
                // that is equal to or greater than the element we are
                // trying to add.
                while (oldgridi < oldgrid.size() &&
                        oldgrid.elementAt(oldgridi).doubleValue() <
                        logval) {
                    oldgridi++;
                }

                if (oldgridi < oldgrid.size()) {
                    // Using == on doubles is bad if the numbers are close,
                    // but not exactly equal.
                    if (Math.abs(
                            oldgrid.elementAt(oldgridi).doubleValue()
                            - logval)
                            > 0.00001) {
                        grid.addElement(new Double(logval));
                    }
                } else {
                   grid.addElement(new Double(logval));
                }
            } else {
                grid.addElement(new Double(logval));
            }
        }

        // _gridCurJuke and _gridBase are used in _gridStep();
        _gridCurJuke = 0;
        if (low == -0.0)
            low = 0.0;
        _gridBase = Math.floor(low);
        double x = low - _gridBase;

        // Set gridCurJuke so that the value in grid is greater than
        // or equal to x.  This sets us up to process the first point.
        for (_gridCurJuke = -1;
             (_gridCurJuke+1) < grid.size() && x >=
                 grid.elementAt(_gridCurJuke+1).doubleValue();
             _gridCurJuke++){
        }
        return grid;
    }

    /*
     * Round pos up to the nearest value in the grid.
     */
    private double _gridRoundUp(Vector<Double> grid, double pos) {
        if (grid == null) {
            return pos;
        }
        double x = pos - Math.floor(pos);
        int i;
        for(i = 0; i < grid.size() &&
                x >= grid.elementAt(i).doubleValue();
            i++){}
        if (i >= grid.size())
            return pos;
        else
            return Math.floor(pos) + grid.elementAt(i).doubleValue();
    }

    /*
     * Used to find the next value for the axis label.
     * For non-log axes, we just return pos + step.
     * For log axes, we read the appropriate value in the grid Vector,
     * add it to _gridBase and return the sum.  We also take care
     * to reset _gridCurJuke if necessary.
     * Note that for log axes, _gridInit() must be called before
     * calling _gridStep().
     * Based on stepGrid() from xgraph.c by David Harrison.
     */
    private double _gridStep(Vector<Double> grid, double pos, double step,
            boolean logflag) {
        if (logflag) {
            if (++_gridCurJuke >= grid.size()) {
                _gridCurJuke = 0;
                _gridBase += Math.ceil(step);
            }
            if (_gridCurJuke >= grid.size())
                return pos + step;
            return _gridBase +
                grid.elementAt(_gridCurJuke).doubleValue();
        } else {
            return pos + step;
        }
    }

    /*
     * Measure the various fonts.  You only want to call this once.
     */
    private void _measureFonts() {
        // We only measure the fonts once, and we do it from addNotify().
        if (_labelFont == null)
            _labelFont = new Font("Helvetica", Font.PLAIN, 12);
        if (_superscriptFont == null)
            _superscriptFont = new Font("Helvetica", Font.PLAIN, 11);
        if (_titleFont == null)
            _titleFont = new Font("Helvetica", Font.BOLD, 14);

        _labelFontMetrics = getFontMetrics(_labelFont);
        _superscriptFontMetrics = getFontMetrics(_superscriptFont);
        _titleFontMetrics = getFontMetrics(_titleFont);
    }

    /*
     * Return the number of fractional digits required to display the
     * given number.  No number larger than 15 is returned (if
     * more than 15 digits are required, 15 is returned).
     */
    private int _numFracDigits(double num) {
        int numdigits = 0;
        while (numdigits <= 15 && num != Math.floor(num)) {
            num *= 10.0;
            numdigits += 1;
        }
        return numdigits;
    }

    /*
     * Return the number of integer digits required to display the
     * given number.  No number larger than 15 is returned (if
     * more than 15 digits are required, 15 is returned).
     */
    private int _numIntDigits(double num) {
        int numdigits = 0;
        while (numdigits <= 15 && (int)num != 0.0) {
            num /= 10.0;
            numdigits += 1;
        }
        return numdigits;
    }

    /*
     * Parse a string of the form: "word num, word num, word num, ..."
     * where the word must be enclosed in quotes if it contains spaces,
     * and the number is interpreted as a floating point number.  Ignore
     * any incorrectly formatted fields.  I <i>xtick</i> is true, then
     * interpret the parsed string to specify the tick labels on the x axis.
     * Otherwise, do the y axis.
     */
    private void _parsePairs(String line, boolean xtick) {
        // Clear current ticks first.
        if (xtick) {
            _xticks = null;
            _xticklabels = null;
        } else {
            _yticks = null;
            _yticklabels = null;
        }

        int start = 0;
        boolean cont = true;
        while (cont) {
            int comma = line.indexOf(",", start);
            String pair = null ;
            if (comma > start) {
                pair = (line.substring(start, comma)).trim();
            } else {
                pair = (line.substring(start)).trim();
                cont = false;
            }
            int close = -1;
            int open = 0;
            if (pair.startsWith("\"")) {
                close = pair.indexOf("\"",1);
                open = 1;
            } else {
                close = pair.indexOf(" ");
            }
            if (close > 0) {
                String label = pair.substring(open, close);
                String index = (pair.substring(close+1)).trim();
                try {
                    double idx = (Double.valueOf(index)).doubleValue();
                    if (xtick) addXTick(label, idx);
                    else addYTick(label, idx);
                } catch (NumberFormatException e) {
                    System.err.println("Warning from PlotBox: " +
                            "Unable to parse ticks: " + e.getMessage());
                    // ignore if format is bogus.
                }
            }
            start = comma + 1;
            comma = line.indexOf(",",start);
        }
    }

    /** Return a default set of rendering hints for image export, which
     *  specifies the use of anti-aliasing.
     */
    private RenderingHints _defaultImageRenderingHints() {
        if (_defaultImageRenderingHints == null) {
            _defaultImageRenderingHints = new RenderingHints(null);
            _defaultImageRenderingHints.put(RenderingHints.KEY_ANTIALIASING,
                                            RenderingHints.VALUE_ANTIALIAS_ON);
        }
        return _defaultImageRenderingHints;
    }

    /** Return a default set of rendering hints for display, which
     *  specifies the use of anti-aliasing.
     */
    protected RenderingHints _defaultPlotRenderingHints() {
        if (_defaultPlotRenderingHints == null) {
            _defaultPlotRenderingHints = new RenderingHints(null);
            _defaultPlotRenderingHints.put(RenderingHints.KEY_ANTIALIASING,
                                           RenderingHints.VALUE_ANTIALIAS_ON);
        }
        return _defaultPlotRenderingHints;
    }

    /*
     * Given the time interval in milliseconds, round up to the
     * nearest "nice" unit of time.
     * <p>
     * Note: Don't deal with units of time larger than a day.
     * Rounding to even months or years requires a different approach,
     * depending on the Gregorian Calendar.
     * <p>
     * Note: The argument must be strictly positive.
     */
    private double _roundUpDate(double val) {
        double rtn;
        if (val <= 1000) {     // <= 1 s
            // Round in milliseconds
            rtn = _roundUp(val);
        } else if (val <= 60000) { // <= 1 min
            // Round in seconds
            double[] cut = {60, 30, 20, 10, 5, 2, 1};
            rtn = _roundUp(val / 1000, cut) * 1000;
        } else if (val <= 3600000) { // <= 1 hr
            // Round in minutes
            double[] cut = {60, 30, 20, 10, 5, 2, 1};
            rtn = _roundUp(val / 60000, cut) * 60000;
        } else if (val <= 86400000) {     // <= 1 day
            // Round in hours
            double[] cut = {24, 12, 6, 3, 2, 1};
            rtn = _roundUp(val / 3600000, cut) * 3600000;
        } else {
            // Round in days
            rtn = _roundUp(val / 86400000) * 86400000;
        }
        return rtn;
    }

    /**
     * Given a date in ms, and a stepsize in ms, round to the
     * next even stepsize.  The wrinkle is that the date is in UTC,
     * and we want the steps to be in local time.  So we have to
     * allow for the time zone offset.
     */
    private double _roundUpDate(double val, double step) {
        if (step < 1) {
            return val;
        }
        double rtn;
        if (_dateFormat == null) {
            _dateFormat = new SimpleDateFormat();
        }
        double timeZoneOffset
                = _dateFormat.getTimeZone().getOffset((long)val);
        double valUT = val + timeZoneOffset;
        rtn = step * (1 + ((long)valUT - 1) / (long)step);
        rtn -= timeZoneOffset;
        return rtn;
    }

    /*
     * Given a number, round up to the nearest power of ten
     * times 1, 2, or 5.
     * <p>
     * Note: The argument must be strictly positive.
     * @param val The value to round up.
     */
    private double _roundUp(double val) {
        double[] cut = {10, 5, 2, 1};
        return _roundUp(val, cut);
    }

    /*
     * Given a number, round up to the nearest power of ten
     * times a good number.
     * <p>
     * Note: The argument must be strictly positive.
     * @param val The value to round up.
     * @param cut An array of "good" numbers. Must be monotonically
     * decreasing with "1" as the last number. E.g., {10, 5, 2, 1}.
     */
    private double _roundUp(double val, double[] cut) {
        double logBase = 1 / Math.log(cut[0]);
        int exponent = (int)Math.floor(Math.log(val) * logBase);
        val *= Math.pow(cut[0], -exponent);
        int i;
        int length = cut.length;
        for (i = 1; i < length && val <= cut[i]; i++) {} // Find our slot
        val = cut[i - 1];
        val *= Math.pow(cut[0], exponent);
        return val;
    }

    /*
     * Internal implementation of setXRange, so that it can be called when
     * autoranging.
     */
    private void _setXRange(double min, double max) {
        // If values are invalid, try for something reasonable.
        if (min > max) {
            min = -1.0;
            max = 1.0;
        } else if (min == max) {
            min -= 1.0;
            max += 1.0;
        }

        //if (_xRangeGiven) {
        // The user specified the range, so don't pad.
        //    _xMin = min;
        //    _xMax = max;
        //} else {
        // Pad slightly so that we don't plot points on the axes.
        _xMin = min - ((max - min) * _padding);
        _xMax = max + ((max - min) * _padding);
        //}

        // Find the exponent.
        double largest = Math.max(Math.abs(_xMin), Math.abs(_xMax));
        _xExp = (int) Math.floor(Math.log(largest)*_LOG10SCALE);
        // Use the exponent only if it's large.
        if (Math.abs(_xExp) > 2 && !_xtime) {
            double xs = 1.0/Math.pow(10.0, (double)_xExp);
            _xtickMin = _xMin*xs;
            _xtickMax = _xMax*xs;
        } else {
            _xtickMin = _xMin;
            _xtickMax = _xMax;
            _xExp = 0;
        }
    }

    /*
     * Internal implementation of setYRange, so that it can be called when
     * autoranging.
     */
    private void _setYRange(double min, double max) {
        // If values are invalid, try for something reasonable.
        if (min > max) {
            min = -1.0;
            max = 1.0;
        } else if (min == max) {
            min -= 0.1;
            max += 0.1;
        }
        //if (_yRangeGiven) {
        // The user specified the range, so don't pad.
        //    _yMin = min;
        //    _yMax = max;
        //} else {
        // Pad slightly so that we don't plot points on the axes.
        _yMin = min - ((max - min) * _padding);
        _yMax = max + ((max - min) * _padding);
        //}

        // Find the exponent.
        double largest = Math.max(Math.abs(_yMin), Math.abs(_yMax));
        _yExp = (int) Math.floor(Math.log(largest)*_LOG10SCALE);
        // Use the exponent only if it's larger than 1 in magnitude.
        if (_yExp > 2 || _yExp < -2) {
            double ys = 1.0/Math.pow(10.0, (double)_yExp);
            _ytickMin = _yMin*ys;
            _ytickMax = _yMax*ys;
        } else {
            _ytickMin = _yMin;
            _ytickMax = _yMax;
            _yExp = 0;
        }
    }

    /*
     *  Zoom in or out based on the box that has been drawn.
     *  The argument gives the lower right corner of the box.
     *  This method is not synchronized because it is called within
     *  the UI thread, and making it synchronized causes a deadlock.
     *  @param x The final x position.
     *  @param y The final y position.
     */
    void _zoom(int x, int y) {
        // NOTE: Due to a bug in JDK 1.1.7B, the BUTTON1_MASK does
        // not work on mouse drags, thus we have to use this variable
        // to determine whether we are actually zooming. It is used only
        // in _zoomBox, since calling this method is properly masked.
        _zooming = false;

        Graphics graphics = getGraphics();
        // Ignore if there is no graphics object to draw on.
        if (graphics == null) return;

        if ((_zoomin == true) && (_drawn == true)){
            if (_zoomxn != -1 || _zoomyn != -1) {
                // erase previous rectangle.
                int minx = Math.min(_zoomx, _zoomxn);
                int maxx = Math.max(_zoomx, _zoomxn);
                int miny = Math.min(_zoomy, _zoomyn);
                int maxy = Math.max(_zoomy, _zoomyn);
                graphics.setXORMode(_boxColor);
                graphics.drawRect(minx, miny, maxx - minx, maxy - miny);
                graphics.setPaintMode();
                // constrain to be in range
                if (y > _lry) y = _lry;
                if (y < _uly) y = _uly;
                if (x > _lrx) x = _lrx;
                if (x < _ulx) x = _ulx;
                // NOTE: ignore if total drag less than 5 pixels.
                if ((Math.abs(_zoomx-x) > 5) && (Math.abs(_zoomy-y) > 5)) {
                    double a = _xMin + (_zoomx - _ulx)/_xscale;
                    double b = _xMin + (x - _ulx)/_xscale;
                    // NOTE: It used to be that it was problematic to set
                    // the X range here because it conflicted with the wrap
                    // mechanism.  But now the wrap mechanism saves the state
                    // of the X range when the setWrap() method is called,
                    // so this is safe.
                    // EAL 6/12/00.
                    if (a < b) setXRange(a, b);
                    else setXRange(b, a);
                    a = _yMax - (_zoomy - _uly)/_yscale;
                    b = _yMax - (y - _uly)/_yscale;
                    if (a < b) setYRange(a, b);
                    else setYRange(b, a);
                    zoomin();
                }
                repaint();
            }
        } else if ((_zoomout == true) && (_drawn == true)){
            // Erase previous rectangle.
            graphics.setXORMode(_boxColor);
            int x_diff = Math.abs(_zoomx-_zoomxn);
            int y_diff = Math.abs(_zoomy-_zoomyn);
            graphics.drawRect(_zoomx-15-x_diff, _zoomy-15-y_diff,
                    30+x_diff*2, 30+y_diff*2);
            graphics.setPaintMode();

            // Calculate zoom factor.
            double a = (double)(Math.abs(_zoomx - x)) / 30.0;
            double b = (double)(Math.abs(_zoomy - y)) / 30.0;
            double newx1 = _xMax + (_xMax - _xMin) * a;
            double newx2 = _xMin - (_xMax - _xMin) * a;
            // NOTE: To limit zooming out to the fill area, uncomment this...
            // if (newx1 > _xTop) newx1 = _xTop;
            // if (newx2 < _xBottom) newx2 = _xBottom;
            double newy1 = _yMax + (_yMax - _yMin) * b;
            double newy2 = _yMin - (_yMax - _yMin) * b;
            // NOTE: To limit zooming out to the fill area, uncomment this...
            // if (newy1 > _yTop) newy1 = _yTop;
            // if (newy2 < _yBottom) newy2 = _yBottom;
            zoom(newx2, newy2, newx1, newy1);
        } else if (_drawn == false){
            repaint();
        }
        _drawn = false;
        _zoomin = _zoomout = false;
        _zoomxn = _zoomyn = _zoomx = _zoomy = -1;
    }

    /*
     *  Draw a box for an interactive zoom box.  The starting point (the
     *  upper left corner of the box) is taken
     *  to be that specified by the startZoom() method.  The argument gives
     *  the lower right corner of the box.  If a previous box
     *  has been drawn, erase it first.
     *  This method is not synchronized because it is called within
     *  the UI thread, and making it synchronized causes a deadlock.
     *  @param x The x position.
     *  @param y The y position.
     */
    void _zoomBox(int x, int y) {
        // NOTE: Due to a bug in JDK 1.1.7B, the BUTTON1_MASK does
        // not work on mouse drags, thus we have to use this variable
        // to determine whether we are actually zooming.
        if (!_zooming) return;

        Graphics graphics = getGraphics();
        // Ignore if there is no graphics object to draw on.
        if (graphics == null) return;

        // Bound the rectangle so it doesn't go outside the box.
        if (y > _lry) y = _lry;
        if (y < _uly) y = _uly;
        if (x > _lrx) x = _lrx;
        if (x < _ulx) x = _ulx;
        // erase previous rectangle, if there was one.
        if ((_zoomx != -1 || _zoomy != -1)) {
            // Ability to zoom out added by William Wu.
            // If we are not already zooming, figure out whether we
            // are zooming in or out.
            if (_zoomin == false && _zoomout == false){
                if (y < _zoomy) {
                    _zoomout = true;
                    // Draw reference box.
                    graphics.setXORMode(_boxColor);
                    graphics.drawRect(_zoomx-15, _zoomy-15, 30, 30);
                } else if (y > _zoomy) {
                    _zoomin = true;
                }
            }

            if (_zoomin == true){
                // Erase the previous box if necessary.
                if ((_zoomxn != -1 || _zoomyn != -1) && (_drawn == true)) {
                    int minx = Math.min(_zoomx, _zoomxn);
                    int maxx = Math.max(_zoomx, _zoomxn);
                    int miny = Math.min(_zoomy, _zoomyn);
                    int maxy = Math.max(_zoomy, _zoomyn);
                    graphics.setXORMode(_boxColor);
                    graphics.drawRect(minx, miny, maxx - minx, maxy - miny);
                }
                // Draw a new box if necessary.
                if (y > _zoomy) {
                    _zoomxn = x;
                    _zoomyn = y;
                    int minx = Math.min(_zoomx, _zoomxn);
                    int maxx = Math.max(_zoomx, _zoomxn);
                    int miny = Math.min(_zoomy, _zoomyn);
                    int maxy = Math.max(_zoomy, _zoomyn);
                    graphics.setXORMode(_boxColor);
                    graphics.drawRect(minx, miny, maxx - minx, maxy - miny);
                    _drawn = true;
                    return;
                } else _drawn = false;
            } else if (_zoomout == true){
                // Erase previous box if necessary.
                if ((_zoomxn != -1 || _zoomyn != -1) && (_drawn == true)) {
                    int x_diff = Math.abs(_zoomx-_zoomxn);
                    int y_diff = Math.abs(_zoomy-_zoomyn);
                    graphics.setXORMode(_boxColor);
                    graphics.drawRect(_zoomx-15-x_diff, _zoomy-15-y_diff,
                            30+x_diff*2, 30+y_diff*2);
                }
                if (y < _zoomy){
                    _zoomxn = x;
                    _zoomyn = y;
                    int x_diff = Math.abs(_zoomx-_zoomxn);
                    int y_diff = Math.abs(_zoomy-_zoomyn);
                    graphics.setXORMode(_boxColor);
                    graphics.drawRect(_zoomx-15-x_diff, _zoomy-15-y_diff,
                            30+x_diff*2, 30+y_diff*2);
                    _drawn = true;
                    return;
                } else _drawn = false;
            }
        }
        graphics.setPaintMode();
    }

    /*
     *  Set the starting point for an interactive zoom box (the upper left
     *  corner).
     *  This method is not synchronized because it is called within
     *  the UI thread, and making it synchronized causes a deadlock.
     *  @param x The x position.
     *  @param y The y position.
     */
    void _zoomStart(int x, int y) {
        // constrain to be in range
        if (y > _lry) y = _lry;
        if (y < _uly) y = _uly;
        if (x > _lrx) x = _lrx;
        if (x < _ulx) x = _ulx;
        _zoomx = x;
        _zoomy = y;
        _zooming = true;
    }

    ///////////////////////////////////////////////////////////////////
    ////                         private variables                 ////

    /** @serial Indicator of whether actions are deferred. */
    private boolean _actionsDeferred = false;

    /** @serial List of deferred actions. */
    private List<Runnable> _deferredActions;

    // Call setXORMode with a hardwired color because
    // _background does not work in an application,
    // and _foreground does not work in an applet.
    // NOTE: For some reason, this comes out blue, which is fine...
    private static final Color _boxColor = Color.orange;

    /** @serial The range of the plot as labeled
     * (multiply by 10^exp for actual range.
     */
    private double _ytickMax = 0.0, _ytickMin = 0.0,
        _xtickMax = 0.0 , _xtickMin = 0.0 ;
    /** @serial The power of ten by which the range numbers should
     *  be multiplied.
     */
    private int _yExp = 0, _xExp = 0;

    /** @serial Scaling used in making tick marks. */
    private double _ytickscale = 0.0, _xtickscale = 0.0;

    /** @serial Font information. */
    private Font _labelFont = null, _superscriptFont = null,
        _titleFont = null;
    /** @serial FontMetric information. */
    private FontMetrics _labelFontMetrics = null,
        _superscriptFontMetrics = null,
        _titleFontMetrics = null;

    // Number format cache used by _formatNum.
    // See the comment in _formatNum for more information.
    // private transient NumberFormat _numberFormat = null;

    // Used for log axes. Index into vector of axis labels.
    private transient int _gridCurJuke = 0;

    // Used for log axes.  Base of the grid.
    private transient double _gridBase = 0.0;

    // An array of strings for reporting errors.
    private transient String _errorMsg[];

    /** @serial The title and label strings. */
    private String _xlabel, _ylabel, _title;

    private Color _titleColor = Color.black;

    /** @serial Legend information. */
    private Vector<String> _legendStrings = new Vector<String>();
    private Vector<Integer> _legendDatasets = new Vector<Integer>();

    /** @serial Suppress dataset plotting. */
    private ArrayList<Boolean> _datasetPlotFlags = new ArrayList<Boolean>();

    /** @serial If XTicks or YTicks are given/ */
    private Vector<Double> _xticks = null;
    private Vector<String> _xticklabels = null;
    private Vector<Double> _yticks = null;
    private Vector<String> _yticklabels = null;

    /** @serial If the X-Axis is drawn */
    private boolean _showXAxis = false;

    /** @serial If the Y-Axis is drawn */
    private boolean _showYAxis = false;

    /** @serial True if we want a "walking window". */
    private boolean _walkingWindow = false;

    /** @serial Default width of the "walking window". */
    private double _walkingWidth = 0;

    // A list of the legend buttons
    private ArrayList<AbstractButton> _legendButtonList = new ArrayList<AbstractButton>();

    // A button for filling the plot
    protected transient JButton _fillButton = null;

    // A button for showing the tail-end of the plot
    protected transient JButton _tailButton = null;

    // A button for formatting the plot
    protected transient JButton _formatButton = null;

    // Indicator of whether X and Y range has been first specified.
    boolean _originalXRangeGiven = false, _originalYRangeGiven = false;

    // First values specified to setXRange() and setYRange().
    double _originalXlow = 0.0, _originalXhigh = 0.0,
        _originalYlow = 0.0, _originalYhigh = 0.0;

    // A button for printing the plot
    protected transient JButton _printButton = null;

    // A button for filling the plot
    protected transient JButton _resetButton = null;

    private ButtonListener _buttonListener;
    private LegendListener _legendListener;

    private RenderingHints _defaultPlotRenderingHints = null;
    private RenderingHints _defaultImageRenderingHints = null;

    // Variables keeping track of the interactive zoom box.
    // Initialize to impossible values.
    private transient int _zoomx = -1;
    private transient int _zoomy = -1;
    private transient int _zoomxn = -1;
    private transient int _zoomyn = -1;

    // Control whether we are zooming in or out.
    private transient boolean _zoomin = false;
    private transient boolean _zoomout = false;
    private transient boolean _drawn = false;
    private transient boolean _zooming = false;

    class ButtonListener implements ActionListener {
        public void actionPerformed(ActionEvent event) {
            if (event.getSource() == _fillButton) {
                fillButtonClick();
            } else if (event.getSource() == _tailButton) {
            	tailButtonClick();
            } else if (event.getSource() == _printButton) {
            	printButtonClick();
            } else if (event.getSource() == _resetButton) {
                resetAxes();
            } else if (event.getSource() == _formatButton) {
            	formatButtonClick();
            }
        }
    }


    class LegendListener implements ActionListener {
        public void actionPerformed(ActionEvent event) {
            AbstractButton button = (AbstractButton) event.getSource();
            Set<Integer> datasetList;
            datasetList = (Set<Integer>)button.getClientProperty("datasets");
            for (int dataset : datasetList) {
                setShown(dataset, button.isSelected());
                setScalingValid(false);
                calcMinMax();
            }
        }
    }


    public class ZoomListener implements MouseListener {
        public void mouseClicked(MouseEvent event) {
            requestFocus();
        }
        public void mouseEntered(MouseEvent event) {
        }
        public void mouseExited(MouseEvent event) {
        }
        public void mousePressed(MouseEvent event) {
            // http://developer.java.sun.com/developer/bugParade/bugs/4072703.html
            // BUTTON1_MASK still not set for MOUSE_PRESSED events
            // suggests:
            // Workaround
            //   Assume that a press event with no modifiers must be button 1.
            //   This has the serious drawback that it is impossible to be sure
            //   that button 1 hasn't been pressed along with one of the other
            //   buttons.
            // This problem affects Netscape 4.61 under Digital Unix and
            // 4.51 under Solaris
            if ((event.getModifiers() & MouseEvent.BUTTON1_MASK) != 0
                || event.getModifiers() == 0)
            {
                PlotBox.this._zoomStart(event.getX(), event.getY());
            }
        }
        public void mouseReleased(MouseEvent event) {
            if ((event.getModifiers() & MouseEvent.BUTTON1_MASK) != 0 ||
                    event.getModifiers() == 0) {
                PlotBox.this._zoom(event.getX(), event.getY());
            }
        }
    }


    public class DragListener implements MouseMotionListener {
        public void mouseDragged(MouseEvent event) {
            // NOTE: Due to a bug in JDK 1.1.7B, the BUTTON1_MASK does
            // not work on mouse drags.  It does work on MouseListener
            // methods, so those methods set a variable _zooming that
            // is used by _zoomBox to determine whether to draw a box.
            // if ((event.getModifiers() & event.BUTTON1_MASK)!= 0) {
            PlotBox.this._zoomBox(event.getX(), event.getY());
            // }
        }
        public void mouseMoved(MouseEvent event) {
        }
    }


    public class PlotLayout implements LayoutManager {

        private final static int PAD = 1;
        private final static int PAD2 = 2 * PAD;
        private final static int PAD3 = 3 * PAD;

        public void addLayoutComponent(String name, Component comp) {}
        public void removeLayoutComponent(Component comp) {}
        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        }
        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        }

        /**
         * Do the layout
         * @param target component to be laid out
         */
        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension targetSize = target.getSize();
                Insets insets = target.getInsets();

                int width = targetSize.width - insets.left - insets.right;
                int x1 = insets.left;
                int x2 = x1 + width;
                Component[] comps = getComponents();
                int count = comps.length;

                // Make separate lists of pushButtons and toggleButtons
                int nButtons = 0;
                int nToggles = 0;
                for (int i = 0; i < count; i++) {
                    if (comps[i] != null) {
                        if (comps[i] instanceof JButton) {
                            nButtons++;
                        } else if (comps[i] instanceof JToggleButton) {
                            nToggles++;
                        }
                    }
                }
                JButton[] buttons = new JButton[nButtons];
                JToggleButton[] toggles = new JToggleButton[nToggles];
                int iButton = 0;
                int iToggle = 0;
                for (int i = 0; i < count; i++) {
                    if (comps[i] != null) {
                        if (comps[i] instanceof JButton) {
                            buttons[iButton++] = (JButton)comps[i];
                        } else if (comps[i] instanceof JToggleButton) {
                            toggles[iToggle++] = (JToggleButton)comps[i];
                        }
                    }
                }

                // Do the regular buttons
                SizeRequirements[] xCompRq = new SizeRequirements[nButtons];
                SizeRequirements[] yCompRq = new SizeRequirements[nButtons];
                for (int i = 0; i < nButtons; i++) {
                    JComponent comp = buttons[i];
                    Dimension min = comp.getMinimumSize();
                    Dimension max = comp.getMaximumSize();
                    Dimension pref = comp.getPreferredSize();
                    xCompRq[i] = new SizeRequirements(min.width + PAD3,
                                                       pref.width + PAD3,
                                                       max.width + PAD3,
                                                       0.0f);
                    yCompRq[i] = new SizeRequirements(min.height + PAD,
                                                       pref.height + PAD,
                                                       max.height + PAD,
                                                       0.5f);
                }

                int[] xoff = new int[nButtons];
                int[] xspan = new int[nButtons];
                SizeRequirements xRq
                        = SizeRequirements.getAlignedSizeRequirements(xCompRq);
                int[] yoff = new int[nButtons];
                int[] yspan = new int[nButtons];
                SizeRequirements yRq
                        = SizeRequirements.getTiledSizeRequirements(yCompRq);

                SizeRequirements.calculateAlignedPositions
                        (xRq.preferred, xRq, xCompRq, xoff, xspan);
                SizeRequirements.calculateTiledPositions
                        (yRq.preferred, yRq, yCompRq, yoff, yspan, false);

                int x = x2 - xRq.preferred + PAD2;
                int y = _lry - yRq.preferred + PAD;
                for (int i = 0; i < nButtons; i++) {
                    buttons[i].setBounds(x + xoff[i], y + yoff[i],
                                         xspan[i] - PAD3, yspan[i] - PAD);
                }
                _buttonWidths = xRq.preferred;
                _legendRectangle = new Rectangle(x,
                                                 _uly,
                                                 targetSize.width - x - PAD,
                                                 y - _uly - 5);

                // Do toggleButtons
                if (nToggles > 0) {
                    xCompRq = new SizeRequirements[nToggles];
                    yCompRq = new SizeRequirements[nToggles];
                    for (int i = 0; i < nToggles; i++) {
                        JComponent comp = toggles[i];
                        Dimension min = comp.getMinimumSize();
                        Dimension max = comp.getMaximumSize();
                        Dimension pref = comp.getPreferredSize();
                        xCompRq[i] = new SizeRequirements(min.width + PAD3,
                                                           pref.width + PAD3,
                                                           max.width + PAD3,
                                                           0.0f);
                        yCompRq[i] = new SizeRequirements(min.height + PAD,
                                                           pref.height + PAD,
                                                           max.height + PAD,
                                                           0.5f);
                    }

                    xoff = new int[nToggles];
                    xspan = new int[nToggles];
                    xRq = SizeRequirements.getAlignedSizeRequirements(xCompRq);
                    yoff = new int[nToggles];
                    yspan = new int[nToggles];
                    yRq = SizeRequirements.getTiledSizeRequirements(yCompRq);

                    SizeRequirements.calculateAlignedPositions
                            (xRq.preferred, xRq, xCompRq, xoff, xspan);
                    SizeRequirements.calculateTiledPositions
                            (yRq.preferred, yRq, yCompRq, yoff, yspan, false);

                    x = x2 - xRq.preferred + PAD2;
                    y = _uly + PAD;
                    int j = nToggles - 1;
                    for (int i = 0; i < nToggles; i++, j--) {
                        // It's "convenient" to invert the order of the toggles!
                        toggles[i].setBounds(x + xoff[j], y + yoff[j],
                                             xspan[j] - PAD3, yspan[j] - PAD);
                    }
                    _buttonWidths = Math.max(_buttonWidths, xRq.preferred);
                    int rwidth = Math.max(_legendRectangle.width,
                                          targetSize.width - x - PAD);
                    _legendRectangle = new Rectangle(x, _uly,
                                                     rwidth, y - _uly - 5);
                }
            }
        }
    }

    class Range {
        public double min;
        public double max;

        public Range(double min, double max) {
            this.min = min;
            this.max = max;
        }
    }

    class LegendIcon implements Icon {
        private Dimension dimension;
        private int dataset;

        public LegendIcon(int dataset) {
            //dimension = _drawPoint(null, dataset, 0, 0, false);
            dimension = new Dimension(12, 6);
            this.dataset = dataset;
        }

        public void paintIcon(Component c, Graphics g, int x, int y) {
            if (!(c instanceof JToggleButton)
                || !((JToggleButton)c).isSelected())
            {
                return;
            }
            int iColor = dataset % _colors.length; // Colors wrap
            g.setColor(_colors[iColor]);
            g.fillRect(x + 1, y + 1, dimension.width - 2, dimension.height - 2);
            g.setColor(Color.BLACK);
            g.drawRect(x, y, dimension.width - 1, dimension.height - 1);
        }

        public int getIconWidth() {
            return dimension.width;
        }

        public int getIconHeight() {
            return dimension.height;
        }
    }
}
