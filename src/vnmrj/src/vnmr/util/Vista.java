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

import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.geom.Point2D;
import java.awt.print.Pageable; 
import java.awt.print.PageFormat;
import java.awt.print.Printable;
import java.awt.print.PrinterException;

/**
 * A simple Pageable class that can
 * split a large drawing canvas over multiple
 * pages.
 *
 * The pages in a canvas are laid out on
 * pages going left to right and then top
 * to bottom.
 */

public class Vista implements Pageable {

    protected int mNumPagesX;
    protected int mNumPagesY;
    protected int mNumPages;
    protected Printable mPainter;
    protected PageFormat mFormat;

    /**
     * Create a java.awt.Pageable that will print
     *  a canvas over as many pages as are needed.
     * A Vista can be passed to PrinterJob.setPageable.
     *
     * @param width The width, in 1/72nds of an inch, 
     * of the vist's canvas.
     *
     * @param height The height, in 1/72nds of an inch, 
     * of the vista's canvas.
     *
     * @param painter The object that will drawn the contents
     * of the canvas.
     *
     * @param format The description of the pages on to which
     * the canvas will be drawn.
     */
    public Vista(float width, float height,
                 Printable painter, PageFormat format) {
        setPrintable(painter);
        setPageFormat(format);
        setSize(width, height);
    }

    /**
     * Create a vista over a canvas whose width and height
     * are zero and whose Printable and PageFormat are null.
     */
    protected Vista() {
    }

    /**
     * Set the object responsible for drawing the canvas.
     */
    protected void setPrintable(Printable painter) {
        mPainter = painter;
    }

    /**
     * Set the page format for the pages over which the
     * canvas will be drawn.
     */
    protected void setPageFormat(PageFormat pageFormat) {
        mFormat = pageFormat;
    }

    /**
     * Set the size of the canvas to be drawn.
     * 
     * @param width The width, in 1/72nds of an inch, of
     * the vist's canvas.
     *
     * @param height The height, in 1/72nds of an inch, of
     * the vista's canvas.
     */
    protected void setSize(float width, float height) {
        mNumPagesX = (int) ((width + mFormat.getImageableWidth() - 1)
                            / mFormat.getImageableWidth());
        mNumPagesY = (int) ((height + mFormat.getImageableHeight() - 1)
                            / mFormat.getImageableHeight());
        mNumPages = mNumPagesX * mNumPagesY;
    }

    /**
     * Returns the number of pages over which the canvas
     * will be drawn.
     */
    public int getNumberOfPages() {
        return mNumPages;
    }

    protected PageFormat getPageFormat() {
        return mFormat;
    }

    /** 
     * Returns the PageFormat of the page specified by
     * pageIndex. For a Vista the PageFormat
     * is the same for all pages.
     *
     * @param pageIndex the zero based index of the page whose
     * PageFormat is being requested
     * @return the PageFormat describing the size and
     * orientation.
     * @exception IndexOutOfBoundsException
     * the Pageable  does not contain the requested
     * page.
     */
    public PageFormat getPageFormat(int pageIndex)
        throws IndexOutOfBoundsException {

        if (pageIndex >= mNumPages) {
            throw new IndexOutOfBoundsException();
        }
        return getPageFormat();
    }

    /**
     * Returns the <code>Printable</code> instance responsible for
     * rendering the page specified by <code>pageIndex</code>.
     * In a Vista, all of the pages are drawn with the same
     * Printable. This method however creates
     * a Printable which calls the canvas's
     * Printable. This new Printable
     * is responsible for translating the coordinate system
     * so that the desired part of the canvas hits the page.
     *
     * The Vista's pages cover the canvas by going left to
     * right and then top to bottom. In order to change this
     * behavior, override this method.
     *
     * @param pageIndex the zero based index of the page whose
     * Printable is being requested
     * @return the Printable that renders the page.
     * @exception IndexOutOfBoundsException
     * the Pageable does not contain the requested
     * page.
     */
    public Printable getPrintable(int pageIndex)
        throws IndexOutOfBoundsException {

        if (pageIndex >= mNumPages) {
            throw new IndexOutOfBoundsException();
        }
        double originX = (pageIndex % mNumPagesX) * mFormat.getImageableWidth();
        double originY = (pageIndex / mNumPagesX)
                * mFormat.getImageableHeight();
        Point2D.Double origin = new Point2D.Double(originX, originY);
        return new TranslatedPrintable(mPainter, origin);
    }

    /**
     * This inner class's sole responsibility is to translate
     * the coordinate system before invoking a canvas's
     * painter. The coordinate system is translated in order
     * to get the desired portion of a canvas to line up with
     * the top of a page.
     */
    public static final class TranslatedPrintable implements Printable {

        /**
         * The object that will draw the canvas.
         */
        private Printable mPainter;

        /**
         * The upper-left corner of the part of the canvas
         * that will be displayed on this page. This corner
         * is lined up with the upper-left of the imageable
         * area of the page.
         */
        private Point2D mOrigin;

        /**
         * Create a new Printable that will translate
         * the drawing done by painter on to the
         * imageable area of a page.
         *
         * @param painter The object responsible for drawing
         * the canvas
         *
         * @param origin The point in the canvas that will be
         * mapped to the upper-left corner of
         * the page's imageable area.
         */
        public TranslatedPrintable(Printable painter, Point2D origin) {
            mPainter = painter;
            mOrigin = origin;
        }

        /**
         * Prints the page at the specified index into the specified 
         * {@link Graphics} context in the specified
         * format. A PrinterJob calls the 
         * Printableinterface to request that a page be
         * rendered into the context specified by 
         * graphics. The format of the page to be drawn is
         * specified by pageFormat. The zero based index
         * of the requested page is specified by pageIndex. 
         * If the requested page does not exist then this method returns
         * NO_SUCH_PAGE; otherwise PAGE_EXISTS is returned.
         * The Graphics class or subclass implements the
         * {@link PrinterGraphics} interface to provide additional
         * information. If the Printable object
         * aborts the print job then it throws a {@link PrinterException}.
         * @param graphics the context into which the page is drawn 
         * @param pageFormat the size and orientation of the page being drawn
         * @param pageIndex the zero based index of the page to be drawn
         * @return PAGE_EXISTS if the page is rendered successfully
         * or NO_SUCH_PAGE if pageIndex specifies a
         * non-existent page.
         * @exception java.awt.print.PrinterException
         * thrown when the print job is terminated.
         */
        public int print(Graphics graphics, PageFormat pageFormat,
                         int pageIndex) throws PrinterException {
            Graphics2D g2 = (Graphics2D) graphics;
            g2.translate(-mOrigin.getX(), -mOrigin.getY());
            mPainter.print(g2, pageFormat, pageIndex);
            return PAGE_EXISTS;
        }
    }
}
