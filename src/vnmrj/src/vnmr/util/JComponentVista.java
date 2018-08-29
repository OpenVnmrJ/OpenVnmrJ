/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 *
 * This file is from "SDK 1.2 Printing API: A Tutorial"
 * <http://java.sun.com/products/java-media/2D/forDevelopers/sdk12print.html>
 *
 */

package vnmr.util;

import java.awt.*;
import java.awt.print.*;
import javax.swing.*;


public class JComponentVista extends Vista implements Printable {

    protected static final boolean SYMMETRIC_SCALING = true;
    protected static final boolean ASYMMETRIC_SCALING = false;
    protected double mScaleX;
    protected double mScaleY;

    /**
     * The Swing component to print.
     */
    protected JComponent mComponent;

    /**
     * Create a Pageable that can print a
     * Swing JComponent over multiple pages.
     *
     * @param c The swing JComponent to be printed.
     *
     * @param format The size of the pages over which
     * the componenent will be printed.
     */
    public JComponentVista(JComponent c, PageFormat format) {
        setPageFormat(format);
        setPrintable(this);
        setComponent(c);

        // Tell the Vista we subclassed the size of the canvas.
        Rectangle componentBounds = c.getBounds(null);
        setSize(componentBounds.width, componentBounds.height);
        setScale(1, 1);
    }

    protected void setComponent(JComponent c) {
        mComponent = c;
    }

    protected void setScale(double scaleX, double scaleY) {
        mScaleX = scaleX;
        mScaleY = scaleY;
    }

    public double getXScale() {
        return mScaleX;
    }

    public void setXScale(double scaleX, boolean symmetric) {
        double scaleY = symmetric ? scaleX : getYScale();
        Rectangle componentBounds = mComponent.getBounds(null);
        setSize( (float) (componentBounds.width * scaleX),
                 (float) (componentBounds.height * scaleY));
        setScale(scaleX, scaleY);
    }

    public double getYScale() {
        return mScaleY;
    }

    public void scaleToFitX() {
        PageFormat format = getPageFormat();
        Rectangle componentBounds = mComponent.getBounds(null);
        double scaleX = format.getImageableWidth() /componentBounds.width;
        double scaleY = scaleX;
        if (scaleX < 1) {
            setSize( (float) format.getImageableWidth(),
                     (float) (componentBounds.height * scaleY));
            setScale(scaleX, scaleY);
        }
    }

    public void scaleToFitY() {
        PageFormat format = getPageFormat();
        Rectangle componentBounds = mComponent.getBounds(null);
        double scaleY = format.getImageableHeight() /componentBounds.height;
        double scaleX = scaleY;
        if (scaleY < 1) {
            setSize((float)(componentBounds.width * scaleX),
                    (float)format.getImageableHeight());
            setScale(scaleX, scaleY);
        }
    }

    public void scaleToFit(boolean useSymmetricScaling) {
        PageFormat format = getPageFormat();
        Rectangle componentBounds = mComponent.getBounds(null);
        double scaleX = format.getImageableWidth() /componentBounds.width;
        double scaleY = format.getImageableHeight() /componentBounds.height;
        if (scaleX < 1 || scaleY < 1) {
            if (useSymmetricScaling) {
                if (scaleX < scaleY) {
                    scaleY = scaleX;
                } else {
                    scaleX = scaleY;
                }
            }
            setSize((float)(componentBounds.width * scaleX),
                    (float) (componentBounds.height * scaleY) );
            setScale(scaleX, scaleY);
        }
    }

    public int print(Graphics graphics, PageFormat pageFormat, int pageIndex)
        throws PrinterException {

        Graphics2D g2 = (Graphics2D) graphics;
        g2.translate(pageFormat.getImageableX(), pageFormat.getImageableY());
        Rectangle componentBounds = mComponent.getBounds(null);
        g2.translate(-componentBounds.x, -componentBounds.y);
        g2.scale(mScaleX, mScaleY);
        boolean wasBuffered = mComponent.isDoubleBuffered();
        mComponent.paint(g2);
        mComponent.setDoubleBuffered(wasBuffered);
        return PAGE_EXISTS;
    }
}
