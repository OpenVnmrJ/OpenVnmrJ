/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.awt.print.*;
import javax.print.*;
import java.io.File;
import java.io.IOException;

import javax.print.attribute.*;
import javax.print.attribute.standard.*;


import vnmr.admin.ui.WGlobal;
import vnmr.util.FileUtil;
import vnmr.util.Messages;
import vnmr.util.Util;
// import vnmr.util.XmlSerialization;


public class VPrintDialog {
    private static PageFormat m_pageFormat = null;
    private static PrinterJob m_printerJob = null;
    private static PrintService m_printService = null;
    private static PrintRequestAttributeSet m_printRequestAttributes
            = new  HashPrintRequestAttributeSet();

    private static final String PRINTING_PERSISTENCE = "PrinterSetup";


    static {
        readPersistence();
    }


    VPrintDialog(String generalGraphicsFilepath, DocFlavor generalDocFlavor, String postscriptFilepath) {

        DocFlavor localDocFlavor;
        String localFilepath;

        PrintService printServices[];
        printServices = PrintServiceLookup.lookupPrintServices(null, null);
        if (printServices == null || printServices.length == 0) {
                Messages.postError("No printers available");
                return;
        }
        if (m_printService == null) {
                m_printService = PrintServiceLookup.lookupDefaultPrintService();
        }
        // This printDialog returns null if the user presses Cancel --
        // the value of the PrintService and PrintRequestAttributes are
        // changed according to the user selection
        m_printService = ServiceUI.printDialog(null,
                                               getPopupX(),
                                               getPopupY(),
                                               printServices,
                                               m_printService,
                                               generalDocFlavor,
                                               m_printRequestAttributes);
        if (m_printService != null) {
            DocPrintJob docPrintJob = m_printService.createPrintJob();
            if (postscriptFilepath != null
                && m_printService.isDocFlavorSupported(DocFlavor.INPUT_STREAM.POSTSCRIPT))
            {
                localDocFlavor = DocFlavor.INPUT_STREAM.POSTSCRIPT;
                localFilepath = new String(postscriptFilepath);
            } else {
                localDocFlavor = generalDocFlavor;
                localFilepath = generalGraphicsFilepath;
            }

            Doc doc = new InputStreamDoc(localFilepath, localDocFlavor);
                m_printRequestAttributes.add(Chromaticity.COLOR);
            try {
                docPrintJob.print(doc, m_printRequestAttributes);
            } catch(PrintException pe) {
                Messages.postError("VPrintDialog.java printer error: " + pe);
            }
        }
    }

    VPrintDialog(String generalGraphicsFilepath, DocFlavor docFlavor) {
        this(generalGraphicsFilepath, docFlavor, null);
    }

    boolean isPostscriptAvailable(PrintService ps) {
        return false;
    }

    VPrintDialog() {
        PrintService printServices[];
        printServices = PrintServiceLookup.lookupPrintServices(null, null);
        if (printServices == null || printServices.length == 0) {
            System.out.println("  No printers available");
            return;
        }
        PrintRequestAttributeSet attributes = new HashPrintRequestAttributeSet();
        if (m_printService == null) {
                m_printService = PrintServiceLookup.lookupDefaultPrintService();
        }
        m_printService = ServiceUI.printDialog(null,
                                               getPopupX(),
                                               getPopupY(),
                                               printServices,
                                               m_printService,
                                               null,
                                               m_printRequestAttributes);
        if (m_printService != null) {
            showDialog(true);
        }

    }

    private static int getPopupX() {
        return VNMRFrame.getVNMRFrame().getBounds().x + 24;
    }

    private static int getPopupY() {
        return VNMRFrame.getVNMRFrame().getBounds().y + 24;
    }

    public void showDialog(boolean visible) {
        if (m_printService == null) {
            return;
        }
        if (!visible) {
            return;
        }
        DocPrintJob docPrintJob = m_printService.createPrintJob();

    }


    /* Static convenience method included in this class since it is associated
     * with printing.  It is a general utility, however, for use by any party
     * interested in file conversion.  It can obviously be generalized and/or
     * refactored further, but we will create the simple known case only for
     * now.  JGW 25 October 2007
     */
    public static boolean generatePngFile(String inputPath, String outputPath,
                  boolean isLoRes) {
        //String cmd;
        Process process;
        int previewDPI = (int)(600 / getPortraitPageHeight());
        int plotDPI = 300;
        File outfile = new File(outputPath);
        String tmpOutputPath =  outfile.getParent() + File.separator
                                + "tmp_" + outfile.getName();
        int dpi = isLoRes ? previewDPI : plotDPI;

        String[] cmdTokens = new String[9];
        cmdTokens[0] = "vnmr_gs";
        cmdTokens[1] = "-dSAFER";
        cmdTokens[2] = "-dBATCH";
        cmdTokens[3] = "-dNOPAUSE";
        cmdTokens[4] = "-dQUIET";
        cmdTokens[5] = "-sDEVICE=png16m";
        cmdTokens[6] = "-r" + dpi;
        cmdTokens[7] = "-sOutputFile=" + tmpOutputPath;
        cmdTokens[8] = inputPath;
//        cmd = "gs";
//        cmd += " -dSAFER -dBATCH -dNOPAUSE -dQUIET ";
//        cmd += " -sDEVICE=png16m -r" + dpi;
//        cmd += " -sOutputFile=" + tmpOutputPath + " " + inputPath;

         try {
            process = Runtime.getRuntime().exec(cmdTokens);
            process.waitFor();
        } catch (InterruptedException ie) {
            Messages.writeStackTrace(ie, "Wait for GhostScript interrupted");
        } catch (IOException e) {
            Messages.writeStackTrace(e, "Failed to run GhostScript");
            return false;
        }
        boolean rtn = true;
        if (isLoRes) {
                rtn = showMargins(tmpOutputPath, outputPath, dpi);
        } else {
                rtn = trimMargins(tmpOutputPath, outputPath, dpi);
        }

        return rtn;
    }

     public static String generatePdfFile(String inPostScriptPath,
                                         String outPdfDir,
                                         String outFile) {
        if (outFile == null) {
            // Bring up a dialog to get the output file
            String title = Util.getLabelString("Select PDF output file");
            FileDialog dialog = new FileDialog(VNMRFrame.getVNMRFrame(),
                                               title,
                                               FileDialog.SAVE);
            dialog.setDirectory(outPdfDir);
            dialog.setVisible(true);
            outFile = dialog.getFile();
            if (outFile != null) {
                outPdfDir = dialog.getDirectory();
            }
        }
        if (outFile != null) {
            if (outFile.indexOf(".pdf") < 0 && outFile.indexOf(".PDF") < 0) {
                outFile = outFile + ".pdf";
            }
            String outPdfPath = new File(outPdfDir, outFile).getPath();
            // TODO: Remove hard coded path
            //String cmd = FileUtil.sysdir() + "/gs/gs8.60/bin/gswin32c";
            String[] cmdTokens = new String[8];
            cmdTokens[0] = "vnmr_gs";
            cmdTokens[1] = "-dSAFER";
            cmdTokens[2] = "-dBATCH";
            cmdTokens[3] = "-dNOPAUSE";
            cmdTokens[4] = "-dQUIET";
            cmdTokens[5] = "-sDEVICE=pdfwrite";
            cmdTokens[6] = "-sOutputFile=" + outPdfPath;
            cmdTokens[7] = inPostScriptPath;
//            String cmd =  "gs ";
//            cmd += " -dSAFER -dBATCH -dNOPAUSE -dQUIET ";
//            cmd += " -sDEVICE=pdfwrite ";
//            cmd += " -sOutputFile=\"" + outPdfPath + "\" " + inPostScriptPath;
//            System.out.println("PS2PDF COMMAND: " + cmd);

            try {
                Process process = Runtime.getRuntime().exec(cmdTokens);
                process.waitFor();
            } catch (InterruptedException ie) {
                Messages.writeStackTrace(ie, "Wait for ps2pdf interrupted");
            } catch (IOException e) {
                Messages.writeStackTrace(e, "Failed to run ps2pdf");
            }

        }
        return outPdfDir;
    }

    /**
     * See if the PageFormat specifies landscape orientation.
     * @return Returns true if the PageFormat is in landscape mode.
     */
    public static boolean isLandscape() {
        return m_pageFormat.getOrientation() == PageFormat.LANDSCAPE;
    }

    /**
     * Get the width of the printer paper.
     * @return Width in inches.
     */
    public static double getPageWidth() {
        return m_pageFormat.getWidth() / 72;
    }

    /**
     * Get the Portrait mode width of the printer paper.
     * @return Width in inches.
     */
    private static double getPortraitPageWidth() {
        return isLandscape() ? getPageHeight() : getPageWidth();
    }

    /**
     * Get the height of the printer paper.
     * @return Height in inches.
     */
    public static double getPageHeight() {
        return m_pageFormat.getHeight() / 72;
    }

    /**
     * Get the Portrait mode height of the printer paper.
     * @return Height in inches.
     */
    private static double getPortraitPageHeight() {
        return isLandscape() ? getPageWidth() : getPageHeight();
    }

    /**
     * Get the left margin width in the PageFormat.
     * @return Margin width in inches.
     */
    private static double getImageX() {
        return m_pageFormat.getImageableX() / 72;
    }

    /**
     * Get the Portrait mode left margin width in the PageFormat.
     * @return Margin width in inches.
     */
    private static double getPortraitImageX() {
        return isLandscape() ? getImageY() : getImageX();
    }

    /**
     * Get the top margin width in the PageFormat.
     * @return Margin width in inches.
     */
    private static double getImageY() {
        return m_pageFormat.getImageableY() / 72;
    }

    /**
     * Get the Portrait mode top margin width in the PageFormat.
     * @return Margin width in inches.
     */
    private static double getPortraitImageY() {
        return isLandscape()
                         ? getPageWidth() - getImageWidth() - getImageX()
                         : getImageY();
    }

    /**
     * Get the width of the imagable area in the PageFormat.
     * @return Width in inches.
     */
    private static double getImageWidth() {
        return m_pageFormat.getImageableWidth() / 72;
    }

    /**
     * Get the Portrait mode width of the imagable area in the PageFormat.
     * @return Width in inches.
     */
    private static double getPortraitImageWidth() {
        return isLandscape() ? getImageHeight() : getImageWidth();
    }

/**
     * Get the height of the imagable area in the PageFormat.
     * @return Height in inches.
     */
    private static double getImageHeight() {
        return m_pageFormat.getImageableHeight() / 72;
    }

    /**
     * Get the Portrait mode height of the imagable area in the PageFormat.
     * @return Height in inches.
     */
    private static double getPortraitImageHeight() {
        return isLandscape() ? getImageWidth() : getImageHeight();
    }

    public static MediaSizeName getMediaSizeName() {
        MediaSizeName name = MediaSize.findMedia((float)getPortraitPageWidth(),
                                                 (float)getPortraitPageHeight(),
                                                 Size2DSyntax.INCH);
        return name;
    }

    /**
     * Crop "imageIn" to fit the PageFormat and put the result in "imageOut".
     * @param imageIn Path to the input file (in Unix environment).
     * @param imageOut Path to the output file (in Unix environment).
     * @param dpi Dots per inch in the image resolution.
     */
    private static boolean trimMargins(String imageIn, String imageOut, int dpi) {
        boolean rtn = true;
        int x = (int)Math.round(getImageX() * dpi);
        int y = (int)Math.round(getImageY() * dpi);
        int width = (int)Math.round(getImageWidth() * dpi);
        int height = (int)Math.round(getImageHeight() * dpi);
        int nargs = isLandscape() ? 8 : 6;
        String[] cmdTokens = new String[nargs+2];
        int i = 0;
        cmdTokens[i++] = WGlobal.SHTOOLCMD;
        cmdTokens[i++] = WGlobal.SHTOOLOPTION;
        cmdTokens[i++] = FileUtil.SYS_VNMR + "/bin/convert";
        cmdTokens[i++] = imageIn;
        if (isLandscape()) {
            cmdTokens[i++] = "-rotate";
            cmdTokens[i++] = "90<";
        }
        cmdTokens[i++] = "-crop";
        cmdTokens[i++] = width + "x" + height + "+" + x + "+" + y;
        cmdTokens[i++] = "+repage";
        cmdTokens[i++] = imageOut;

        try {
            Process process = Runtime.getRuntime().exec(cmdTokens);
            process.waitFor();
        } catch (InterruptedException ie) {
            Messages.writeStackTrace(ie, "Wait for Convert interrupted");
            rtn = false;
        } catch (IOException e) {
            Messages.writeStackTrace(e, "Failed to run Convert");
            rtn = false;
        }
        return rtn;
    }

    /**
     * Shade the margins in "imageIn" according to the PageFormat
     * and put the result in "imageOut".
     * @param imageIn Path to the input file (in Unix environment).
     * @param imageOut Path to the output file (in Unix environment).
     * @param dpi Dots per inch in the image resolution.
     */
    private static boolean showMargins(String imageIn, String imageOut, int dpi) {
        boolean rtn = true;
        int pw = (int)Math.round(getPageWidth() * dpi);
        int ph = (int)Math.round(getPageHeight() * dpi);
        int x = (int)Math.round(getImageX() * dpi);
        int y = (int)Math.round(getImageY() * dpi);
        int w = (int)Math.round(getImageWidth() * dpi);
        int h = (int)Math.round(getImageHeight() * dpi);
//        String rotate = isLandscape() ? " -rotate \"90<\"" : "";
//        String cmd = FileUtil.sysdir() + "/bin/convert "
//        + "\"" + imageIn + "\""
//        + rotate
//        // NB: fill color format is hex #rrggbbaa, where aa is transparency
//        + " -fill \"#0000dde0\""
//        + " -draw \"rectangle 0,0 " + pw + "," + y + "\""
//        + " -draw \"rectangle 0," + (y + h) + " " + pw + "," + ph + "\""
//        + " -draw \"rectangle 0," + (y + 1) + " " + x + "," + (y + h - 1) + "\""
//        + " -draw \"rectangle " + (x + w) + "," + (y + 1)
//        + " " + pw + "," + (y + h - 1) + "\""
//        + " \"" + imageOut + "\"";
        int nargs = isLandscape() ? 15 : 13;
        String[] cmdTokens = new String[nargs+2];
        int i = 0;
        cmdTokens[i++] = WGlobal.SHTOOLCMD;
        cmdTokens[i++] = WGlobal.SHTOOLOPTION;
        cmdTokens[i++] = FileUtil.sysdir() + "/bin/convert";
        cmdTokens[i++] = imageIn;
        if (isLandscape()) {
            cmdTokens[i++] = "-rotate";
            cmdTokens[i++] = "90<";
        }
        cmdTokens[i++] = "-fill";
        cmdTokens[i++] = "#0000dde0";
        cmdTokens[i++] = "-draw";
        cmdTokens[i++] = "rectangle 0,0 " + pw + "," + y;
        cmdTokens[i++] = "-draw";
        cmdTokens[i++] = "rectangle 0," + (y + h) + " " + pw + "," + ph;
        cmdTokens[i++] = "-draw";
        cmdTokens[i++] = "rectangle 0," + (y + 1) + " " + x + "," + (y + h - 1);
        cmdTokens[i++] = "-draw";
        cmdTokens[i++] = "rectangle " + (x + w) + "," + (y + 1)
                         + " " + pw + "," + (y + h - 1);
        cmdTokens[i++] = imageOut;
        try {
            Process process = Runtime.getRuntime().exec(cmdTokens);
            process.waitFor();
        } catch (InterruptedException ie) {
            Messages.writeStackTrace(ie, "Wait for GhostScript interrupted");
            rtn = false;
        } catch (IOException e) {
            Messages.writeStackTrace(e, "Failed to run GhostScript");
            rtn = false;
        }
        return rtn;
    }

     public static void displayPageDialog(ExpPanel expPanel) {
        m_pageFormat = m_printerJob.pageDialog(m_pageFormat);
        m_printService = m_printerJob.getPrintService();

        if (isLandscape()) {
                m_printRequestAttributes.add(OrientationRequested.LANDSCAPE);
        } else {
                m_printRequestAttributes.add(OrientationRequested.PORTRAIT);
        }
        MediaPrintableArea pa;
        pa = new MediaPrintableArea((float)getPortraitImageX(),
                                    (float)getPortraitImageY(),
                                            (float)getPortraitImageWidth(),
                                            (float)getPortraitImageHeight(),
                                            MediaPrintableArea.INCH);
        m_printRequestAttributes.add(pa);
        m_printRequestAttributes.add(getMediaSizeName());
        m_printRequestAttributes.add(Chromaticity.COLOR);

        init(expPanel);
        writePersistence();
    }


    private static void init(ExpPanel expPanel) {
        if (m_printerJob == null) {
            m_printerJob = PrinterJob.getPrinterJob();
            m_printerJob.setJobName("VnmrJ Printing");
            m_printService = m_printerJob.getPrintService();
            Messages.postWarning("Using default printer");
        }
        if (m_pageFormat == null) {
            m_pageFormat = new PageFormat();
            m_pageFormat.setOrientation(PageFormat.LANDSCAPE);
            m_printRequestAttributes.add(OrientationRequested.LANDSCAPE);
            Messages.postWarning("Using default page layout");
        }

        if (expPanel != null) {
            writeDeviceTable(expPanel);

            // Make Vnmr reread the devicetable file
            String orientation = isLandscape() ? "'landscape'" : "'portrait'";
            expPanel.sendToActiveVnmr("plotter='ps'"
                                      + " pageSize_mm[1]="
                                      + (25.4 * getPortraitPageWidth())
                                      + " pageSize_mm[2]="
                                      + (25.4 * getPortraitPageHeight())
                                      + " pageOrientation=" + orientation
            );
        }
    }

    private static void readPersistence() {
/***************************************
        State state;
        state = (State)XmlSerialization.readPersistence(new State(),
                                                        PRINTING_PERSISTENCE);
        if (state == null) {
            Messages.postWarning("Could not restore printer setup");
        } else {
            String printerName = state.printerName;
            m_printService = getServiceByName(printerName);
            m_printerJob = PrinterJob.getPrinterJob();
            if (m_printService == null) {
                m_printerJob = null;
            } else {
                if (m_printerJob != null) {
                    try {
                        m_printerJob.setPrintService(m_printService);
                    } catch (PrinterException e) {
                        m_printerJob = null;
                    }
                }
            }
            m_pageFormat = state.pageFormat;
        }
******************************************/
        init(null);
    }


    private static void writePersistence() {
        /*****
        State state = new State();
        state.printerName = m_printService.getName();
        state.pageFormat = m_pageFormat;
        XmlSerialization.writePersistence(PRINTING_PERSISTENCE, state);
        ****/
    }

    private static PrintService getServiceByName(String name) {
        PrintService rtn = null;
        PrintService printServices[];
        printServices = PrintServiceLookup.lookupPrintServices(null, null);
        for (PrintService ps : printServices) {
             // System.out.println("ps name=" + ps.getName());/*CMP*/
             if (name.equals(ps.getName())) {
                  rtn = ps;
                  break;
             }
        }
        return rtn;
    }

    public static void writeDeviceTable(ExpPanel expPanel) {
        float pagew = (float)(getPageWidth() * 25.4);
        float pageh = (float)(getPageHeight() * 25.4);
        float imagew = (float)(getImageWidth() * 25.4);
        float imageh = (float)(getImageHeight() * 25.4);
        float imagex = (float)(getImageX() * 25.4);
        float imagey = (float)(getImageY() * 25.4);
        String type = "'PostScript'";
        String cmd = "writeDeviceTable("
                + type + ", "
                + pagew + ", " + pageh + ", "
                + imagex + ", " + imagey + ", "
                + imagew + ", " + imageh + ")";

        expPanel.sendToActiveVnmr(cmd);
    }

    /**
     * Container class for holding our state.
     * Used for saving/restoring persistence file.
     */
    private static class State {
        public String printerName = null;
        public PageFormat pageFormat = null;
    }

}
