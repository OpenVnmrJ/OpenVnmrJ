/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.awt.*;
import java.awt.print.*;
import java.awt.geom.Point2D;
import java.util.*;

import javax.print.PrintService;
import javax.print.attribute.HashPrintRequestAttributeSet;
import javax.print.attribute.PrintRequestAttributeSet;
import javax.print.attribute.standard.Destination;
import javax.swing.*;
import javax.swing.text.*;

import java.io.*;

/**
 * This class supports printing of multi-page HTML-formatted text.
 * The HTML is rendered on a JTextVista, which is Pageable and
 * Printable.
 */

public class JTextVista extends JComponentVista implements Printable {

    /**
     * The list of page boundaries.
     */
    protected ArrayList<Page> mPageList = new ArrayList<Page>();

    /**
     * Create a Pageable that can print a
     * Swing JTextComponent over multiple pages.
     * Attempts to put page breaks between lines.
     *
     * @param c The swing JTextComponent to be printed.
     *
     * @param format The size of the pages over which
     * the componenent will be printed.
     */
    public JTextVista(JTextComponent c, PageFormat format) {
        super(c, format);
    }

    /**
     * Prints a given HTML file to a given file or on a given printer.
     * Originally, this only dumped postscript output to a given file.
     * As a kluge, if outPath does not contain a "/" it is taken as a
     * printer name and the output is sent directly to the printer.
     * If outPath is "default" it is sent to the default printer.
     * 
     * @param filePath The HTML file to print.
     * @param outPath The path for PostScript output, or a printer name.
     */
    static public void printHtml(String filePath, String outPath) {
        JTextPane tp = new JTextPane();
        tp.setContentType("text/html");
        //tp.setPreferredSize(new Dimension(800, 1000));

        BufferedReader in = null;
        try {
            in = new BufferedReader
                    (new InputStreamReader(new FileInputStream(filePath),
                                           "UTF-8"));
            //in = new BufferedReader(new FileReader(filePath));
        } catch(FileNotFoundException fnfe) {
            Messages.postError("JTextVista.printHtml(): Cannot find file: "
                               + filePath);
            return;
        } catch (UnsupportedEncodingException uee) {
            // UTF-8 is always supported, but...
            uee.printStackTrace();
        }
        try {
            // This block would be a one-liner {tp.read(in, null);},
            // except that the HTML parser chokes on charset directive.
            // (See Java bug #4765240 -- 18-OCT-2002)
            // NB: This only supports UTF-8 (and ASCII, of course).
            // If need to support other charsets, parse file for "charset="
            // directive and _then_ create InputStreamReader with whatever
            // charset you found.
            // Or check if Sun has fixed this bug yet!
            // By the way, I copied this stuff from the JTextPane code:
            EditorKit kit = tp.getUI().getEditorKit(tp);
            Document doc = kit.createDefaultDocument();
            doc.putProperty("IgnoreCharsetDirective", Boolean.TRUE);
            kit.read(in, doc, 0);
            tp.setDocument(doc);
        } catch(IOException ioe) {
            Messages.postError("JTextVista.printHtml(): Cannot load file: "
                               + filePath + "\n\t" + ioe);
        } catch (BadLocationException ble) {
            // Don't expect this!
            Messages.writeStackTrace(ble);
        }

        try {
            in.close();
        } catch(IOException ee) { }

        // TODO: Can we just call tp.setBounds(...) and forget the frame?
        
        JFrame jf = new JFrame();
        //JScrollPane jsp = new JScrollPane();
        //jf.getContentPane().add(jsp);
        //jsp.add(tp);
        LayoutManager lm = new PrintLayout();
        jf.getContentPane().setLayout(lm);
        jf.getContentPane().add(tp);
        //jf.pack();
        jf.setSize(100,100);
        jf.validate();
        //jf.doLayout();
        jf.getContentPane().doLayout();
        //jf.setVisible(true);
        //jf.setVisible(false);

        JTextVista vista = new JTextVista(tp, new PageFormat());
        vista.scaleToFitX();
        if (vista.getXScale() > 0.75) {
            vista.setXScale(0.75, true);
        }

        //
        // KLUGE TO PRINT DIRECTLY TO A PRINTER
        //
        // This is the new, non-deprecateed way to use this method
        //
        String printerName = null;
        if (outPath.indexOf("/") < 0) {
            printerName = outPath;
            outPath = null;
        }
            
        if (printerName != null) {
            PrinterJob pj = PrinterJob.getPrinterJob();
            PrintService ps = pj.getPrintService();
            if (ps == null) {
                Messages.postError("No suitable printer available");
                return;
            }
            PrintService printService = null;
            if (!printerName.equalsIgnoreCase("default")) {
                // User requests a specific printer - see if we can use it
                PrintService[] services = PrinterJob.lookupPrintServices();
                boolean printerFound = false;
                for (int i = 0; i < services.length; i++) {
                    String name = services[i].getName();
                    if (printerName.equalsIgnoreCase(name)) {
                        printService = services[i];
                        printerFound = true;
                        break;
                    }
                }
                if (!printerFound) {
                    Messages.postWarning("Printer " + printerName
                            + " not found; using "
                            + ps.getName());
                }
            }
            if (printService != null) {
                // Set print job to use requested printer
                try {
                    pj.setPrintService(printService);
                } catch (PrinterException pe) {
                    Messages.postError("Cannot use printer \""
                            + printerName + "\": " + pe);
                }
            }

            // Set the print job to print the HTML-displaying component
            pj.setPageable(vista);

            // Send job to printer
            try {
                pj.print();
            } catch (PrinterException pe) {
                Messages.postError("Print job failed: " + pe);
            }
            return;
        }
        //
        // END OF KLUGE TO PRINT DIRECTLY TO A PRINTER
        //


        // Print to file specified by outPath
        PrinterJob pj = PrinterJob.getPrinterJob();
        pj.setPageable(vista);

        try {
            // Set up destination attribute
            PrintRequestAttributeSet aset = new HashPrintRequestAttributeSet();
            aset.add(new Destination(new java.net.URI("file:" + outPath)));
   
            // Print it
            pj.print(aset);
        } catch (PrinterException pe) {
            Messages.postError(pe.toString());
        } catch (java.net.URISyntaxException use) {
            Messages.postError(use.toString());
        }
    }

    public int print(Graphics graphics, PageFormat pageFormat, int pageIndex)
        throws PrinterException {

        Graphics2D g2 = (Graphics2D) graphics;
        Rectangle clipRect = g2.getClipBounds();
        int yIndex = pageIndex / mNumPagesX;
        if (yIndex >= mPageList.size()) {
            Messages.postDebug("JTextVista.print: cannot print page "
                               + pageIndex + "; only have "
                               + mPageList.size() + " page(s)");
            return NO_SUCH_PAGE;
        }
        Page page = mPageList.get(yIndex);
        clipRect.height = (int)(mScaleY * (page.bottom - page.top + 1));
        if (clipRect.y < 0) {
            clipRect.y = 0;
        }
        g2.setClip(clipRect);

        return super.print(g2, pageFormat, pageIndex);
    }

    
    /**
     * 
     */
    public Printable getPrintable(int pageIndex)
        throws IndexOutOfBoundsException {

        if (pageIndex >= mNumPages) {
            throw new IndexOutOfBoundsException();
        }
        double originX = (pageIndex % mNumPagesX) * mFormat.getImageableWidth();
        int yIndex = pageIndex / mNumPagesX;
        double originY = (mPageList.get(yIndex)).top * mScaleY;
        Point2D.Double origin = new Point2D.Double(originX, originY);

        return new TranslatedPrintable(mPainter, origin);
    }

    /**
     * 
     */
    protected void setSize(float width, float height) {
        if (mPageList == null) {
            return;
        }
        mPageList.clear();
        mNumPagesX = (int) ((width + mFormat.getImageableWidth() - 1)
                            / mFormat.getImageableWidth());
        // Calculate good page-break positions
        JTextComponent jtc = (JTextComponent)mComponent;
        int pageHeight = (int)mFormat.getImageableHeight();
        int compHeight = jtc.getHeight();
        mScaleY = (height > 0 && compHeight > 0) ? height / compHeight : 1;
        int compTop;
        int compBottom;
        mNumPagesY = 0;
        for (compTop = 0; compTop < compHeight; compTop = compBottom + 1) {
            compBottom = compTop + (int)(pageHeight / mScaleY);
            if (compBottom >= compHeight) {
                compBottom = compHeight - 1;
            } else {
                int idx = jtc.viewToModel(new Point(0, compBottom));
                try {
                    Rectangle r = new Rectangle();
                    for (r = jtc.modelToView(idx);
                         r.y > compBottom;
                         r = jtc.modelToView(--idx))
                    {}
                    compBottom = r.y;
                } catch (javax.swing.text.BadLocationException ble) {
                    Messages.writeStackTrace(ble);
                }
            }
            mPageList.add(new Page(compTop, compBottom));
            mNumPagesY++;
        }
        mNumPages = mNumPagesX * mNumPagesY;
    }


    class Page {
        /** Position in component coordinates of the top of the page */
        public int top;

        /** Position in component coordinates of the bottom of the page */
        public int bottom;

        /**
         * Instantiate a page with the specified top and bottom.
         */
        Page(int top, int bottom) {
            this.top = top;
            this.bottom = bottom;
        }

        public String toString() {
            return "top=" + top + ", bottom=" + bottom;
        }
    }

    static class PrintLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {}// unused

        public void removeLayoutComponent(Component comp) {}// unused

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
            //synchronized (target.getTreeLock()) {
            int n = target.getComponentCount();
            for (int i = 0; i < n; i++) {
                Component c = target.getComponent(i);
                Dimension d = c.getPreferredSize();
                c.setBounds(0, 0, d.width, d.height);
            }
            //}
        }
    }

}
