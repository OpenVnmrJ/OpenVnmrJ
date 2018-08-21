/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * Copyright (c) 1998-2001 The Regents of the University of California.
 * All rights reserved.
 */

/*
 * Adapted from the PtPlot plotting package.
 */

/* Graphics class supporting EPS export from plots.

 Copyright (c) 1998-2001 The Regents of the University of California.
 All rights reserved.
 Permission is hereby granted, without written agreement and without
 license or royalty fees, to use, copy, modify, and distribute this
 software and its documentation for any purpose, provided that the above
 copyright notice and the following two paragraphs appear in all copies
 of this software.

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

                                        PT_COPYRIGHT_VERSION_2
                                        COPYRIGHTENDKEY
@ProposedRating Yellow (cxh@eecs.berkeley.edu)
@AcceptedRating Yellow (cxh@eecs.berkeley.edu)
*/
package vnmr.bo;

import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.Composite;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Image;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.GraphicsConfiguration;
import java.awt.Paint;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.RenderingHints;
import java.awt.Shape;
import java.awt.Stroke;
import java.awt.Toolkit;
import java.awt.font.FontRenderContext;
import java.awt.font.GlyphVector;
import java.awt.image.BufferedImage;
import java.awt.image.BufferedImageOp;
import java.awt.image.ImageObserver;
import java.awt.image.RenderedImage;
import java.awt.image.renderable.RenderableImage;
import java.awt.datatransfer.Clipboard;
import java.awt.datatransfer.StringSelection;
import java.awt.geom.AffineTransform;
import java.io.BufferedOutputStream;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.text.AttributedCharacterIterator;
import java.util.Map;

//import vnmr.util.Messages;

//////////////////////////////////////////////////////////////////////////
//// EPSGraphics
/**
Graphics class supporting EPS export from plots.

@author Edward A. Lee
*/

public class EPSGraphics extends Graphics2D {

    /** Constructor for a graphics object that writes encapsulated
     *  PostScript to the specified output stream.  If the out argument is
     *  null, then it writes to standard output (it would write it to
     *  the clipboard, but as of this writing, writing to the clipboard
     *  does not work in Java).
     *  @param out The stream to write to, or null to write to standard out.
     */
    public EPSGraphics(OutputStream out) {
        _left = 36;
        _top = 720;
        _out = out;
        _buffer.append("%!PS-Adobe-3.0 EPSF-3.0\n");
        _buffer.append("%%BoundingBox: 36 36 " + (36 + 540) + " "
                + (36 + 720) +"\n");
        _buffer.append("%%Creator: VnmrJ\n");
        _buffer.append("%%Pages: 1\n");
        _buffer.append("%%Page: 1 1\n");
        _buffer.append("%%LanguageLevel: 2\n");
        defineProcedures();
    }

    public EPSGraphics(OutputStream out, int ptLeft, int ptTop,
                       int ptWidth, int ptHeight,
                       int pixWidth, int pixHeight,
                       String creator, String date, String title) {
        /*Messages.postDebug("EPSGraphics(ptLeft=" + ptLeft + ", ptTop=" + ptTop
                           + ", ptWidth=" + ptWidth + ", ptHeight=" + ptHeight
                           + ", pixWidth=" + pixWidth
                           + ", pixHeight=" + pixHeight + ")");/*CMP*/
        double scale = 1;
        if (ptHeight > 0 && pixHeight > 0) {
            scale = (double)ptHeight / pixHeight;
            ptWidth = (int)(scale * pixWidth);
        } else if (ptWidth > 0 && pixWidth > 0) {
            scale = (double)ptWidth / pixWidth;
            ptHeight = (int)(scale * pixHeight);
        } else {
            ptWidth = (int)(72 * 8.5 - ptLeft);
            ptHeight = 72 * 11 - ptTop;
        }
        _left = (int)(ptLeft / scale);
        _top = (int)(ptTop / scale);
        System.out.println("EPSGraphics: _left=" + _left + ", _top=" + _top
                           + ", scale=" + scale);/*CMP*/
        _out = out;
        _buffer.append("%!PS-Adobe-3.0 EPSF-3.0\n");
        _buffer.append("%%BoundingBox: "
                       + ptLeft + " "
                       + (ptTop - ptHeight) + " "
                       + (ptLeft + ptWidth) + " "
                       + ptTop +"\n");
        _buffer.append("%%Creator: " + creator + "\n");
        _buffer.append("%%CreationDate: " + date + "\n");
        _buffer.append("%%Title: " + title + "\n");
        _buffer.append("%%Pages: 1\n");
        _buffer.append("%%Page: 1 1\n");
        _buffer.append("%%LanguageLevel: 2\n");
        defineProcedures();
        _buffer.append("" + scale + " " + scale + " scale\n");
    }

    private void defineProcedures() {
        // NB: UPDATE DICTIONARY SIZE IN THE NEXT LINE IF YOU ADD
        // DEFINITIONS HERE.
        _buffer.append("1 dict begin\n"); // Begin EPS dictionary scope
        _buffer.append("/cliprect\n");
        _buffer.append("{ newpath\n");
        _buffer.append("  moveto\n"); // Go to x, y
        _buffer.append("  neg dup 0 exch rlineto\n"); // Draw to x, y+height
        _buffer.append("  exch 0 rlineto\n"); // Draw to x+width, y+height
        _buffer.append("  0 exch neg rlineto\n"); // Draw to x+width, y
        _buffer.append("  closepath\n");
        _buffer.append("  clip } def\n");
        _buffer.append("gsave\n");
    }


    ///////////////////////////////////////////////////////////////////
    ////                         public methods                    ////

    /*
     * "Graphics2D" methods
     */
    
    public void addRenderingHints(Map<?,?> hints) {}

    public void clip(Shape s) {}

    public void draw(Shape s) {}

    public void draw3DRect(int x, int y, int width, int height,
                           boolean raised) {}

    public void drawGlyphVector(GlyphVector g, float x, float y) {}

    public void drawImage(BufferedImage img, BufferedImageOp op,
                          int x, int y) {}

    public boolean drawImage(Image img, AffineTransform xform,
                             ImageObserver obs) { return false; }

    public void drawRenderableImage(RenderableImage img,
                                    AffineTransform xform) {}

    public void drawRenderedImage(RenderedImage img, AffineTransform xform) {}

    public void drawString(AttributedCharacterIterator iterator,
                           float x, float y) {}

    public void drawString(String s, float x, float y) {}

    public void fill(Shape s) {}

    public Color getBackground() { return null; }

    public Composite getComposite() { return null; }

    public GraphicsConfiguration getDeviceConfiguration() { return null; }

    public FontRenderContext getFontRenderContext() { return null; }

    public Paint getPaint() { return null; }

    public Object getRenderingHint(RenderingHints.Key hintKey) { return null; }

    public RenderingHints getRenderingHints() { return null; }

    public Stroke getStroke() { return null; }

    public AffineTransform getTransform() { return null; }

    public boolean hit(Rectangle rect, Shape s, boolean onStroke) {
        return false;
    }

    public void rotate(double theta) {}

    public void rotate(double theta, double x, double y) {
        Point axis = _convert((int)x, (int)y);
        _buffer.append("" + axis.x + " " + axis.y + " translate\n");
        _buffer.append("" + -Math.toDegrees(theta) + " rotate\n");
        _buffer.append("" + -axis.x + " " + -axis.y + " translate\n");
    }

    public void scale(double sx, double sy) {}

    public void setBackground(Color color) {}

    public void setComposite(Composite comp) {}

    public void setPaint(Paint paint) {}

    public void setRenderingHint(RenderingHints.Key hintKey,
                                 Object hintValue) {}

    public void setRenderingHints(Map<?,?> hints) {}

    public void setStroke(Stroke s) {
        //Messages.postDebug("EPS", "setStroke()");
        if (s instanceof BasicStroke) {
            BasicStroke bs = (BasicStroke)s;
            float[] dashes = bs.getDashArray();
            _buffer.append("[");
            String sp = "";
            for (int i = 0; i < dashes.length; i++) {
                //Messages.postDebug("EPS", "dashes[" + i + "]=" + dashes[i]);
                int j = (int)dashes[i];
                if (j > 0) {
                    _buffer.append(sp + j);
                    sp = " ";
                }
            }
            _buffer.append("] 0 setdash\n");
            _buffer.append(bs.getLineWidth() + " setlinewidth\n");
            return;
        }
        System.out.println("EPSGraphics: "
                           + "Trying to set a general (non-Basic) Stroke");
        _buffer.append("[] 0 setdash\n"); // Solid line
    }

    public void setTransform(AffineTransform Tx) {}

    public void shear(double shx, double shy) {}

    public void transform(AffineTransform Tx) {}

    public void translate(double tx, double ty) {}

    /*
     * "Graphics" methods
     */
    
    public void clearRect(int x, int y, int width, int height) {
    }

    public void clipRect(int x, int y, int width, int height) {
    }

    public void copyArea(int x, int y, int width, int height, int dx, int dy) {
    }

    public Graphics create() {
        return new EPSGraphics(_out /*_width, _height*/);
    }

    public void dispose() {
    }

    public void drawArc(int x, int y, int width, int height,
            int startAngle, int arcAngle) {
    }

    public boolean drawImage(Image img, int x, int y, ImageObserver observer) {
        return true;
    }

    public boolean drawImage(Image img, int x, int y, int width, int height,
            ImageObserver observer) {
        return true;
    }

    public boolean drawImage(Image img, int x, int y, Color bgcolor,
            ImageObserver observer) {
        return true;
    }

    public boolean drawImage(Image img, int x, int y, int width,
            int height, Color bgcolor, ImageObserver observer) {
        return true;
    }

    public boolean drawImage(Image img, int dx1, int dy1, int dx2, int dy2,
            int sx1, int sy1, int sx2, int sy2, ImageObserver observer) {
        return true;
    }

    public boolean drawImage(Image img,
            int dx1,
            int dy1,
            int dx2,
            int dy2,
            int sx1,
            int sy1,
            int sx2,
            int sy2,
            Color bgcolor,
            ImageObserver observer) {
        return true;
    }

    /** Draw a line, using the current color, between the points (x1, y1)
     *  and (x2, y2) in this graphics context's coordinate system.
     *  @param x1 the x coordinate of the first point.
     *  @param y1 the y coordinate of the first point.
     *  @param x2 the x coordinate of the second point.
     *  @param y2 the y coordinate of the second point.
     */
    public void drawLine(int x1, int y1, int x2, int y2) {
        Point start = _convert(x1, y1);
        Point end = _convert(x2, y2);
        _buffer.append("newpath " + start.x + " " + start.y + " moveto\n");
        _buffer.append("" + end.x + " " + end.y + " lineto\n");
        _buffer.append("stroke\n");
    }

    public void drawPolyline(int xPoints[], int yPoints[], int nPoints) {
        if (nPoints < 2) {
            return;
        }
        Point pt = _convert(xPoints[0], yPoints[0]);
        _buffer.append("newpath " + pt.x + " " + pt.y + " moveto\n");
        for (int i=1; i<nPoints; ++i) {
            pt = _convert(xPoints[i], yPoints[i]);
            _buffer.append("" + pt.x + " " + pt.y + " lineto\n");
        }
        _buffer.append("stroke\n");
    }

    /** Draw a closed polygon defined by arrays of x and y coordinates.
     *  Each pair of (x, y) coordinates defines a vertex. The third argument
     *  gives the number of vertices.  If the arrays are not long enough to
     *  define this many vertices, or if the third argument is less than three,
     *  then nothing is drawn.
     *  @param xPoints An array of x coordinates.
     *  @param yPoints An array of y coordinates.
     *  @param nPoints The total number of vertices.
     */
    public void drawPolygon(int xPoints[], int yPoints[], int nPoints) {
        if (!_polygon(xPoints, yPoints, nPoints)) {
            return;
        } else {
            _buffer.append("closepath stroke\n");
        }
    }

    /** Draw an oval bounded by the specified rectangle with the current color.
     *  @param x The x coordinate of the upper left corner
     *  @param y The y coordinate of the upper left corner
     *  @param width The width of the oval to be filled.
     *  @param height The height of the oval to be filled.
     */
    // TODO: Currently, this ignores the fourth argument and draws a circle
    // with diameter given by the third argument.
    public void drawOval(int x, int y, int width, int height) {
        int radius = width/2;
        Point center = _convert(x + radius, y + radius);
        _buffer.append("newpath " + center.x + " " + center.y + " "
                + radius + " 0 360 arc closepath stroke\n");
    }

    public void drawRect(int x, int y, int width, int height) {
        Point start = _convert(x, y);
        _buffer.append("newpath " + start.x + " " + start.y + " moveto\n");
        _buffer.append("0 " + (-height) + " rlineto\n");
        _buffer.append("" + width + " 0 rlineto\n");
        _buffer.append("0 " + height + " rlineto\n");
        _buffer.append("" + (-width) + " 0 rlineto\n");
        _buffer.append("closepath stroke\n");
    }

    public void drawRoundRect(int x, int y, int width, int height,
            int arcWidth, int arcHeight) {
    }

    public void drawString(java.text.AttributedCharacterIterator iterator,
            int x, int y) {
        // TODO: This method is present in the graphics class in JDK1.2,
        // but not in JDK1.1.
        throw new RuntimeException(
                "Sorry, drawString(java.text.AttributedCharacterIterator, " +
                "int , int) is not implemented in EPSGraphics");
    }

    public void drawString(String str, int x, int y) {
        Point start = _convert(x, y);
        _buffer.append("" + start.x + " " + start.y + " moveto\n");
        _buffer.append("(" + str + ") show\n");
    }

    public void fillArc(int x, int y, int width, int height,
            int startAngle, int arcAngle) {
    }

    /** Draw a filled polygon defined by arrays of x and y coordinates.
     *  Each pair of (x, y) coordinates defines a vertex. The third argument
     *  gives the number of vertices.  If the arrays are not long enough to
     *  define this many vertices, or if the third argument is less than three,
     *  then nothing is drawn.
     *  @param xPoints An array of x coordinates.
     *  @param yPoints An array of y coordinates.
     *  @param nPoints The total number of vertices.
     */
    public void fillPolygon(int xPoints[], int yPoints[], int nPoints) {
        if (!_polygon(xPoints, yPoints, nPoints)) {
            return;
        } else {
            _buffer.append("closepath fill\n");
        }
    }

    /** Fill an oval bounded by the specified rectangle with the current color.
     *  @param x The x coordinate of the upper left corner
     *  @param y The y coordinate of the upper left corner
     *  @param width The width of the oval to be filled.
     *  @param height The height of the oval to be filled.
     */
    // TODO: Currently, this ignores the fourth argument and draws a circle
    // with diameter given by the third argument.
    public void fillOval(int x, int y, int width, int height) {
        int radius = width/2;
        Point center = _convert(x + radius, y + radius);
        _buffer.append("newpath " + center.x + " " + center.y + " "
                + radius + " 0 360 arc closepath fill\n");
    }

    /** Fill the specified rectangle.
     *  The left and right edges of the rectangle are at x and x + width - 1.
     *  The top and bottom edges are at y and y + height - 1.
     *  The resulting rectangle covers an area width pixels wide by
     *  height pixels tall. The rectangle is filled using the
     *  brightness of the current color to set the level of gray.
     *  @param x The x coordinate of the top left corner.
     *  @param y The y coordinate of the top left corner.
     *  @param width The width of the rectangle.
     *  @param height The height of the rectangle.
     */
    public void fillRect(int x, int y, int width, int height) {
        Point start = _convert(x, y);
        _fillPattern();
        _buffer.append("newpath " + start.x + " " + start.y + " moveto\n");
        _buffer.append("0 " + (-height) + " rlineto\n");
        _buffer.append("" + width + " 0 rlineto\n");
        _buffer.append("0 " + height + " rlineto\n");
        _buffer.append("" + (-width) + " 0 rlineto\n");
        _buffer.append("closepath gsave fill grestore\n");
        //_buffer.append("0.5 setlinewidth 0 setgray [] 0 setdash stroke\n");
        // reset the gray scale to black
        _buffer.append("1 setlinewidth\n");
    }

    public void fillRoundRect(int x, int y, int width, int height,
            int arcWidth, int arcHeight) {
    }

    public Shape getClip() {
        return null;
    }

    public Rectangle getClipBounds() {
        return null;
    }

    public Color getColor() {
        return _currentColor;
    }

    public Font getFont() {
        return _currentFont;
    }

    public FontMetrics getFontMetrics(Font f) {
        return null;
    }

    public void setFont(Font font) {
        if (font == null) return;
        int size = font.getSize();
        boolean bold = font.isBold();
        if (bold) {
            _buffer.append("/Helvetica-Bold findfont\n");
        } else {
            _buffer.append("/Helvetica findfont\n");
        }
        _buffer.append("" + size + " scalefont setfont\n");
        _currentFont = font;
    }

    public void setClip(Shape clip) {
    }

    public void setClip(int x, int y, int width, int height) {
        Point p = _convert(x, y);
        _buffer.append("" + width + " " + height + " " + p.x + " " + p.y
                       + " cliprect\n");
    }

    /** Set the current color.
     *  @param c The desired current color.
     */
    public void setColor(Color c) {
        _currentColor = c;
        int red = _currentColor.getRed();
        int green = _currentColor.getGreen();
        int blue = _currentColor.getBlue();
        _buffer.append("" + red/255.0 + " " + green/255.0 + " " +
                       blue/255.0 + " setrgbcolor\n");/*CMP-Color Printer*/
    }

    public void setPaintMode() {
    }

    public void setXORMode(Color c1) {
    }

    /** Issue the PostScript showpage command, then write and flush the output.
     *  If the output argument of the constructor was null, then write
     *  to the clipboard.
     */
    public void showpage() {
        _buffer.append("grestore\n");
        _buffer.append("showpage\n");
        _buffer.append("end\n"); // Pop EPS Dictionary
        if (_out != null) {
            PrintWriter output = new PrintWriter(
                    new BufferedOutputStream(_out));

            output.println(_buffer.toString());
            output.flush();
        } else {
            // Write to clipboard instead
            // NOTE: This doesn't work at least with jdk 1.3beta
            if (_clipboard == null) {
                _clipboard = Toolkit.getDefaultToolkit().getSystemClipboard();
            }
            StringSelection sel = new StringSelection(_buffer.toString());
            _clipboard.setContents(sel, sel);
        }
    }

    public void translate(int x, int y) {
    }

    ///////////////////////////////////////////////////////////////////
    ////                         private methods                   ////

    // Convert the screen coordinate system to that of postscript.
    private Point _convert(int x, int y) {
        return new Point(_left + x, _top - y);
    }

    // Draw a closed polygon defined by arrays of x and y coordinates.
    // Return false if arguments are misformed.
    private boolean _polygon(int xPoints[], int yPoints[], int nPoints) {
        if (nPoints < 3 || xPoints.length < nPoints
                || yPoints.length < nPoints) return false;
        Point start = _convert(xPoints[0], yPoints[0]);
        _buffer.append("newpath " + start.x + " " + start.y + " moveto\n");
        for (int i = 1; i < nPoints; i++) {
            Point vertex = _convert(xPoints[i], yPoints[i]);
            _buffer.append("" + vertex.x + " " + vertex.y + " lineto\n");
        }
        return true;
    }

    // Issue a command to set the color.
    // This used to be a gray level used to substitute for color.
    // TODO: Support fill patterns?
    private void _fillPattern() {
        int red = _currentColor.getRed();
        int green = _currentColor.getGreen();
        int blue = _currentColor.getBlue();
        /*
        // Scaling constants so that fully saturated R, G, or B appear
        // different.
        double bluescale = 0.6;    // darkest
        double redscale = 0.8;
        double greenscale = 1.0;   // lightest
        double fullscale = Math.sqrt(255.0*255.0*(bluescale*bluescale
                + redscale*redscale + greenscale*greenscale));
        double graylevel = Math.sqrt((double)(red*red*redscale*redscale
                + blue*blue*bluescale*bluescale
                + green*green*greenscale*greenscale))/fullscale;
        _buffer.append("" + graylevel + " setgray\n");
        */
        // NOTE -- for debugging, output color spec in comments
        //_buffer.append("%---- rgb: "+ red + " " + green + " " + blue +"\n");
        _buffer.append("" + red/255.0 + " " + green/255.0 + " " +
                       blue/255.0 + " setrgbcolor\n");/*CMP-Color Printer*/
    }

    ///////////////////////////////////////////////////////////////////
    ////                         private variables                 ////

    private Color _currentColor = Color.black;
    private Font _currentFont;
    private int _left, _top;
    private OutputStream _out;
    private StringBuffer _buffer = new StringBuffer();
    private Clipboard _clipboard;

//    // Default line patterns.
//    // TODO: Need at least 11 of these.
//    static private String[] _patterns = {
//        "[]",
//        "[1 1]",
//        "[4 4]",
//        "[4 4 1 4]",
//        "[2 2]",
//        "[4 2 1 2 1 2]",
//        "[5 3 2 3]",
//        "[3 3]",
//        "[4 2 1 2 2 2]",
//        "[1 2 5 2 1 2 1 2]",
//        "[4 1 2 1]",
//    };
//    private int _patternIndex = 0;
}
