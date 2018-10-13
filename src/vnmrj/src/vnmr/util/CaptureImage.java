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
import java.util.*;
import java.awt.*;
import javax.swing.*;
import java.awt.image.*;
import javax.imageio.*;
import javax.print.*;
import javax.print.event.*;
import javax.print.attribute.*;
import javax.print.attribute.standard.*;
// import com.sun.image.codec.jpeg.*;

import vnmr.admin.ui.WGlobal;
import vnmr.admin.util.*;

/**
 * <p>Title: CaptureImage </p>
 * <p>Description: Captures the screen with given dimension and location on the screen
 *                  to a jpeg file on the disk.</p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 */

public class CaptureImage
{

    public static final int GIF         = 10;
    public static final int JPEG        = 11;
    public static final int PCL         = 12;
    public static final int PDF         = 13;
    public static final int PNG         = 14;
    public static final int POSTSCRIPT  = 15;
    private static String imgFormat = "png";
    private static String outFile = "";

    private static Robot m_robot;

    private CaptureImage()
    {
    }

    public static Robot getRobot()
    {
        if (m_robot == null)
        {
            try
            {
                m_robot = new Robot();
            }
            catch (Exception e)
            {
                //e.printStackTrace();
                Messages.writeStackTrace(e);
                Messages.postDebug(e.toString());
            }
        }

        return m_robot;
    }

    /**
     *  Captures the screen with given dimension and location to a jpeg file.
     *  @param strFile  the absolute file path along with .jpeg to which the file
     *                  should be saved.
     *  @param dimCanvas    the dimension of the object whose image should be captured.
     *  @param pointCanvas  the location on the screen of the object whose image
     *                      should be captured.
     */
    public static boolean write(String strFile, BufferedImage img)
    {
        boolean bOk = true;

        if (strFile == null || img == null)
        {
            return false;
        }

        imgFormat = "png";
        try
        {
            File objFile = new File(strFile);
            if (!objFile.exists())
                strFile = FileUtil.savePath(strFile);

            if (strFile == null)
            {
                Messages.postDebug("File Part11/images is not writable by the user");
                return false;
            }

            if (strFile.indexOf(".jpg") > 0 || strFile.indexOf(".jpeg") > 0) {
                imgFormat = "jpg";
            } else if (strFile.indexOf(".gif") > 0) {
                imgFormat = "gif";
            }
            outFile = strFile;
            File out = new File(strFile);
            // If this message is enabled I see that the image gets captured
            // five (5) times! (on Windows XP) for each call to
            // CanvasTransferable.transferToClipboard().
            //Messages.postDebug("out=" + out + ", imgFormat=" + imgFormat);
            ImageIO.write(img, imgFormat, out);
        }
        catch (IOException ex)
        {
            bOk = false;
            Messages.postDebug(ex.toString());
            Messages.writeStackTrace(ex);
        }
        return bOk;
    }

    public static String getOutputFormat() {
         return imgFormat;
    }

    public static String getOutputFileName() {
         return outFile;
    }

    public static boolean doCapture(String strFile, Dimension dimCanvas, Point pointCanvas)
    {
        Robot robot = getRobot();

        BufferedImage img = robot.createScreenCapture(new Rectangle(pointCanvas, dimCanvas));
        return write(strFile, img);
    }


    /**
     *  Prints the image files to the default printer, using java methods.
     *  @param strFile  the absolute path of the file that needs to be printed
     *  @param nType    type of the file (GIF, JPEG ....)
     */
    public static boolean doPrint(String strFile, int nType)
    {
        boolean bOk = true;
        if (strFile == null)
        {
            return false;
        }

        try
        {
            // Open the image file
            InputStream is = new BufferedInputStream(new FileInputStream(strFile));

            // Find the default service
            DocFlavor flavor = getFlavor(nType);
            PrintService service = PrintServiceLookup.lookupDefaultPrintService();

            // Create the print job
            DocPrintJob job = service.createPrintJob();
            Doc doc = new SimpleDoc(is, flavor, null);

            // Monitor print job events; for the implementation of PrintJobWatcher,
            // see e706 Determining When a Print Job Has Finished
            PrintJobWatcher pjDone = new PrintJobWatcher(job);

            // Print it
            job.print(doc, null);

            // Wait for the print job to be done
            pjDone.waitForDone();

            // It is now safe to close the input stream
            is.close();

            Messages.postDebug("Printed the file to the default printer");
        }
        catch (Exception e)
        {
            bOk = false;
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
        return bOk;
    }

    /**
     *  Prints the image of the screen to the default printer, using java methods.
     *  @param dim  the dimension of the screen object to be captured and printed.
     *  @param point    the location on the screen.
     */
    public static void doPrintDefault(final Dimension dim, final Point point)
    {
        new Thread(new Runnable()
        {
            public void run()
            {
                try
                {
                    // Create a file where the file could be captured and saved.
                    File fileImg = File.createTempFile("atImgFile", ".jpeg");
                    String strPath = fileImg.getPath();

                    // Capture the screen to the file.
                    doCapture(strPath, dim, point);

                    // Print the file to the default printer.
                    doPrint(strPath, CaptureImage.JPEG);

                    // delete the file.
                    fileImg.delete();
                }
                catch (Exception e)
                {
                    //e.printStackTrace();
                    Messages.writeStackTrace(e);
                    Messages.postDebug(e.toString());
                }
            }
        }).start();
    }

    /**
     *  Prints the images to the default printer using unix cmd vnmr_cdump.
     *  @param dim  the dimension of the image
     *  @param point the location on the screen
     */
    public static void doPrint(final Dimension dim, final Point point)
    {
        new Thread(new Runnable()
        {
            public void run()
            {
                try
                {
                    // Create a file where the file could be captured and saved.
                    File fileImg = File.createTempFile("atImgFile", ".jpeg");
                    String strPath = fileImg.getPath();

                    // Capture the screen to the file.
                    doCapture(strPath, dim, point);

                    // Convert the file to postscript.
                    doConvert(strPath, JPEG, "ps");
                    String strCnvrPath = strPath.replaceFirst(".jpeg", ".ps");

                    // Print the file to the printer
                    String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, "vnmr_cdump " + strCnvrPath};
                    WUtil.runScript(cmd);

                    // delete the file.
                    fileImg.delete();
                }
                catch (Exception e)
                {
                    //e.printStackTrace();
                    Messages.writeStackTrace(e);
                    Messages.postDebug(e.toString());
                }
            }
        }).start();
    }

    /**
     *  One image file format is converted to the other image file format.
     *  @param strFile  absolute path of the file that needs to be converted.
     *  @param nOrigType    the current type of the file (GIF, JPEG....)
     *  @param strNewSuffix the new suffix that should be added to the new file (ps, gif....)
     */
    public static boolean doConvert(String strFile, int nOrigType, String strExt)
    {
        boolean bOk = false;

        if (strFile == null)
        {
            return bOk;
        }

        File objFile = new File(strFile);
        if (!objFile.exists())
            return bOk;

        InputStream is = null;
        OutputStream fos = null;

        try
        {
            StringBuffer sbFile = new StringBuffer(strFile);
            int nIndex = sbFile.lastIndexOf(".");

            String strFileName = (nIndex >= 0) ? sbFile.substring(0, nIndex)
                                    : strFile;

            String path = strFileName+"."+strExt;

            // Find a factory that can do the conversion
            DocFlavor flavor = getFlavor(nOrigType);
            StreamPrintServiceFactory[] factories = StreamPrintServiceFactory.lookupStreamPrintServiceFactories(
                               flavor, getType(strExt));
            if (factories.length > 0)
            {
                // Open the image file
                is = new BufferedInputStream(new FileInputStream(strFile));
                // Prepare the output file to receive the postscript
                fos = new BufferedOutputStream(new FileOutputStream(path));

                StreamPrintService service = factories[0].getPrintService(fos);

                // Create the print job
                DocPrintJob job = service.createPrintJob();
                Doc doc = new SimpleDoc(is, flavor, null);

                // Monitor print job events; for the implementation of PrintJobWatcher,
                // see e706 Determining When a Print Job Has Finished
                PrintJobWatcher pjDone = new PrintJobWatcher(job);

                // Print it
                job.print(doc, null);

                // Wait for the print job to be done
                pjDone.waitForDone();
                // It is now safe to close the streams
                bOk = true;
            }
            else if (strExt.equalsIgnoreCase("png") ||
                      strExt.equalsIgnoreCase("jpeg") ||
                      strExt.equalsIgnoreCase("gif"))
            {
                BufferedImage im = ImageIO.read(objFile);
                bOk = write(path, im);
            }
        }
        catch (Exception e)
        {
            bOk = false;
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
        finally {
            try {
               if (is != null)
                   is.close();
               if (fos != null)
                   fos.close();
            }
            catch (Exception e2) {}
        }

        return bOk;

    }

    protected static DocFlavor getFlavor(int nType)
    {
        switch(nType)
        {
            case GIF:
                return DocFlavor.INPUT_STREAM.GIF;
            case PCL:
                return DocFlavor.INPUT_STREAM.PCL;
            case PDF:
                return DocFlavor.INPUT_STREAM.PDF;
            case PNG:
                return DocFlavor.INPUT_STREAM.PNG;
            case POSTSCRIPT:
                return DocFlavor.INPUT_STREAM.POSTSCRIPT;
            default:
                return DocFlavor.INPUT_STREAM.JPEG;
        }
    }

    protected static String getType(String strExt)
    {
        String strType = "";

        if (strExt.equalsIgnoreCase("gif"))
            strType = DocFlavor.BYTE_ARRAY.GIF.getMimeType();
        else if (strExt.equalsIgnoreCase("ps"))
            strType = DocFlavor.BYTE_ARRAY.POSTSCRIPT.getMimeType();
        else if (strExt.equalsIgnoreCase("pcl"))
            strType = DocFlavor.BYTE_ARRAY.PCL.getMimeType();
        else if (strExt.equalsIgnoreCase("pdf"))
            strType = DocFlavor.BYTE_ARRAY.PDF.getMimeType();
        else if (strExt.equalsIgnoreCase("png"))
            strType = DocFlavor.BYTE_ARRAY.PNG.getMimeType();
        else
            strType = DocFlavor.BYTE_ARRAY.JPEG.getMimeType();

        return strType;
    }

    protected static class PrintJobWatcher {
        // true iff it is safe to close the print job's input stream
        boolean done = false;

        PrintJobWatcher(DocPrintJob job) {
            // Add a listener to the print job
            job.addPrintJobListener(new PrintJobAdapter() {
                public void printJobCanceled(PrintJobEvent pje) {
                    allDone();
                }
                public void printJobCompleted(PrintJobEvent pje) {
                    allDone();
                }
                public void printJobFailed(PrintJobEvent pje) {
                    allDone();
                }
                public void printJobNoMoreEvents(PrintJobEvent pje) {
                    allDone();
                }
                void allDone() {
                    synchronized (PrintJobWatcher.this) {
                        done = true;
                        PrintJobWatcher.this.notify();
                    }
                }
            });
        }
        public synchronized void waitForDone() {
            try {
                while (!done) {
                    wait();
                }
            } catch (InterruptedException e) {
            }
        }
    }


}
