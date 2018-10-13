/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.io.*;
import java.util.*;
import java.awt.*;

import javax.swing.*;

import vnmr.util.*;

public class VTextBox implements CanvasObjIF
{

    protected int nX = 0;
    protected int nY = 0;
    protected int nWidth = 0;
    protected int nHeight = 0;
    protected int nPw = 200;
    protected int nPh = 200;
    protected float rx, rx2;
    protected float ry, ry2;
    protected float rw, rh; // the ratio of width and height
    protected float fontFs = 0;
    protected float newFontFs = 0;
    protected boolean bVisible = true;
    protected boolean bSelected = false;
    protected boolean bTextFrame = false; // if true, this was created by framecmd
    protected boolean bAvailable = true;
    public  ButtonIF vnmrIf = null;
    public  int  id;
    public  int  frameId;
    private int  fontH = 10;
    private int  fontAscent = 10;
    private int  fontDescent = 0;
    private int  fontStyle = 0;
    private String  fontName = Font.DIALOG;
    private String  newFontName = Font.DIALOG;
    private String  templateName = null;
    private String  textFile = null;
    private String  srcDir = null;  // the directory which contains source
    private String  saveDir = null;  // the user template directory
    private String  saveFile = null;
    private String  srcFile = null;  // the file which contains source
    private String  srcData = null;
    private String  fontSizeStr = null;
    private String  fontStyleStr = null;
    private String  newFtSizeStr = "16";
    private String  newFtStyleStr = "Plain";
    private String  colorStr = "cyan";
    private String  longStr = null;
    private Color   txColor = Color.yellow;
    private Vector  textVec = null;
    private boolean bFixedFont = true;
    private boolean  bNoSize = true;
    private Font   font = null;
    private Font   newFont = null;
    private FontMetrics  fm = null;
    private FontMetrics  newFm = null;


    public VTextBox()
    {
        this(10, 10, 10, 10);
    }

    public VTextBox(int x, int y, int w, int h)
    {
        this.nX = x;
        this.nY = y;
        this.nWidth = w;
        this.nHeight = h;
        this.id = 0;
        this.frameId = 0;
        this.rx = 0;
        this.ry = 0;
        this.rx2 = 1;
        this.ry2 = 1;
        this.rw = 0.5f;
        this.rh = 0.5f;
    }


    public int getX()
    {
        return nX;
    }

    public void setX(int x)
    {
        nX = x;
    }

    public int getY()
    {
        return nY;
    }

    public void setY(int y)
    {
        nY = y;
    }

    public int getWidth()
    {
        return nWidth;
    }

    public void setWidth(int width)
    {
        nWidth = width;
        if (nHeight > 0 && nWidth > 0)
            bNoSize = false;
    }

    public int getHeight()
    {
        return nHeight;
    }

    public void setHeight(int height)
    {
        nHeight = height;
        if (nWidth > 0 && nHeight > 0)
            bNoSize = false;
    }

    public void setTextFrameType(boolean b)
    {
        bTextFrame = b;
    }

    public boolean isTextFrameType()
    {
        return bTextFrame;
    }

    public void setDefaultSize()
    {
    }

    public void setBoundsRatio(float pW, float pH, float x, float y, float w, float h)
    {
        rx = x;
        ry = y;
        rw = w;
        rh = h;
        rx2 = rx + rw;
        ry2 = ry + rh;
        nX = (int) (x * pW);
        nY = (int) (y * pH);
        nWidth = (int) (w * pW);
        nHeight = (int) (h * pH);
    }

    public void setSizeRatio(float ratio)
    {
         nWidth = (int) ((float) nWidth * ratio);
         nHeight = (int) ((float) nHeight * ratio);
    }


    public Rectangle getBounds()
    {
        return (new Rectangle(nX, nY, nWidth, nHeight));
    }

    public void setBounds(Rectangle rectangle)
    {
        setBounds(rectangle.x, rectangle.y, rectangle.width, rectangle.height);
    }

    public void setBounds(int x, int y, int width, int height)
    {
        nX = x;
        nY = y;
        nWidth = width;
        nHeight = height;
    }

    public int getId()
    {
        return id;
    }

    public void setId(int n)
    {
        id = n;
    }

    public int getFrameId()
    {
        return frameId;
    }

    public void setFrameId(int n)
    {
        frameId = n;
    }

    public String getSrcFileName() {
        return srcFile;
    }

    public void setSrcFileName(String s) {
        srcFile = s;
    }

    public void setSrcFileDir(String s) {
        srcDir = s;
    }

    public void setSaveFileDir(String s) {
        saveDir = s;
    }

    public void setSaveFileName(String s) {
        saveFile = s;
    }

    public void setTemplateName(String s) {
        templateName = s;
    }

    // set source data
    //  source data will be converted by Vnmrbg
    public void setSrcData(String s) {
        srcData = s;
    }

    public String getSrcData() {
        return srcData;
    }

    public void readSourceData(String f) {
        if (f == null)
           return;
        String path = FileUtil.openPath(f);
        if (path == null)
           return;
        String d;
        int  lineNo = 0;
        StringBuffer sb = new StringBuffer();
        BufferedReader fin = null;
        try {
            fin = new BufferedReader(new FileReader(path));
            while ((d = fin.readLine()) != null) {
                if (lineNo > 0)
                    sb.append("\n");
                sb.append(d);
                lineNo++;
            }
        }
        catch(IOException e) { }
        if (fin != null) {
            try {
                 fin.close();
            } catch(IOException ee) { }
        }
        srcData = sb.toString();
    }

    public void readSource() {
        if (srcFile == null || srcFile.length() < 1)
           return;
        if (srcFile.equals("text"))
           return;
        StringBuffer sb = new StringBuffer();
        if (srcDir != null) {
           sb.append(srcDir);
           if (srcDir.endsWith(File.separator))
              sb.append(srcFile);
           else
              sb.append(File.separator).append(srcFile);
        }
        else
           sb.append(srcFile);

        readSourceData(sb.toString());
    }

    public void update()
    {
        if (vnmrIf == null || srcFile == null)
            return;
        // if (bTextFrame)
        //    return;
        String s;
        String path = null;
        if (srcFile.equals("text")) {
            if (textFile == null) {
                s = new StringBuffer().append("textbox('update',")
                     .append(id).append(",'text')").toString();
                vnmrIf.sendToVnmr(s);
                return;
            }
            path = textFile;
        }
        else {
            if (saveDir == null)
                saveDir = TextboxEditor.tmpPath;
            if (saveDir.endsWith(File.separator))
                path = new StringBuffer().append(saveDir).append(srcFile).toString();
            else
                path = new StringBuffer().append(saveDir).append(File.separator).append(srcFile).toString();

            path = FileUtil.savePath(path);
            if (path == null)
                 return;
         }
         if (srcData == null)
             return;
         PrintWriter os = null;
         try {
             os = new PrintWriter( new OutputStreamWriter(
                  new FileOutputStream(path), "UTF-8"));
         }
         catch(IOException er) { }
         if (os == null)
              return;
         os.print(srcData);
         os.close();
         if (srcFile.equals("text")) {
             s = new StringBuffer().append("textbox('update',")
                     .append(id).append(",'text')").toString();
         }
         else {
             String file = UtilB.windowsPathToUnix(path);
             s = new StringBuffer().append("textbox('update',")
                 .append(id).append(",'").append(file).append("')").toString();
         }
         vnmrIf.sendToVnmr(s);
    }

    public void readValue(String f, boolean bRemove)
    {
        if (f == null)
            return;
        if (textVec == null)
            textVec = new Vector();
        File fd = new File(f);
        if (fd == null || !fd.exists()) {
            f = UtilB.unixPathToWindows(f);
            fd = new File(f);
            if (fd == null || !fd.exists())
                return;
        }
        textVec.clear();
        longStr = null;
        BufferedReader in;
        try {
            in = new BufferedReader(new FileReader(f));
        } catch(FileNotFoundException e) {
            return;
        }
        int len = 0;
        try {
           String  s = in.readLine();
           longStr = s;
           while (s != null) {
               textVec.addElement(s);
               if (s.length() > 0) {
                   int lw = fm.stringWidth(s);
                   if (lw > len) {
                      longStr = s;
                      len = lw;
                   }
               }
               s = in.readLine();
           }
           in.close();
        } catch(IOException ee) { }

        if (bRemove)
           fd.delete();
    }

    public void setValue(String f1, String f2, boolean bRemove)
    {
        if (fm == null)
            setFontInfo("Plain", "Variable");
        readValue(f1, bRemove);

        if (bTextFrame) {
            if (f2 != null)
                readSourceData(f2);
            else
                readSourceData(f1);
        }
        else {
            if (srcFile.equals("text") && f2 != null) {
               readSourceData(f2);
               textFile = f2;
              //  srcFile = "tmpText";
            }
        }
        if (longStr == null)
            return;
        if (bFixedFont) {
           int len = fm.stringWidth(longStr);
           int w = len + 4;
           int h = fontH * textVec.size() + 4;
           if (nWidth < w)
               nWidth = w;
           if (nHeight < h)
               nHeight = h;
        }
        else {
           if (adjustFont(nWidth - 4, nHeight - 4)) {
               font = newFont;
               fm = newFm;
               fontAscent = fm.getAscent();
               fontDescent = fm.getDescent();
               fontH = fontAscent + fontDescent;
               fontFs =  newFontFs;
           }
        }
    }

    // frome textBox command
    public void setValue(String f1, String f2)
    {
        setValue(f1, f2, true);
    }

    public void setRatio(int w, int h, boolean doAdjust) {
        nPw = w;
        nPh = h;
        if (nWidth < 1 || nHeight < 1)
            return;
        if (doAdjust) { // container size was changed, so it needs to change position
            adjustXY(w, h);
            return;
        }
 
        rx = (float) nX / (float) w;
        ry = (float) nY / (float) h;
        rw = (float) (nWidth) / (float) w;
        rh = (float) (nHeight) / (float) h;
        rx2 = (float) (w - (nX + nWidth)) / (float) w;
        ry2 = (float) (h - (nY + nHeight)) / (float) h;
        if (rx2 < 0)  rx2 = 0;
        if (ry2 < 0)  ry2 = 0;
    }

    public void adjustXY(int w, int h) {
         if (rx <= rx2)
             nX = (int) (rx * (float) w);
         else
             nX = w - ((int) (rx2 * (float) w)) - nWidth;
         if (ry <= ry2)
             nY = (int) (ry * (float) h);
         else
             nY = h - ((int) (ry2 * (float) h)) - nHeight;
         if (nX < 0)
             nX = 0;
         if (nY < 0)
             nY = 0;
    }

    public boolean isAvailable() {
         return bAvailable;
    }

    public void clear() {
         srcFile = null;
         srcData = null;
         fontName = null;
         fontSizeStr = null;
         fontStyleStr = null;
         newFontName = null;
         newFtSizeStr = null;
         newFtStyleStr = null;
         templateName = null;
         textFile = null;
         colorStr = null;
         longStr = null;
         bAvailable = false;
         bSelected = false;
         bVisible = false;
         fm = null;
         newFm = null;
         font = null;
         newFont = null;
         if (textVec != null)
             textVec.clear();
         textVec = null;
         TextboxEditor.updateEditMode();
         if (!bTextFrame || frameId < 1) {
             vnmrIf = null;
             return;
         }
         if (vnmrIf == null)
             return;
         String mess = new StringBuffer().append("framecmd('delete', ").append(
             frameId).append(")").toString();
         vnmrIf.sendToVnmr(mess);
         vnmrIf = null;
    }
   

    public void setColorName(String s) {
         colorStr = s;
         txColor = DisplayOptions.getColor(s);
    }

    public String getColorName() {
         return colorStr;
    }

    private boolean adjustFont(int width, int height) {
         if (longStr == null)
             return false;
         if (fm == null) {
             setFontInfo("Plain", "Variable");
         }
         newFm = fm;
         newFont = font;
         int lineNum = textVec.size();
         int lw = fm.stringWidth(longStr); 
         int lh = fontH * lineNum;
         if (bNoSize) {
             nWidth = lw + 6;
             nHeight = lh + 6;
             bNoSize = false;
             if (nWidth < 20)
                nWidth = 20;
             if (nHeight < 16)
                nHeight = 16;
             return false;
         }
         int  dw = width - lw; 
         int  dh = height - lh; 
         int  sw = 0;
         int  th = 0;
         int  sLen = 0;
         float fh1, fh2, fh3;
         float fw, fh;
 
         newFontFs = fontFs;
         th = fm.getAscent() + fm.getDescent();
         sLen = longStr.length();
         Toolkit tool = Toolkit.getDefaultToolkit();
         if (dw > 10 && dh > lineNum) { // increases font size
             sw = lw;
             fw = (float) width;
             while (dw > 10 && dh >= lineNum) {
                  fh1 = newFontFs * (fw / (float)sw - 1.0f) - 2.0f;
                  fh2 = (float) (dh / lineNum);
                  if (fh1 > fh2)
                       fh1 = fh2;
                  if (fh1 > 1.0f)
                       newFontFs += fh1;
                  else {
                       newFontFs += 1.0f;
                       if (dw >= sLen)
                           newFontFs += 1.0f;
                       if (dw >= sLen*2)
                           newFontFs += 2.0f;
                  }
                  newFont = font.deriveFont(newFontFs);
                  newFm = tool.getFontMetrics(newFont);
                  sw = newFm.stringWidth(longStr);
                  dw = width - sw;
                  th = newFm.getAscent() + newFm.getDescent();
                  dh = height - th * lineNum;
             }
         }
         if (dw < 0 || dh < 0) {
             if (dw < 0)
                 dw = 0 - dw;
             if (dh < 0)
                 dh = 0 - dh;
             fh = 1;
             fh2 = 1;
             if (dh > lineNum) {
                  lh = dh / (lineNum + 1);
                  fh = (float) lh;
                  if (fh < 1)
                      fh = 1;
                  if (fh > 4.0f)
                      fh = 4.0f;

             }
             if (dw > 1) {
                  fh2 = (float) dw / (float) sLen;
                  if (fh2 < 1)
                      fh2 = 1;
                  if (fh2 > 4.0f)
                      fh2 = 4.0f;
             }
             if (fh > fh2)
                  newFontFs = newFontFs - fh2;
             else
                  newFontFs = newFontFs - fh;
             dw = 1;
             dh = 1;
             while (dw > 0 || dh > 0) {
                  if (newFontFs < 6.0f)
                      break;
                  newFontFs -= 1.0f;
                  if (dw > sLen)
                      newFontFs -= 1.0f;
                  if (dw >= sLen * 2)
                      newFontFs -= 2.0f;
                  newFont = font.deriveFont(newFontFs);
                  newFm = tool.getFontMetrics(newFont);
                  sw = newFm.stringWidth(longStr);
                  dw = sw - width;
                  th = newFm.getAscent() + newFm.getDescent();
                  dh = th * lineNum - height;
             }
        }
        return true;
    }

    private boolean adjustFixedFont(float fw, float fh) {
        if (longStr == null || fm == null)
             return false;
        int charH = fontH;
        int count = 0;
        int lineNum = textVec.size();
        float orgW = (float)fm.stringWidth(longStr);
        float orgH = (float) fontH * lineNum;
        float fs = fontFs;
        float newW, newH;
        float difW, difH;
        Toolkit tool = Toolkit.getDefaultToolkit();
        newFont = font;
        newFm = fm;
        if (fw < fh)
             difW = fh;
        else
             difW = fw;
        while (difW > 0.02f || difW < -0.02f) {
             if (difW > 0.02f) {
                 fs += 1.0f;
                 if (difW > 0.4f)
                     fs += 1.0f;
             }
             else {
                 if (difW < 0.1f)
                      break;
                 fs -= 1.0f;
                 if (difW < -0.4f)
                     fs -= 1.0f;
             } 
             newFont = font.deriveFont(fs);
             newFm = tool.getFontMetrics(newFont);
             newW = (float) newFm.stringWidth(longStr);
             charH = newFm.getAscent() + newFm.getDescent();
             newH = (float) charH * lineNum;
             difW = fw - newW / orgW;
             difH = fh - newH / orgH;
             if (difW < difH)
                  difW = difH;
             count++;
             if (count > 5)
                 break;
        }
        return true;
    }

    public void setFontStyle(String s) {
        if (s != null)
           newFtStyleStr = s;
    }

    public String getFontStyle() {
          if (fontStyleStr != null)
              return fontStyleStr;
          return "Plain";
    }

    public void setFontSize(String s) {
        if (s != null)
           newFtSizeStr = s;
    }

    public String getFontSize() {
          if (fontSizeStr != null)
              return  fontSizeStr;
          return  "16";
    }

    public void setFontName(String s) {
        if (s != null)
           newFontName = s;
    }

    public String getFontName() {
        return fontName;
    }

    public void setupFont() {
         if (newFontName == null)
             newFontName = Font.DIALOG;
         if (newFtStyleStr == null)
             newFtStyleStr = "Plain";
         if (newFtSizeStr == null)
             newFtSizeStr = "12";

         boolean bNewFont = false;
         String sizeStr = newFtSizeStr;
         int fs = 0;

         if (!newFontName.equals(fontName)) {
             bNewFont = true;
             fontName = newFontName;
         }
         if (!newFtStyleStr.equals(fontStyleStr)) {
             bNewFont = true;
             fontStyleStr = newFtStyleStr;
         }
         if (!newFtSizeStr.equals(fontSizeStr)) {
             bNewFont = true;
             fontSizeStr = newFtSizeStr;
         }
         if (!bNewFont) {
             if (fm != null)
                return;
         }
 
         if (fontSizeStr.equalsIgnoreCase("variable")) {
              bFixedFont = false;
              fontSizeStr = "Variable";
              sizeStr = "16";
         }
         else {
              try {
                   fs = Integer.parseInt(fontSizeStr);
              }
              catch (NumberFormatException er) { }
         }
         
         if (fs > 1) {
              bFixedFont = true;
         }
         else {
              bFixedFont = false;
              fs = 16;
         }
         // font = DisplayOptions.getFont(fontName, fontStyleStr, sizeStr);
         int style = Util.fontStyle(fontStyleStr);
         int size = 12;
         try {
              size = Integer.parseInt(sizeStr);
         }
         catch (NumberFormatException e) {}

         font = new Font(fontName, style ,size);

         fm = Toolkit.getDefaultToolkit().getFontMetrics(font);
         fontAscent = fm.getAscent();
         fontDescent = fm.getDescent();
         fontH = fontAscent + fontDescent;
         fontFs = (float) fs;
    }

    public void setFontInfo(String name, String style, String size) {
         if (name != null)
             newFontName = name;
         if (style != null)
             newFtStyleStr = style;
         if (size != null)
             newFtSizeStr = size;
         setupFont();
    }

    public void setFontInfo(String style, String size) {
         setFontInfo(Font.DIALOG, style, size);
    }

    public void setSize(int w, int h) {
         if (w <= 0 || h <= 0)
             return;
         bNoSize = false;
         if (nWidth == w && nHeight == h)
             return;
         nWidth = w;
         nHeight = h;
         if (fm == null)
             return;
         if (bFixedFont) {
             if (longStr != null) {
                  w = fm.stringWidth(longStr) + 4;
                  h = fontH * textVec.size() + 4;
                  if (nWidth < w)
                       nWidth = w;
                  if (nHeight < h)
                       nHeight = h;
             }
         }
         else {
             if (adjustFont(nWidth - 4, nHeight - 4)) {
                  font = newFont;
                  fm = newFm;
                  fontAscent = fm.getAscent();
                  fontDescent = fm.getDescent();
                  fontH = fontAscent + fontDescent;
                  fontFs =  newFontFs;
             }
         }
    }

    public void setVisible(boolean b) {
         bVisible = b;
         if (!b) {
             bSelected = b;
             TextboxEditor.updateEditMode();
         }
    }

    public boolean isVisible() {
         return bVisible;
    }

    public void setSelected(boolean b) {
         boolean bOldSelected = bSelected;
         bSelected = b;
         if (b) {
            if (b != bOldSelected) {
                if (isActive())
                    TextboxEditor.setEditObj(this);
            }
         }
         else
            TextboxEditor.updateEditMode();
    }

    public boolean isSelected() {
         return bSelected;
    }

    public boolean isResizable() {
         if (bFixedFont)
             return false;
         return true;
    }

    public boolean isActive() {
         if (vnmrIf == null || !bSelected)
         {
            bSelected = false;
            return false;
         }
         if (vnmrIf instanceof ExpPanel)
             return ((ExpPanel) vnmrIf).isInActive();
         else
             return true;
    }

    public void delete() {
         if (!bSelected)
            return;
         if (vnmrIf != null)
            vnmrIf.sendToVnmr("vnmrjcmd('canvas textBox delete')");
    }

    public void  paint(Graphics2D g, Color hcolor, boolean bBorder) {
        if (!bVisible || textVec == null)
           return;
        g.setColor(txColor);
        if (font != null)
           g.setFont(font);
        if (nY < 0)
           nY = 0;
        // g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON); 
        g.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_LCD_HBGR); 
        int n = textVec.size();
        int y = nY + fontAscent + 2;
        for (int i = 0; i < n; i++) {
           String s = (String)textVec.elementAt(i);
           g.drawString(s, nX + 1, y);
           y += fontH;
        }
        if (!bSelected)
           return;
        g.setColor(hcolor);
        if (bBorder) {
            g.drawRect(nX, nY, nWidth-1, nHeight-1);
            return;
        }
        int y2 = nY + nHeight;
        int x2 = nX + nWidth;
        g.drawLine( nX, nY, nX + 8, nY);
        g.drawLine( nX, nY, nX, nY + 8);
        g.drawLine( nX, y2, nX + 8, y2);
        g.drawLine( nX, y2 - 8 , nX, y2);
        g.drawLine( x2 - 8, nY, x2, nY);
        g.drawLine( x2, nY, x2, nY + 8);
        g.drawLine( x2 - 8, y2, x2, y2);
        g.drawLine( x2, y2 - 8, x2, y2);
    }

    // pw and ph are the width and height of print canvas
    public void  print(Graphics2D g, boolean bColor, int pw, int ph) {
        if (!bVisible || textVec == null || longStr == null)
           return;
        if (nWidth < 2 || nHeight < 2)
           return;
        int lines = textVec.size();
        if (lines < 1)
           return;
        if (bColor)
            g.setColor(txColor);
        else
            g.setColor(Color.black);
        Font orgFont = g.getFont();
        if (nPw < nWidth)
            nPw = nWidth;
        if (nPh < nHeight)
            nPw = nHeight;
        float f1 = (float)nWidth /(float) nPw;
        float f2 = (float)nHeight / (float) nPh;
        float orgFs = fontFs;
        int x, y, ch;
        int w, h, dx, dy;
        w = (int) ((float) pw * f1);
        h = (int) ((float) ph * f2);
        if (rx <= rx2)
             x = (int) (rx * (float) pw);
        else
             x = pw - ((int) (rx2 * (float) pw)) - w;
        if (ry <= ry2)
             y = (int) (ry * (float) ph);
        else
             y = ph - ((int) (ry2 * (float) ph)) - h;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        f1 = (float)pw /(float) nPw;
        f2 = (float)ph / (float) nPh;
        dx = (int) (f1 * 4.0f);
        dy = (int) (f2 * 4.0f);
        if (f1 < f2)
             fontFs = orgFs * f1;
        else
             fontFs = orgFs * f2;
        if (bFixedFont)
             adjustFixedFont(f1, f2);
        else
             adjustFont(w - dx, h - dy);
        g.setFont(newFont);
        // g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON); 
        g.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_LCD_HBGR); 
        fontFs = orgFs;
        ch = newFm.getAscent() + newFm.getDescent();
        dy = (int) (f2 * 2.0f);
        dx = (int) (f1 * 1.0f);
        y = y + newFm.getAscent() + dy;
        x += dx;
        for (int i = 0; i < lines; i++) {
           String s = (String)textVec.elementAt(i);
           g.drawString(s, x, y);
           y += ch;
        }
        g.setFont(orgFont);
    }
}
