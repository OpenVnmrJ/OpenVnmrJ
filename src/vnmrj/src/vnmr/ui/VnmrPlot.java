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
import java.io.*;
import java.awt.image.*;
import java.util.StringTokenizer;
import java.util.Collections;
import java.util.LinkedList;
import java.awt.print.*;
import javax.swing.*;
import javax.imageio.*;
import javax.print.attribute.HashPrintRequestAttributeSet;
import javax.print.attribute.PrintRequestAttributeSet;
import javax.print.attribute.standard.Destination;
import javax.print.attribute.standard.Sides;

import vnmr.bo.LoginService;
import vnmr.bo.User;
import vnmr.util.*;
import vnmr.admin.ui.*;
import vnmr.admin.util.*;


public class VnmrPlot extends JComponent implements Printable, VGaphDef
{
    private static int STROKE_NUM = 12;
    private String inFile;
    private String outFile;
    private String outFormat;
    private String printerName;
    private String orientStr;
    private String tmpPsFile;
    private String tmpPdfFile;
    private String userDir;
    private String operatorName;
    private String dateStr;
    private String pltTmpName;
    private String topFileName;
    private String sysDir;
    private String specMinStr = null;
    private String specMaxStr = null;
    private String specRatioStr = "1.0";
    private String lineThickStr = "1.0";
    private int raster;
    private int drawWidth;
    private int drawHeight;
    private int printWidth;
    private int printHeight;
    private int canvasWidth;
    private int canvasHeight;
    private int bgIndex = 20;
    private int fgIndex = 1;
    private int boxBgIndex = 255;
    private int colorSize = 0;
    private int vcolorSize = 320;
    private int pntSize;
    private int fontHeight = 12;
    private int fontAscent = 10;
    private int fontDescent = 2;
    private int charHeight = 30;
    private int bannerSize = 1;
    private int lineLw = 1;
    private int previewNo = 1;
    // private int charWidth = 15;
    // private int psLw = 1;
    private int spLw = 1;
    private int colorId;
    private int bufWidth = 0;
    private int bufHeight = 0;
    private int bufFilled = 0;
    private int bufDestX = 0;
    private int bufDestY = 0;
    private int printXoff = 0;
    private int printYoff = 0;
    private int drawXoff;
    private int drawYoff;
    private int argNum;
    private int rgbAlpha = 255;
    private int[] xPoints;
    private int[] yPoints;
    private int[] pvs;
    private double paperWidth;
    private double paperHeight;
    private double topMargin;
    private double leftMargin;
    private double scaleX = 1.0;
    private double scaleY = 1.0;
    private double lineScale = 1.0;
    private double printDpi = 1.0;
    private double scrnDpi;
    private byte[] imgByte;
    private byte[] redBytes;
    private byte[] grnBytes;
    private byte[] bluBytes;
    private Color txtColor;
    private Color bgColor;
    private Color graphColor;
    private Color aColor;
    private Graphics2D g2d;
    private Graphics2D bufGc;
    private Graphics2D prtGc;
    private boolean bExitOnDone = false;
    private boolean bCustomTxtColor;
    private boolean bThreshold;
    private boolean bPreview;
    private boolean bNoUi;
    private boolean bAipFunc;
    private boolean bDebug;
    private boolean bTopOnTop;
    private Font myFont = null;
    private Font bannerFont = null;
    private FontMetrics fontMetric = null;
    private float fontScale = 1.0f;
    private float alphaSet = 1.0f;
    private GraphicsFont graphFont;
    private BufferedReader reader = null;
    private BasicStroke specStroke = null;
    private BasicStroke lineStroke = null;
    private BasicStroke[] strokeList;
    private BufferedImage bufImage = null;
    private Toolkit tk = null;
    private IndexColorModel indexCm;
    private Rectangle defaultClip;
    private VJArrow jArrow;
    private MemoryImageSource mis = null;
    private java.util.List<GraphicsFont> graphFontList;
    private java.util.List<GraphSeries> gSeriesList;
    private java.util.List<GraphSeries> vSeriesList;
    private static double deg90 = Math.toRadians(90.0);


    public VnmrPlot() {
    }


    public void setExitOnDone(boolean b) {
         bExitOnDone = b;
    }

    private void parsePrintAttr(String attr, StringTokenizer token) {

         if (!token.hasMoreTokens())
              return;

         int k;
         String type = attr.toLowerCase();
         String value = token.nextToken().trim();

         if (type.equals("#file")) {
              outFile = value;
              return;
         }
         if (type.equals("#format")) {
              outFormat = value;
              return;
         }
         if (type.equals("#printer")) {
              printerName = value;
              return;
         }
         if (type.equals("#orientation")) {
              orientStr = value;
              return;
         }
         if (type.equals("#userdir")) {
              userDir = value;
              return;
         }
         if (type.equals("#systemdir")) {
              sysDir = value;
              return;
         }
         if (type.equals("#operator")) {
              operatorName = value;
              return;
         }
         if (type.equals("#topfile")) {
              topFileName = value;
              return;
         }
         if (type.equals("#date")) {
              dateStr = value;
              if (token.hasMoreTokens())
                 dateStr = value + token.nextToken("\r\n");
              return;
         }
         try {
             if (type.equals("#paperwidth")) {
                 paperWidth = Double.parseDouble(value); 
                 return;
             }
             if (type.equals("#paperheight")) {
                 paperHeight = Double.parseDouble(value);
                 return;
             }
             if (type.equals("#imagewidth")) {
                 printWidth = Integer.parseInt(value); 
                 return;
             }
             if (type.equals("#imageheight")) {
                 printHeight = Integer.parseInt(value); 
                 return;
             }
             if (type.equals("#canvaswidth")) {
                 canvasWidth = Integer.parseInt(value);
                 return;
             }
             if (type.equals("#canvasheight")) {
                 canvasHeight = Integer.parseInt(value);
                 return;
             }
             if (type.equals("#raster")) {
                 raster = Integer.parseInt(value); 
                 return;
             }
             if (type.equals("#noui")) {
                 k = Integer.parseInt(value); 
                 if (k > 0)
                    bNoUi = true;
                 return;
             }
             if (type.equals("#view")) {
                 k = Integer.parseInt(value); 
                 if (k > 0)
                    bPreview = true;
                 return;
             }
             if (type.equals("#debug")) {
                 k = Integer.parseInt(value); 
                 if (k > 0)
                    bDebug = true;
                 return;
             }
             if (type.equals("#charx")) {
                 // charWidth = Integer.parseInt(value);
                 return;
             }
             if (type.equals("#chary")) {
                 charHeight = Integer.parseInt(value); 
                 return;
             }
             if (type.equals("#pslw")) {
                 // psLw = Integer.parseInt(value);
                 return;
             }
             if (type.equals("#colors")) {
                 vcolorSize = Integer.parseInt(value);
                 if (vcolorSize < 256)
                     vcolorSize = 256;
                 return;
             }
             if (type.equals("#topmargin")) {
                 topMargin = Double.parseDouble(value);
                 return;
             }
             if (type.equals("#leftmargin")) {
                 leftMargin = Double.parseDouble(value);
                 return;
             }
             if (type.equals("#scalex")) {
                 scaleX = Double.parseDouble(value);
                 return;
             }
             if (type.equals("#scaley")) {
                 scaleY = Double.parseDouble(value);
                 return;
             }
             if (type.equals("#dpi")) {
                 printDpi = Double.parseDouble(value);
                 return;
             }
             if (type.equals("#topontop")) {
                 k = Integer.parseInt(value);
                 if (k < 1)
                     bTopOnTop = false;
                 return;
             }
         }
         catch (Exception e) { }
    }

    private void preparePrintItem(String type, StringTokenizer tok) {
         if (!type.equals("#func"))
            return;
         if (!tok.hasMoreTokens())
             return;
         int func = 0;
         try {
             func = Integer.parseInt(tok.nextToken());
             if (!tok.hasMoreTokens())
                 return;
             argNum = Integer.parseInt(tok.nextToken());
             if (argNum < 0 || argNum > 10)
                 return;
             for (int n = 0; n < argNum; n++) {
                 if (!tok.hasMoreTokens())
                     return;
                 pvs[n] = Integer.parseInt(tok.nextToken());
             }
         }
         catch (Exception e) { return; }
         switch (func) {
             case JTABLE: // 83
                 create_table(pvs[0], pvs[1], pvs[2], pvs[3], pvs[4], pvs[5]);
                 break;
             case VBG_WIN_GEOM: // 88
                 canvasWidth = pvs[2];
                 canvasHeight = pvs[3];
                 break;
         }

    }

    private void setUpPrinter() {
         String line, data;

         outFormat = null;
         printerName = null;
         outFile = null;
         sysDir = null;
         userDir = null;
         operatorName = null;
         dateStr = null;
         topFileName = null;
         paperWidth = 0.0;
         paperHeight = 0.0;
         printWidth = 0;
         printHeight = 0;
         canvasWidth = 0;
         canvasHeight = 0;
         leftMargin = 0.0;
         topMargin = 0.0;
         scaleX = 1.0;
         scaleY = 1.0;
         raster = 0;
         printDpi = 10.0;
         bPreview = false;
         bDebug = false;
         bTopOnTop = true;
         if (bExitOnDone)
             bNoUi = true;
         else {
             bNoUi = false;
             if (Util.getActiveView() == null) {
                 if (Util.getMainFrame() == null)
                      bNoUi = true;
             }
         }
         if (pvs == null)
             pvs = new int[12];
         if (gSeriesList != null)
             gSeriesList.clear();
         vSeriesList = null;

         VnmrCanvas canvas = null;
         ExpPanel exp = Util.getActiveView();
         if (exp != null)
             canvas = exp.getCanvas();
         if (canvas != null) {
             vSeriesList = canvas.getGraphSeries();
             if (vSeriesList != null) {
                 for (GraphSeries gs: vSeriesList) {
                     if (gs instanceof VJTable)
                        ((VJTable) gs).bPrinted = false;
                 }
             }
         }

         StringTokenizer tok;

         try {
            reader = new BufferedReader(new FileReader(inFile));
            while ((line = reader.readLine()) != null) {
                tok = new StringTokenizer(line, " \t\r\n");
                if (!tok.hasMoreTokens())
                    continue;
                data = tok.nextToken();
                if (data.startsWith("#")) {
                    if (data.equalsIgnoreCase("#endsetup"))
                        break;
                    if (tok.hasMoreTokens())
                        parsePrintAttr(data, tok);
                }
            }
            while ((line = reader.readLine()) != null) {
                tok = new StringTokenizer(line, " \t\r\n");
                if (!tok.hasMoreTokens())
                    continue;
                data = tok.nextToken();
                if (data.startsWith("#")) {
                   preparePrintItem(data, tok);
                }
            }
         }
         catch (Exception e) { }
         finally {
            try {
               if (reader != null)
                   reader.close();
               reader = null;
            }
            catch (Exception e2) {}
         }

         if (operatorName == null)
             operatorName = "vj";
         if (sysDir == null) {
            sysDir = System.getProperty("sysdir");
            if (sysDir == null)
                sysDir = File.separator+"vnmr";
         }
         if (userDir != null)
            pltTmpName = userDir+File.separator+"persistence"+File.separator+
                   "plotpreviews"+File.separator;
         else
            pltTmpName = sysDir+File.separator;
         if (operatorName == null)
             operatorName = "vj";
         pltTmpName = pltTmpName+operatorName+"_tmpplot_";
    }

    public void preview() {
         String file = FileUtil.openPath(tmpPsFile);
         if (file == null)
              return;

         tmpPdfFile = pltTmpName+Integer.toString(previewNo)+".pdf";
         previewNo++;
         if (previewNo > 30)
             previewNo = 1;
         if (bDebug)
             System.out.println("convert ps to pdf ");

         tmpPdfFile = ImageUtil.convertImageFile(tmpPsFile, tmpPdfFile, "pdf");

         String[] cmds = {
               WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION,
               FileUtil.SYS_VNMR + "/bin/vnmr_open "
               + " \"" + tmpPdfFile + "\"" };
         WUtil.runScriptNoWait(cmds,false);
    }

    private void printFile() {
         if (bPreview)
             return;

         String file = FileUtil.openPath(tmpPsFile);
         if (file == null)
             return;
         if (printerName != null) {
             String[] cmds = {
                 WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION,
                 FileUtil.SYS_VNMR + "/bin/vnmrplot \""
                 + tmpPsFile + "\" \"" + printerName + "\"" };
             WUtil.runScriptNoWait(cmds);
         }
         else {
             String[] cmds2 = {
                 WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION,
                 FileUtil.SYS_VNMR + "/bin/vnmrplot \""
                 + tmpPsFile + "\"" };
             WUtil.runScriptNoWait(cmds2);
         }
         Messages.postInfo("Printing file: " + tmpPsFile);
    }

    private void saveFile() {
         if (outFile == null)
             return;

         String file = FileUtil.openPath(tmpPsFile);
         if (file == null) {
             Messages.postError("Error: failed to create " + outFile);
             return;
         }

         int n;

         if (outFormat == null || outFormat.length() < 1) {
             outFormat = "";
             n = outFile.lastIndexOf('.');
             if (n > 0)
                 outFormat = outFile.substring(n + 1).toLowerCase();
         }
         if (outFormat.length() < 1) {
             Messages.postError("Unknown image format.");
             return;
         }

         ImageUtil.convertImageFile(tmpPsFile, outFile, outFormat);
         n = 0;
         file = null;
         try {
             while (n < 5) {
                 Thread.sleep(200);
                 file = FileUtil.openPath(outFile);
                 if (file != null)
                     break;
                 n++;
             }
         }
         catch (Exception e) {
         }
         if (file != null)
             Messages.postInfo("Saving file: " + outFile);
         else
             Messages.postError("Error: failed to create " + outFile);
    }

    private void cleanUp() {
         File fd;

         if (topFileName != null) {
             fd = new File(topFileName);
             if (fd.canWrite())
                 fd.delete();
         }
         if (inFile != null) {
             fd = new File(inFile);
             if (fd.canWrite())
                 fd.delete();
         }
    }

    public void plot(String datafile) {
         if (bDebug)
             System.out.println("VnmrPlot data file: "+datafile);
         inFile = FileUtil.openPath(datafile);
         if (inFile == null) {
             if (bExitOnDone)
                 System.exit(0);
             return;
         }
         setUpPrinter();
         updateFonts();

         Runnable runPrint = new Runnable() {
              public void run() {
                   startPrint();
              }
         };

         SwingUtilities.invokeLater(runPrint);
    }

    private void startPrint() {
         PrinterJob printJob = PrinterJob.getPrinterJob();

         PrintRequestAttributeSet attributes = new HashPrintRequestAttributeSet();
         tmpPsFile = pltTmpName+Integer.toString(previewNo)+".ps";
         try {
            attributes.add(new Destination(new java.net.URI("file:"+tmpPsFile)));
         }
         catch (Exception e) {
             return;
         }
         // attributes.add(Sides.DUPLEX);
         if (orientStr != null) {
             if (orientStr.equalsIgnoreCase("landscape"))
                 raster = 4;
         }

         if (paperWidth < 1.0)
             paperWidth = 612;  // Letter
         if (paperHeight < 1.0)
             paperHeight = 792;

         if (leftMargin > (paperWidth / 3.0))
             leftMargin = paperWidth / 3.0;
         if (topMargin > (paperHeight / 3.0))
             topMargin = paperHeight / 3.0;
         if (scaleX < 0.01 || scaleX > 1.0)
             scaleX = 0.24;
         if (scaleY < 0.01 || scaleY > 1.0)
             scaleY = 0.24;
         PageFormat page = printJob.defaultPage();
         Paper paper = page.getPaper();
         paper.setSize(paperWidth, paperHeight);
         if (bDebug)
             System.out.println("paper size: "+paperWidth+" "+paperHeight);
         if (printWidth > 0 && printHeight > 0) {
             if (printWidth > printHeight)
                 raster = 4;
             paper.setImageableArea(leftMargin,topMargin,
                  paperWidth - leftMargin * 2.0, paperHeight - topMargin * 2.0); 
         }

         if (raster == 4 || raster == 2)
             page.setOrientation(PageFormat.LANDSCAPE);
         page.setPaper(paper);
         printJob.setPrintable(this, page);

         try {
             printJob.print(attributes);
         }
         catch (Exception e2) {
             System.out.println("Error: VnmrPlot Exception !!!");
             return;
         }
         if (bDebug)
             System.out.println("printJob done");
         if (bPreview)
             preview();
         if (outFile != null)
             saveFile();
         else
             printFile();
         if (!bDebug)
             cleanUp();
    }

    private void setSpectrumWidth(int r) {
        if (r < 1)
            r = 1;
        if (r > 60)
            r = 60;
        spLw = r;

        int w = (int) (lineScale * r);
        if (w < STROKE_NUM) {
            specStroke = strokeList[w];
            return;
        }
        if (specStroke.getLineWidth() != (float) w)
            specStroke = new BasicStroke((float) w,
                        BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND);
    }

    private void setSpectrumThickness() {
         if (specMaxStr == null || specMinStr == null)
             return;
         double r = 1.0;
         try {
             r = Double.parseDouble(specRatioStr);
         } catch (NumberFormatException er) {
             r = 1.0;
         }
         int thick = DisplayOptions.getLineThicknessPix(specMinStr, specMaxStr, r);
         setSpectrumWidth(thick);
    }


    private void setLineWidth(int r) {
        if (r < 1)
            r = 1;
        if (r > 60)
            r = 60;
        lineLw = r;

        int w = (int) (lineScale * r);
        if (w < STROKE_NUM)
            lineStroke = strokeList[w];
        else {
            if (lineStroke.getLineWidth() != (float) w)
                lineStroke = new BasicStroke((float) w,
                        BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND);
        }
        g2d.setStroke(lineStroke);
    }

    private void setLineThickness() {
         int thick = DisplayOptions.getLineThicknessPix(lineThickStr);
         setLineWidth(thick);
    }

    private void setDrawLineWidth(int r) {
        setSpectrumWidth(r);
        setLineWidth(r);
    }

    private Color getTransparentColor(Color c) {
        int r = c.getRed();
        int g = c.getGreen();
        int b = c.getBlue();
        Color color = new Color(r, g, b, rgbAlpha);
        return color;
    }


    private void setTransparentColor() {
        Color color = getTransparentColor(g2d.getColor());
        g2d.setColor(color);
        graphColor = getTransparentColor(graphColor);
        txtColor = getTransparentColor(txtColor);
    }

    private void setTransparentLevel(int v)  {
       float fv = (float) v;
       if (fv > 80.0f)
           fv = 80.0f;
       else if (fv < 0.0f)
           fv = 0.0f;
       float newSet = 1.0f - fv / 100.0f;
       rgbAlpha = (int) (255.0f * newSet);
       if (newSet != alphaSet) {
          alphaSet = newSet;
          colorId = -1;
          setTransparentColor();
       }
    }

    private String getNextLine() {
         String line = null;

         if (reader == null)
             return line;
         try {
             line = reader.readLine();
         }
         catch (Exception e) {
         }
         return line;
    }

    // TODO
    private void openColormap(int id, int firstNum, int num) {
    }

    // TODO
    private void setImageGrayMap() {
         setColorModel();
    }

    // TODO
    private void switchColormap(int mapId, int whichAip) {
    }

    private void copyBufferedImage() {
         if (bufImage == null || bufFilled < 1)
             return;
         
         int sy1 = bufHeight - bufFilled;
         int sy2;
         int dy2 = bufDestY;
         int dy1 = bufDestY - bufFilled + 1;
         int w = bufWidth - 1;

         if (sy1 < 0)
             sy1 = 0;
         sy2 = sy1 + bufFilled - 1;
         g2d.drawImage(bufImage, bufDestX, dy1, bufDestX + w, dy2,
              0, sy1, w, sy2, null);
         bufFilled = 0;
         bufGc.clearRect(0, 0, bufWidth, bufHeight);
    }

    private void drawRasterImage() {
         int len = pvs[0];
         int x = pvs[1];
         int y = pvs[2];
         int repeats = pvs[3];
         int i;
         boolean bNewImage;
         String line;
         StringTokenizer tok;

         if (imgByte == null || imgByte.length <= len) {
            if (bDebug)
                System.out.println(" Rast img data length: "+len);
            imgByte = new byte[len+4]; 
            mis = null;
         }
         bNewImage = false;
         if (bufImage != null) {
            if ((bufFilled + repeats) >= bufHeight)
                bNewImage = true;
            if (x != bufDestX || bufWidth != len)
                bNewImage = true;
            if (bNewImage) {
                bNewImage = false;
                copyBufferedImage();
                bufDestY = y;
            }
         }
         if (bufImage == null || bufWidth != len || x != bufDestX)
            bNewImage = true;

         if (bNewImage) {
            bufWidth = len;
            bufHeight = repeats * 2 + 400;
            bufFilled = 0;
            bufDestX = x;
            bufDestY = y;
            if (bDebug)
                System.out.println("create image Buffer size: "+bufWidth+" "+bufHeight);
            try {
                bufImage = new BufferedImage(bufWidth, bufHeight, BufferedImage.TYPE_INT_RGB);
            }
            catch (OutOfMemoryError e) {
                 bufImage = null;
            }
            catch (Exception ex) {
                 bufImage = null;
            }

            if (bufImage != null) {
                bufGc = (Graphics2D) bufImage.createGraphics();
                bufGc.setBackground(bgColor);
                bufGc.clearRect(0, 0, bufWidth, bufHeight);
            }
            else {
                bufGc = null;
                Messages.postError("Could not allocate memory("+bufWidth+" x "+bufHeight+") for plot.");
            }
         }

         i = 0; 
         try {
             while ((line = reader.readLine()) != null) {
                 if (line.startsWith("#"))
                     break;
                 if (i >= len)
                      break;
                 tok = new StringTokenizer(line, " \r\n");
                 if (!tok.hasMoreTokens())
                     continue;
                 while (tok.hasMoreTokens()) {
                     imgByte[i] = Byte.parseByte(tok.nextToken());
                     i++;
                     if (i >= len)
                          break;
                 }
             }
         }
         catch (Exception e) {
             return;
         }

         if (i < 2)
             return;

         if (tk == null)
            tk = Toolkit.getDefaultToolkit();
         if (mis == null)
            mis = new MemoryImageSource(len, 1, indexCm, imgByte, 0, len);
         else
            mis.newPixels(imgByte, indexCm, 0, len);

         Image imgData = tk.createImage(mis);
         if (imgData == null || bufGc == null)
             return;
         int bufPtrY = bufHeight - bufFilled - 1;
         do {
              bufGc.drawImage(imgData, 0, bufPtrY, len, 1, null);
              bufFilled++;
              bufPtrY--;
              repeats--;
         } while (repeats > 0);
    }

    private void dpcon() {
         int i;
         String line;
         StringTokenizer tok;

         if (pvs[1] > 0)
             setGraphicsColor(pvs[1]);
         i = 0;
         try {
             while ((line = reader.readLine()) != null) {
                 if (line.startsWith("#"))
                     break;
                 tok = new StringTokenizer(line, " \r\n");
                 if (!tok.hasMoreTokens())
                     continue;
                 while (tok.hasMoreTokens()) {
                     xPoints[i] = Integer.parseInt(tok.nextToken());
                     if (tok.hasMoreTokens()) { 
                         yPoints[i] = Integer.parseInt(tok.nextToken());
                         i++;
                     }
                     if (i >= pntSize) {
                        g2d.drawPolyline(xPoints, yPoints, i);
                        xPoints[0] = xPoints[i-1];
                        yPoints[0] = yPoints[i-1];
                        i = 1;
                     }
                 }
             }
         }
         catch (Exception e) {
             return;
         }
         if (i > 0) {
             if (i == 1) {
                xPoints[1] = xPoints[0];
                yPoints[1] = yPoints[0];
                i = 2;
             }
             g2d.drawPolyline(xPoints, yPoints, i);
         }
    }

    private void drawIcon() {
         String name = getNextLine();
         int x = pvs[0];
         int y = pvs[1];
         int w = pvs[2];
         int h = pvs[3];
         String path = FileUtil.openPath(name);
 
         if (path == null)
             return;
         BufferedImage img = null;
         try {
             img = ImageIO.read(new File(path));
         }
         catch (IOException e) {
             img = null;
         }
         if (img == null)
            return;
         if (w < 2 || h < 2) {
             w = img.getWidth();
             h = img.getHeight();
             double r = printDpi / scrnDpi;
             w = (int) (r * w);
             h = (int) (r * h);
         }
         if (w > printWidth)
             w = printWidth;
         if (h > printHeight)
             h = printHeight;
         if ((x + w) > printWidth)
             x = printWidth - w;
         if ((y + h) > printHeight)
             y = printHeight - h;
         g2d.setRenderingHint(RenderingHints.KEY_INTERPOLATION,RenderingHints.VALUE_INTERPOLATION_BILINEAR);
         g2d.drawImage(img, x, y, w, h, null);
         g2d.setRenderingHint(RenderingHints.KEY_INTERPOLATION,RenderingHints.VALUE_INTERPOLATION_NEAREST_NEIGHBOR);
    }

    private void setColorModel() {
        if (colorSize > 255)
            indexCm = new IndexColorModel(8, 256, redBytes, grnBytes, bluBytes, bgIndex);
    }

    private void drawMultiPoints(boolean bLine, boolean bFill) {
         int i;
         String line;
         StringTokenizer tok;

         i = 0;
         try {
             while ((line = reader.readLine()) != null) {
                 if (line.startsWith("#"))
                     break;
                 tok = new StringTokenizer(line, " \r\n");
                 if (!tok.hasMoreTokens())
                     continue;
                 while (tok.hasMoreTokens()) {
                     xPoints[i] = Integer.parseInt(tok.nextToken());
                     if (tok.hasMoreTokens()) { 
                         yPoints[i] = Integer.parseInt(tok.nextToken());
                         i++;
                     }
                     if (i >= pntSize) {
                        if (bLine)
                            g2d.drawPolyline(xPoints, yPoints, i);
                        else {
                            if (bFill)
                                g2d.fillPolygon(xPoints, yPoints, i);
                            else
                                g2d.drawPolygon(xPoints, yPoints, i);
                        }
                        xPoints[0] = xPoints[i-1];
                        yPoints[0] = yPoints[i-1];
                        i = 1;
                     }
                 }
             }
         }
         catch (Exception e) {
             return;
         }
         if (i > 0) {
             if (i == 1) {
                xPoints[1] = xPoints[0];
                yPoints[1] = yPoints[0];
                i = 2;
             }
             if (bLine)
                  g2d.drawPolyline(xPoints, yPoints, i);
             else {
                  if (bFill)
                      g2d.fillPolygon(xPoints, yPoints, i);
                  else
                      g2d.drawPolygon(xPoints, yPoints, i);
             }
         }
    }

    private void drawPolyLine() {
         if (spLw != lineLw)
            g2d.setStroke(specStroke);
         drawMultiPoints(true, false);
         if (spLw != lineLw)
             g2d.setStroke(lineStroke);
    }

    private void fillPolygon() {
         drawMultiPoints(false, true);
    }

    /*****
    private void drawPolygon() {
         drawMultiPoints(false, false);
    }
    *****/

    private void drawString(String str, int x, int y) {
        Color color = g2d.getColor();
        g2d.setColor(txtColor);
        g2d.drawString(str, x, y);
        g2d.setColor(color);
    }

    private void drawText(int x, int y) {
        String str = getNextLine();
        if (str == null)
            return;
        y -= fontDescent;
        drawString(str, x, y);
    }

    public void drawIText(int x, int y, int c) {
        String str = getNextLine();
        if (str == null)
            return;
        y -= fontDescent;
        Color fcolor = txtColor;
        if (c >= 0)
           txtColor = getTransparentColor(DisplayOptions.getColor(c));
        drawString(str, x, y);
        if (c >= 0)
           txtColor = fcolor;
    }

    private void drawJText(int x, int y) {
        String str = getNextLine();
        if (str == null)
            return;
        g2d.drawString(str, x, y);
    }

    private void drawVText(int x, int y, int c) {
        String str = getNextLine();
        if (str == null)
            return;
        Color fcolor = txtColor;
        if (c >= 0)
           txtColor = getTransparentColor(DisplayOptions.getColor(c));
        g2d.setColor(txtColor);
        g2d.translate(x, y);
        g2d.rotate(-deg90);
        drawString(str, 0, fontAscent);
        g2d.rotate(deg90);
        g2d.translate(-x, -y);
        if (c >= 0)
           txtColor = fcolor;
    }

    private void drawRText(int x, int y) {
        String str = getNextLine();
        if (str == null)
            return;
        int ty = y + fontAscent;
        int tw = fontMetric.stringWidth(str) + 2;
        g2d.setColor(Color.blue);
        g2d.fillRect(x - 1, y, tw, fontHeight);
        g2d.setColor(Color.white);
        g2d.drawString(str, x, ty);
    }

    private void drawSText(int x, int y) {
        String str = getNextLine();
        if (str == null)
            return;
        if (bannerFont != null)
            g2d.setFont(bannerFont);
        g2d.drawString(str, x, y + bannerSize);
        g2d.setFont(myFont);
    }

    private void drawRBox(int x, int y) {
        String str = getNextLine();
        if (str == null)
            return;
        int tw = fontMetric.stringWidth(str) + 2;
        g2d.setColor(Color.white);
        g2d.fillRect(x, y, tw, fontHeight);
    }

    private void drawTicText(int x, int y) {
        String str = getNextLine();
        if (str == null)
            return;
        x = x - fontMetric.stringWidth(str) / 2;
        drawString(str, x, y);
    }

    private void drawHText(int x, int y) {
        String str = getNextLine();
        if (str == null)
            return;
        y -= fontDescent;
        x = x - fontMetric.stringWidth(str) / 2;
        drawString(str, x, y);
    }

    private void setBannerFont(int size) {
        if (bannerSize == size)
            return;
        bannerFont = new Font("SansSerif", Font.BOLD | Font.ITALIC, size);
        bannerSize = size;
    }

    private void drawBanner(int c) {
        String str = getNextLine();
        if (str == null)
            return;
        VStrTokenizer tok = new VStrTokenizer(str, "\\");
        int lines = tok.countTokens();
        if (lines < 1)
            return;

        FontMetrics fm;
        String d;
        int s = drawWidth - 12;
        int w = 0;
        int k, h, ygap;

        float fsize = (float) (drawHeight / (lines + 1));
        k = (int) (lineScale * 80);
        if (fsize > k)
            fsize = k;
        if (fsize < 10)
            fsize = 10;
        if (bannerFont == null)
            setBannerFont((int) fsize);
        else
            bannerFont = bannerFont.deriveFont(fsize);
        fm = getFontMetrics(bannerFont);
        d = tok.nextToken();
        while (tok.hasMoreTokens()) {
            String d2 = tok.nextToken();
            if (d2.length() > d.length())
                d = d2;
        }
        while (fsize > 10) {
            k = fm.stringWidth(d);
            k = k - s;
            h = (fm.getAscent() + fm.getDescent()) * lines - drawHeight;
            if (k > 0 || h > 0) {
                if (k > 0) {
                    k = k / d.length() + 1;
                    fsize = fsize - (float) k;
                } else if (h > 0) {
                    h = h / lines + 1;
                    fsize = fsize - (float) h;
                }
                if (fsize < 10)
                    fsize = 10;
                bannerFont = bannerFont.deriveFont(fsize);
                fm = getFontMetrics(bannerFont);
            } else
                break;
        }
                fm = getFontMetrics(bannerFont);
        w = fm.stringWidth(d);
        s = fm.getAscent() + fm.getDescent();
        ygap = s / 2;
        tok.rewind();
        int x = (drawWidth - w) / 2;
        int y;
        if (x < 0)
            x = 0;
        h = drawHeight - s * lines;
        if (h < 0)
            h = 0;
        y = (h - ygap * (lines - 1)) / 2;
        while (y < s && lines > 1) {
            if (ygap < 1)
                break;
           ygap = ygap / 2;
            y = (h - ygap * (lines - 1)) / 2;
        }
        g2d.setFont(bannerFont);
        if (y < 0)
            y = 0;
        y += fm.getAscent();

        while (tok.hasMoreTokens()) {
            d = Util.getMessageString(tok.nextToken());
            g2d.drawString(d, x, y);
            y = y + s + ygap;
        }

        g2d.setFont(myFont);
    }

    public Color getColor(int n) {
        if (n < 0 || n >= colorSize)
            return graphColor;
        if (colorId == n)
           return aColor;
        colorId = n;
        int r = (int) redBytes[n] & 0xFF;
        int g = (int) grnBytes[n] & 0xFF;
        int b = (int) bluBytes[n] & 0xFF;
        aColor = new Color(r, g, b, rgbAlpha);
        return aColor;
    }

    private void setColorIntensity(int color_index, int levels, int intensity) {
        if (color_index < 0 || color_index >= colorSize)
            return;
        if (intensity < 1)
            intensity = 1;
        if (levels < intensity)
            levels = intensity;

        int r = (int)redBytes[color_index] & 0xFF;
        int g = (int) grnBytes[color_index] & 0xFF;
        int b = (int) bluBytes[color_index] & 0xFF;
        double rate = (double) intensity / (double) levels;

        colorId = -1;
        r = (int) (rate * r);
        g = (int) (rate * g);
        b = (int) (rate * b);
        graphColor = new Color(r, g, b);
        g2d.setColor(graphColor);
    }

    private void setGraphicsColor(String str) {
        graphColor = getTransparentColor(DisplayOptions.getColor(str));
        g2d.setColor(graphColor);
        if (bCustomTxtColor)
            txtColor = graphColor;
        colorId = -1;
    }

    private void setGraphicsColor(int n) {
        if (n < 0)
            return;
        graphColor = getColor(n);
        g2d.setColor(graphColor);
        if (bCustomTxtColor)
            txtColor = graphColor;
    }

    private void setRgbAlpha(int v)  {
       float fv = (float) v;
       if (fv > 80.0f)
           fv = 80.0f;
       else if (fv < 0.0f)
           fv = 0.0f;
       float newSet = fv / 100.0f;
       rgbAlpha = (int) (255.0f * newSet);
       if (newSet != alphaSet) {
          alphaSet = newSet;
          colorId = -1;
       }
    }


    private void setColorByName(int n) {
        String str = getNextLine();
        if (str == null)
            return;
        if (n < 0 || n >= colorSize)
            return;
        Color c = VnmrRgb.getColorByName(str);
        redBytes[n] = (byte) c.getRed();
        grnBytes[n] = (byte) c.getGreen();
        bluBytes[n] = (byte) c.getBlue();
    }

    private void draw_arrow(int x1, int y1,int x2,int y2,int thick,int c) {
        if (jArrow == null)
           jArrow = new VJArrow();
        Color color = getColor(c);
        int lw = lineLw;

        if (thick > 0)
           setLineWidth(thick);
        jArrow.setEndPoints(x1, y1, x2, y2);
        jArrow.setLineWidth(lineLw);
        jArrow.setScale(lineScale);
        jArrow.setColor(color);
        jArrow.draw(g2d, false, false);

        if (thick > 0)
           setLineWidth(lw);
        g2d.setColor(graphColor);
    }


    private void draw_roundRect(int x1,int y1,int x2,int y2,int thick,int c) {
        int lw = lineLw;
        int w = x2 - x1;
        int h = y2 - y1;
        if (w < 2 || h < 2)
           return;
        int arcW = w / 3;

        if (thick > 0)
           setLineWidth(thick);
        if (arcW > 20)
            arcW = 20;
        g2d.setColor(getColor(c));
        g2d.drawRoundRect(x1, y1, w, h, arcW, arcW);

        if (thick > 0)
           setLineWidth(lw);
        g2d.setColor(graphColor);
    }

    private void draw_oval(int x1,int y1,int x2,int y2,int thick,int c) {
        int lw = lineLw;
        int w = x2 - x1;
        int h = y2 - y1;
        if (w < 3 || h < 3)
           return;

        if (thick > 0)
           setLineWidth(thick);
        g2d.setColor(getColor(c));
        g2d.drawOval(x1, y1, w, h);

        if (thick > 0)
           setLineWidth(lw);
        g2d.setColor(graphColor);
    }

    private void create_table(int id, int x,int y,int w,int h, int color) {
        String fname = getNextLine();
        String path = FileUtil.openPath(fname);
        if (path == null)
            return;

        VJTable table = null;
        if (vSeriesList != null) {
            for (GraphSeries gs: vSeriesList) {
                if (gs.isValid() && (gs instanceof VJTable)) {
                    table = (VJTable) gs;
                    if (path.equals(table.filePath) && !table.bPrinted) {
                        if (id == table.id) {
                            table.bPrinted = true;
                            break;
                        }
                    }
                    table = null;
                }
            }
        }
        if (table == null) {
            table = new VJTable(id);
            table.setValid(true);
            table.setVisible(true);
            table.setColor(Color.red);
            table.setBounds(x, y, w, h);
            table.doLayout();
            table.load(path);
        }
        if (gSeriesList == null)
            gSeriesList = Collections.synchronizedList(new LinkedList<GraphSeries>());

        gSeriesList.add(table);
    }

    private void draw_table(int id, int x,int y,int w,int h, int color) {
        String fname = getNextLine();
        String path = FileUtil.openPath(fname);
        if (gSeriesList == null)
           return;
        VJTable table = null;
        for (GraphSeries gs: gSeriesList) {
            if (gs.isValid() && (gs instanceof VJTable)) {
               table = (VJTable) gs;
               if (path.equals(table.filePath))
                    break;
               table = null;
            }
        }
        if (table != null) {
            table.print(g2d);
        }
    }

    private void frameFunc() {

        switch (pvs[0]) {
            case JFRAME_ACTIVE:
                   if (pvs[4] < 2 || pvs[5] < 2) // width and height
                       return;
                   drawWidth = pvs[4];
                   drawHeight = pvs[5];
                   if (drawWidth > printWidth)
                       drawWidth = printWidth;
                   if (drawHeight > printHeight)
                       drawHeight = printHeight;
                   if (drawXoff != 0 || drawYoff != 0)
                       g2d.translate(-drawXoff, -drawYoff);
                   drawXoff = pvs[2];
                   drawYoff = pvs[3];
                   if (drawXoff != 0 || drawYoff != 0)
                       g2d.translate(drawXoff, drawYoff);
                   setGraphicsFont(GraphicsFont.defaultName);
                   break;
        }
    }

    private void gFunc(int func) {
        int i;

        switch (func) {
            case CLEAR:  // 1
                    break;
            case XBAR: // 2
                    drawPolyLine();
                    break;
            case TEXT: // 3
                    drawText(pvs[1], pvs[2]);
                    break;
            case VTEXT: // 4
                    drawVText(pvs[1], pvs[2], pvs[3]);
                    break;
            case RASTER: // 6
                    drawRasterImage();
                    break;
            case RGB: // 7
                    i = pvs[0];
                    if (i < 0 || i >= colorSize)
                        return;
                    redBytes[i] = (byte) pvs[1];
                    grnBytes[i] = (byte) pvs[2];
                    bluBytes[i] = (byte) pvs[3];
                    break;
            case COLORI: // 8
                    fgIndex = pvs[0];
                    setGraphicsColor(pvs[0]);
                    break;
            case LINE: // 9
                    g2d.drawLine(pvs[0], pvs[1], pvs[2], pvs[3]);
                    break;
            case BOX: // 12
                    if (fgIndex != bgIndex && fgIndex != boxBgIndex)
                       g2d.fillRect(pvs[0], pvs[1], pvs[2], pvs[3]);
                    else {
                       if (pvs[2] > drawWidth / 2) {
                             if (pvs[3] > drawHeight / 2)
                                return;
                       }
                       g2d.clearRect(pvs[0], pvs[1], pvs[2] - 1, pvs[3] - 1);
                    }
                    break;
            case COLORTABLE:
                    setColorModel();
                    break;
            case REFRESH:
                    setColorModel();
                    break;
            case BGCOLOR: // 18
                    bgIndex = pvs[0];
                    break;
            case RTEXT: // 19
                    drawRText(pvs[1], pvs[2]);
                    break;
            case RBOX: // 20
                    drawRBox(pvs[1], pvs[2]);
                    break;
        }
    }

    private void g2Func(int func) {
        switch (func) {
            case STEXT:   // 21
                    drawSText(pvs[1], pvs[2]);
                    break;
            case SFONT: // 22
                    setBannerFont(pvs[0] * fontHeight);
                    break;
            case VCURSOR:
                    break;
            case ICON: // 25
                    drawIcon();
                    break;
            case COLORNAME: // 26
                    setColorByName(pvs[3]);
                    break;
            case CLEAR2:  // 31
                    break;
            case BAR: // 32
                    drawPolyLine();
                    break;
            case YBAR:  // 33
                    drawPolyLine();
                    break;
            case VIMAGE: // 34
                    break;
            case ICURSOR:
                    if (pvs[0] == 2 && bThreshold) {
                       if (pvs[3] > drawWidth)
                           pvs[3] = drawWidth;
                       int x = (drawWidth - pvs[3]) / 2;
                       g2d.drawLine(x, pvs[2], x + pvs[3], pvs[2]);
                    }
                    break;
            case DCURSOR:
                    break;
            case BANNER:
                    drawBanner(pvs[3]);
                    break;
            case ACURSOR:
                    break;
            case FGCOLOR: // 39
                    graphColor = new Color(pvs[0], pvs[1], pvs[2], rgbAlpha);
                    g2d.setColor(graphColor);
                    if (bCustomTxtColor)
                       txtColor = graphColor;
                    break;
            case GCOLOR:  // 40
                    setColorIntensity(pvs[0], pvs[1], pvs[2]);
                    break;
        }
    }


    private void g3Func(int func) {
        switch (func) {
            case PRECT:   // 41
                    g2d.drawRect(pvs[2], pvs[3], pvs[4], pvs[5]);
                    break;
            case PARC: // 42
                    g2d.drawArc(pvs[2], pvs[3], pvs[4], pvs[5], pvs[6], pvs[7]);
                    break;
            case POLYGON: // 43
                    fillPolygon();
                    break;
            case CROSS:
                    break;
            case REGION: // 45
                    if (pvs[3] < 1 ||  pvs[4] < 1) {
                       // if (defaultClip != null)
                       //    g2d.setClip(defaultClip);
                       // else
                           g2d.setClip(null);
                    }
                    else {
                        Rectangle clip = new Rectangle(pvs[1],pvs[2],pvs[3],pvs[4]);
                        g2d.setClip(clip);
                    }
                    break;
            case HCURSOR:
                    break;
            case JFRAME: // false
                    frameFunc();
                    break;
            case CLRCURSOR:
                    break;
            case CSCOLOR:
                    bThreshold = false;
                    break;
            case JALPHA: // 52
                    break;
            case HTEXT: // 53
                    drawHText(pvs[1], pvs[2]);
                    break;
            case FRMPRTSYNC: // 57
                    break;
            case TICTEXT: // 58
                    drawTicText(pvs[1], pvs[2]);
                    break;
            case XORON:
                    break;
            case XOROFF:  // 60
                    break;
        }

    }

    private void g4Func(int func) {
        String str;

        switch (func) {
            case XYBAR:   // 61
                    drawPolyLine();
                    break;
            case PENTHICK:  // 62
                    setDrawLineWidth(pvs[0]);
                    break;
            case AIP_TRANSPARENT: // 63
                    break;
            case WINPAINT: // 64
                    break;
            case SPECTRUMWIDTH: // 65
                    setSpectrumWidth(pvs[0]);
                    break;
            case LINEWIDTH: // 66
                    setLineWidth(pvs[0]);
                    break;
            case AIPID:  // 67
                    break;
            case TRANSPARENT:  // 68
                    setTransparentLevel(pvs[0]);
                    break;
            case SPECTRUM_MIN: // 69
                    str = getNextLine();
                    if (str != null)
                        specMinStr = str;
                    break;
            case SPECTRUM_MAX: // 70
                    str = getNextLine();
                    if (str != null)
                        specMaxStr = str;
                    break;
            case SPECTRUM_RATIO:  // 71
                    str = getNextLine();
                    if (str != null) {
                        specRatioStr = str;
                        setSpectrumThickness();
                    }
                    break;
            case LINE_THICK: // 72
                    str = getNextLine();
                    if (str != null) {
                        lineThickStr = str;
                        setLineThickness();
                    }
                    break;
            case GRAPH_FONT:
                    str = getNextLine();
                    if (str != null)
                         setGraphicsFont(str);
                    break;
            case GRAPH_COLOR:
                    str = getNextLine();
                    if (str != null)
                         setGraphicsColor(str);
                    break;
            case NOCOLOR_TEXT:
                    drawJText(pvs[1], pvs[2]);
                    break;
            case BACK_REGION:
                    break;
            case ICURSOR2:
                    if (pvs[0] == 2 && bThreshold) {
                       g2d.drawLine(pvs[3], pvs[2], pvs[4], pvs[2]);
                    }
                    break;
            case THSCOLOR:   // 78
                    setGraphicsColor(pvs[2]);
                    if (pvs[0] == 2)
                       bThreshold = true;
                    break;
        }

    }
    private void g5Func(int func) {
        switch (func) {
            case ENABLE_TOP_FRAME:   // 81
                    break;
            case CLEAR_TOP_FRAME: // 82
                    break;
            case JTABLE: // 83
                    draw_table(pvs[0], pvs[1], pvs[2], pvs[3], pvs[4], pvs[5]);
                    break;
            case JARROW: // 84
                    draw_arrow(pvs[0], pvs[1], pvs[2], pvs[3], pvs[4], pvs[5]);
                    break;
            case JROUNDRECT: // 85
                    draw_roundRect(pvs[0], pvs[1], pvs[2], pvs[3], pvs[4], pvs[5]);
                    break;
            case JOVAL: // 86
                    draw_oval(pvs[0], pvs[1], pvs[2], pvs[3], pvs[4], pvs[5]);
                    break;
            case RGB_ALPHA: // 87
                    setRgbAlpha(pvs[0]);
                    break;
            case VBG_WIN_GEOM: // 88
                    canvasWidth = pvs[2]; 
                    canvasHeight = pvs[3]; 
                    break;
            case IRECT:  // 102
                    setGraphicsColor(pvs[1]);
                    g2d.drawRect(pvs[2], pvs[3], pvs[4], pvs[5]);
                    break;
            case ILINE: // 103
                    setGraphicsColor(pvs[1]);
                    g2d.drawLine(pvs[2], pvs[3], pvs[4], pvs[5]);
                    break;
            case IOVAL: // 104
                    setGraphicsColor(pvs[1]);
                    g2d.drawOval(pvs[2], pvs[3], pvs[4], pvs[5]);
                    break;
            case IPOLYLINE: // 105
                    drawPolyLine();
                    break;
            case IPOLYGON:  // 106
                    fillPolygon();
                    break;
            case IBACKUP:
                    break;
            case IFREEBK:
                    break;
            case IWINDOW: // 109
                    break;
            case ICOPY: // 110
                    break;
            case IRASTER:  // 111
                    drawRasterImage();
                    break;
            case IGRAYMAP: // 112
                    setImageGrayMap();
                    break;
            case ICLEAR:  // 113
                    g2d.clearRect(pvs[1], pvs[2], pvs[3], pvs[4]);
                    break;
            case ITEXT:  // 114
                    drawIText(pvs[1], pvs[2], pvs[3]);
                    break;
            case ICOLOR: // 115
                    setGraphicsColor(pvs[0]);
                    break;
            case IPLINE: // 116
                    g2d.drawLine(pvs[2], pvs[3], pvs[4], pvs[5]);
                    break;
            case IPTEXT:  // 117
                    drawText(pvs[1], pvs[2]);
                    break;
            case DPCON:  // 118
                    if (spLw != lineLw)
                        g2d.setStroke(specStroke);
                    dpcon();
                    if (spLw != lineLw)
                        g2d.setStroke(lineStroke);
                    break;
            case SET3PMODE: // 119
                    break;
            case SET3PCURSOR: // 120
                    break;
            case ICSIWINDOW: // 121
                    break;
            case ICSIDISP: // 122
                    break;
            case OPENCOLORMAP: // 123
                    openColormap(pvs[0], pvs[1], pvs[2]);
                    break;
            case SETCOLORMAP: // 124
                    switchColormap(pvs[0], pvs[1]);
                    break;
            case SETCOLORINFO: // 125
                    break;
            case SELECTCOLORINFO: // 126
                    break;
            case ICSIORIENT:   // 127
                    break;
            case IVTEXT:   // 128
                    drawVText(pvs[1], pvs[2], pvs[3]);
                    break;
            case ANNFONT:   // 129
                    setAnnFont();
                    break;
            case ANNCOLOR:   // 130
                    setAnnColor();
                    break;
        }
    }

    private void printItem(String type, StringTokenizer tok) {
         
         int func = 0;

         bAipFunc = false;
         if (!type.equals("#func")) {
             if (type.equals("#aip"))
                 bAipFunc = true;
             else
                 return;
         }
         if (!tok.hasMoreTokens())
             return;
         try {
             func = Integer.parseInt(tok.nextToken()); 
             if (!tok.hasMoreTokens())
                 return;
             argNum = Integer.parseInt(tok.nextToken()); 
             if (argNum < 0 || argNum > 10)
                 return;
             for (int n = 0; n < argNum; n++) {
                 if (!tok.hasMoreTokens())
                     return;
                 pvs[n] = Integer.parseInt(tok.nextToken());
             }
         }
         catch (Exception e) { return; }
     
         if (func <= RBOX) {   // RBOX is 20
              gFunc(func);
              return;
         }
         if (func <= GCOLOR) {
              g2Func(func);
              return;
         }
         if (func <= XOROFF) {
              g3Func(func);
              return;
         }
         if (func <= IMG_SLIDES_END) {
              g4Func(func);
              return;
         }
         g5Func(func);
    }

    private void printGraphSeries(Graphics g, PageFormat pf, int pageIndex) {
         java.util.List<GraphSeries> seriesList;

         VnmrCanvas canvas = null;
         ExpPanel exp = Util.getActiveView();
         if (exp != null)
             canvas = exp.getCanvas();
         if (canvas == null)
             return;

         seriesList = canvas.getGraphSeries();
         if (seriesList == null)
             return;

         Dimension dim = canvas.getSize();
         Graphics2D g2c = (Graphics2D) prtGc.create();
         double r = 72.0 * 0.8 / scrnDpi;
         double pw, ph, rx;
         int  tx, ty;

         g2c.translate(printXoff, printYoff);
         g2c.scale(r, r);
         pw = pf.getImageableWidth() / r; 
         ph = pf.getImageableHeight() / r; 

         for (GraphSeries gs: seriesList) {
             if (gs.isValid()) {
                if (gs instanceof JComponent) {
                   JComponent comp = (JComponent) gs;
                   Point loc = comp.getLocation();
                   rx = (double) (loc.x + 5) / (double) dim.width;
                   tx = (int) (pw * rx);
                   rx = (double) (loc.y + 5) / (double) dim.height;
                   ty = (int) (ph * rx);
                   g2c.translate(tx, ty); 
                   comp.print(g2c);
                   g2c.translate(-tx, -ty); 
                }
                else
                   gs.draw(g2c, false, false);
             }
         }
         g2c.dispose();
    }

    private void drawFromFile(String fpath) {
         String line, data;
         StringTokenizer tok;
         BufferedReader fReader = null;

         try {
            reader = new BufferedReader(new FileReader(fpath));
            fReader = reader;
            while ((line = reader.readLine()) != null) {
                tok = new StringTokenizer(line, " \t\r\n");
                if (!tok.hasMoreTokens())
                    continue;
                data = tok.nextToken();
                if (data.startsWith("#")) {
                    if (data.equals("#endSetup"))
                        break;
                }
            }
            while ((line = reader.readLine()) != null) {
                tok = new StringTokenizer(line, " \t\r\n");
                if (!tok.hasMoreTokens())
                    continue;
                data = tok.nextToken();
                if (data.startsWith("#")) {
                    if (data.equals("#endplot"))
                        break;
                    printItem(data, tok);
                }
            }
         }
         catch (Exception e) { }
         finally {
            try {
               if (fReader != null)
                   fReader.close();
               fReader = null;
               bufImage = null;
            }
            catch (Exception e2) {}
         }
    }

    private void printPage() {

         bCustomTxtColor = true;
         bThreshold = false;
         bAipFunc = false;
         graphColor = Color.black;
         txtColor = Color.black;

         if (pvs == null)
             pvs = new int[12];
         if (xPoints == null) {
             pntSize = 202;
             xPoints = new int[pntSize];
             yPoints = new int[pntSize];
         }
         setGraphicsFont(GraphicsFont.defaultName);

         bufFilled = 0;
         spLw = 0;
         lineLw = 0;
         lineScale = 1.0;
         colorId = -1;

         if (printDpi > 100.0) {
             fontScale =(float) (printDpi / scrnDpi); 
             fontScale = fontScale * 0.85f;
         }
         else
             fontScale = (float)charHeight / (float)graphFont.originHeight;

         resetFonts();

         if (printDpi > 200.0)
             lineScale = printDpi / 150.0;
         setDrawLineWidth(1);

         if (!bTopOnTop) {
             if (topFileName != null)
                drawFromFile(topFileName);
         }
         drawFromFile(inFile);
         if (bTopOnTop) {
             if (topFileName != null)
                drawFromFile(topFileName);
         }

         g2d.drawString(" ", 0, 0);
         copyBufferedImage();
    }

    public int print(Graphics g, PageFormat pf, int pageIndex)
    {
        if (bDebug)
             System.out.println("print index "+pageIndex);
        if (inFile == null || pageIndex != 0)
             return Printable.NO_SUCH_PAGE;

        defaultClip = g.getClipBounds();
        if (bDebug)
            System.out.println(" clip: "+defaultClip.x+" "+defaultClip.y+" "+
                       defaultClip.width+" "+defaultClip.height);

        prtGc = (Graphics2D) g;
        g2d = (Graphics2D) g.create();
        scrnDpi = 90.0;
        try {
             scrnDpi = (double)Toolkit.getDefaultToolkit().getScreenResolution();
        }
        catch (Exception e) {
             scrnDpi = 90.0;
        }

        if (printWidth < 10) {
             double dw = (double)defaultClip.width / scaleX;
             printWidth = (int) dw;
        }
        if (canvasWidth < 10)
             canvasWidth = printWidth;
        if (printHeight < 10) {
             double dh = (double)defaultClip.height / scaleY;
             printHeight = (int) dh;
        }
        if (canvasHeight < 10)
             canvasHeight = printHeight;

        printXoff = (int)pf.getImageableX();
        printYoff = (int)pf.getImageableY();
        drawWidth = printWidth;
        drawHeight = printHeight;
        drawXoff = 0;
        drawYoff = 0;
        if (bDebug)
             System.out.println(" print image xy: "+printXoff+" "+printYoff);
        bgColor = Color.white;
        g2d.setBackground(bgColor);
        if (bDebug) {
            if (defaultClip.width > paperWidth / 2) {
               g2d.setColor(Color.gray);
               g2d.drawRect(defaultClip.x + 1, defaultClip.y + 1,
                    defaultClip.width - 2, defaultClip.height - 2);
            }
        }

        g2d.translate(printXoff, printYoff);
        g2d.scale(scaleX, scaleY);

        g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);

        printPage();

        printGraphSeries(g, pf, pageIndex);

        g2d.dispose();
        return Printable.PAGE_EXISTS;
    }

    private void resetFonts() {
        for (GraphicsFont gf: graphFontList)
             gf.scale(fontScale);
    }

    private void updateFonts() {
        if (graphFontList == null)
            return;
        for (GraphicsFont gf: graphFontList)
             gf.update();
    }

    private void initEnv() {
        int k;

        if (graphFontList == null) {
              graphFontList = Collections.synchronizedList(new LinkedList<GraphicsFont>());
              graphFont = new GraphicsFont(); // default font
              graphFontList.add(graphFont);
              bCustomTxtColor = true;
        }

        myFont = g2d.getFont();
        fontMetric = getFontMetrics(myFont);

        bgColor = Color.white;
        strokeList = new BasicStroke[STROKE_NUM];
        for (k = 1; k < STROKE_NUM; k++) {
             strokeList[k] = new BasicStroke((float) k,
                        BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND);
        }
        strokeList[0] = strokeList[1];
        specStroke = strokeList[0];
        lineStroke = strokeList[0];

        if (redBytes == null || colorSize <= vcolorSize) {
            colorSize = vcolorSize + 2;
            redBytes = new byte[colorSize];
            grnBytes = new byte[colorSize];
            bluBytes = new byte[colorSize];
            byte r = 120;
            byte g = 120;
            byte b = 20;
            for (k = 1; k < colorSize; k++) {
                redBytes[k] = r;
                grnBytes[k] = g;
                bluBytes[k] = b;
            }
            r = (byte) 255;
            redBytes[bgIndex] = r;
            grnBytes[bgIndex] = r;
            bluBytes[bgIndex] = r;
            redBytes[0] = r;
            grnBytes[0] = r;
            bluBytes[0] = r;
        }

        setColorModel();
    }

    private void setGraphicsFont(String name) {
        if (graphFontList == null)
            initEnv();
        if (name == null)
            return;
        GraphicsFont newGf = null;

        for (GraphicsFont gf: graphFontList) {
            if (name.equals(gf.fontName)) {
                newGf = gf;
                break;
            }
        }
        if (newGf == null) {
            newGf = new GraphicsFont(name);
            graphFontList.add(newGf);
            if (fontScale != 1.0f)
                newGf.scale(fontScale);
        }
        graphFont = newGf;

        myFont = newGf.getFont(drawWidth, drawHeight);
        g2d.setFont(myFont);
        fontMetric = graphFont.fontMetric;
        fontAscent = fontMetric.getAscent();
        fontDescent = fontMetric.getDescent();
        fontHeight = fontAscent + fontDescent;
        bCustomTxtColor = graphFont.isDefault;
        if (bCustomTxtColor)
            txtColor = graphFont.fontColor;
        if (rgbAlpha < 250)
            txtColor = getTransparentColor(txtColor);
    }

    private void setAnnFont() {
        String data = getNextLine();
        if (data == null)
            return;
        StringTokenizer tok = new StringTokenizer(data, ",\n");
        String value;
        String name = null;
        String typeStr = null;
        String colorStr = null;
        String sizeStr = null;
        int  size = 8;
        int  type = Font.PLAIN;

        while (tok.hasMoreTokens()) {
             value = tok.nextToken().trim();
             if (name == null)
                 name = value;
             else if (typeStr == null)
                 typeStr = value;
             else if (colorStr == null)
                 colorStr = value;
             else if (sizeStr == null)
                 sizeStr = value;
        }
        if (sizeStr == null)
             return;
        try {
            size = Integer.parseInt(sizeStr);
        }
        catch (NumberFormatException er) { return; }
        if (size < 8)
            size = 8;
        if (size > 80)
            size = 80;
        if (typeStr.equalsIgnoreCase("bold"))
            type = Font.BOLD;
        else if (typeStr.equalsIgnoreCase("italic"))
            type = Font.ITALIC;
        else if (typeStr.equalsIgnoreCase("bold+italic"))
            type = Font.BOLD + Font.ITALIC;
        if (fontScale != 1.0f)
            size = (int) (fontScale * size);
        Font fnt = new Font(name, type, size);
        g2d.setFont(fnt);

        if (!colorStr.equalsIgnoreCase("null")) {
            txtColor = getTransparentColor(DisplayOptions.getColor(colorStr));
        }
        bCustomTxtColor = true;
    }

    private void setAnnColor() {
        String data = getNextLine();
        if (data == null)
            return;
        colorId = -1;
        graphColor = DisplayOptions.getColor(data.trim());
        if (rgbAlpha < 250) {
            int r = graphColor.getRed();
            int g = graphColor.getGreen();
            int b = graphColor.getBlue();
            graphColor = new Color(r, g, b, rgbAlpha);
        }

        txtColor = graphColor;
        bCustomTxtColor = true;
        g2d.setColor(graphColor);
    }

    public static void initAppDirs(User user) {

    }

    public static void main(String[] args) {
         LoginService loginService = LoginService.getDefault();
         String userName = System.getProperty("user.name");
         String dataFile = System.getProperty("iplotDatafile");
         User usr = loginService.getUser(userName);

         if (dataFile == null)
             System.exit(0);
         if (usr != null) {
             initAppDirs(usr);
             FileUtil.setAppTypes(usr.getAppTypeExts());
         }
         VnmrPlot ploter = new VnmrPlot();
         ploter.setExitOnDone(true);
         ploter.plot(dataFile);
    }
}

