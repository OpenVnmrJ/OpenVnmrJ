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
 *
 */

import java.awt.*;
import java.util.*;
import java.io.*;
import javax.swing.*;

public class PlotUtil implements PlotDefines {
   private static ToVnmrSocket  vnmrPort = null;
   private static PlotItemPref  prefWin = null;
   private static PlotEditTool  editTool = null;
   private static TextInput  textInput = null;
   private static boolean  needWait = false;
   private static String  vnmrData = null;
   private static PlotConfig  mainClass = null;
   private static int vid = 0;
   private static int scrnResolution = 0;
   private static int winWidth = 0;
   private static int winHeight = 0;
   private static int waitCount = 0;
   private static int waitTime = 100;
   private static boolean colorPlotter = false;

   public static void setVnmrPort(ToVnmrSocket port) {
        vnmrPort = port;
   }

   public static void setMainClass(PlotConfig c) {
        mainClass = c;
   }

   public static void sendToVnmr(String data) {
        if (vnmrPort != null)
            vnmrPort.send(data);
    }

   public static String jplotMacro(String cmd, String p1, String p2, int orient, int wait ) {
        if (vnmrPort == null)
            return null;
        int  toDo = 0;
        String  plot = "PS_AR";
        if (cmd.equals("load"))
            toDo = 1;
        if (orient == 1)
            plot = "PS_A";
        String d = "jplot('-macro',"+toDo+", 1, '"+p1+"', '"+p2+"', '"+plot+"', "+wait+")\n";

        needWait = true;
        vnmrPort.send(d);
        if (wait < 1) {
            needWait = false;
            return null;
        }
        waitCount = 1;
        waitTime = 100;
        while (needWait) {
            try {
               Thread.currentThread().sleep(waitTime);
               waitCount++;
               if (waitCount > 20)
                   break;
               if (waitCount > 5)
                   waitTime = 400;
            }
            catch (Exception e) {
                return null;
            }
        }
        return vnmrData;
   }

   public static void processData(String str) {
        vnmrData = str;
        if (str.equals("open_jplot")) {
            mainClass.setState(Frame.NORMAL);
            return;
        }
        if (str.equals("exit_jplot")) {
            mainClass.closeFrame();
            return;
        }

        if (needWait) {
           needWait = false;
        }
   }

	public static Image jplotImage(String inf, int orient) {
		String fimg = System.getProperty("user.name") + "jploti" + vid;
		vid++;
		if (vid > 60)
			vid = 0;
		String image_file_unix=PlotConfig.tmpDirUnix + fimg;
		String vret = jplotMacro("load", inf, image_file_unix,
				orient, 1);
		Image img = null;
		if ((vret != null) && !(vret.equals("null"))&& vret.equals(image_file_unix)) {
			String image_file_system=PlotConfig.tmpDirSystem+fimg;
			MediaTracker tracker = new MediaTracker(mainClass);
			// img = Toolkit.getDefaultToolkit().getImage(vret);
			img = Toolkit.getDefaultToolkit().getImage(image_file_system);
			if (img != null) {
				tracker.addImage(img, 0);
				try {
					tracker.waitForID(0);
				} catch (InterruptedException e) {
				}
			}
			File fd = new File(image_file_system);
			if (fd.exists())
				fd.delete();
		}
		return img;
	}

   public static void writeColor(PrintWriter os, Color c) {
	int     k;
        os.print(""+vcommand+" '-draw', 'color', ");
        k = c.getRed();
        Format.print(os, "%.3f, ", (double) k);
        k = c.getGreen();
        Format.print(os, "%.3f, ", (double) k);
        k = c.getBlue();
        Format.print(os, "%.3f", (double) k);
        os.println(" )");
   }

   public static void writeFont(PrintWriter os, String name, int style, int size) {
	os.print(""+vcommand+" '-draw', 'font', ");
        if (name.equals("Serif"))
	    os.print("'Times");
	else {
	    if (name.equals("SansSerif"))
                  os.print("'Helvetica");
            else
                  os.print("'Courier");
        }
	if(style == Font.BOLD)
 	    os.print("-Bold");
	else if(style == Font.ITALIC)
            os.print("-Italic");
        Format.print(os, "', %d, ", size);
        os.print("'"+name+"', ");
        Format.print(os, "%d ", style);
        os.println(" )");
   }

   public static void writeText(PrintWriter os, String data, double x, double y){
	os.print(""+vcommand+" '-draw', 'text', ");
        Format.print(os, "%f, ", x );
        Format.print(os, "%f, ", y);
        os.print("'");
        for (int k = 0; k < data.length(); k++)
	{
 	    if (data.charAt(k) == '\\')
                os.print("\\\\\\");
            else if (data.charAt(k) == '\'')
		os.print("\\");
	    Format.print(os, "%c", data.charAt(k));
        }
        os.println("')");
   }

   public static void setColorPlotter(boolean s) {
	colorPlotter = s;
   }

   public static boolean isColorPlotter() {
	return colorPlotter;
   }

   public static void setWinDimension(int w, int h) {
	winWidth = w;
	winHeight = h;
   }

   public static int getWinWidth() {
	return winWidth;
   }

   public static int getWinHeight() {
	return winHeight;
   }

   public static Color getWinForeground() {
	return mainClass.getWinForeground();
   }

   public static Color getWinBackground() {
	return mainClass.getWinBackground();
   }

   public static Color getHighlightColor() {
	return mainClass.getHighlightColor();
   }

   public static Graphics getGC() {
        return mainClass.getGC();
   }

   public static Color getColor(String c)
   {
        Color color = PlotRgb.getColorByName(c);
        return color;
   }

   public static void backUp() {
        mainClass.backUp();
   }

   public static void restoreArea(Rectangle rect) {
        mainClass.restoreArea(rect);
   }

   public static Image createBuffer(int w, int h) {
        return mainClass.createBuffer(w, h);
   }

   public static void setPrefWindow(PlotItemPref w) {
	prefWin = w;
   }

   public static PlotItemPref getPrefWindow() {
	return prefWin;
   }

   public static void setTextWindow(TextInput w) {
	textInput = w;
   }

   public static TextInput getTextWindow() {
	return textInput;
   }

   public static void setEditTool(PlotEditTool w) {
	editTool = w;
   }

   public static PlotEditTool getEditTool() {
	return editTool;
   }

   public static int getScrnResolution() {
   	if (scrnResolution <= 0) {
	   scrnResolution = Toolkit.getDefaultToolkit().getScreenResolution();
	}
	return scrnResolution;
   }

   public static Image getTextImage(String str, String ftName, int ftStyle, 
		int ftSize, int w, int h, Color c) {
	String pname = System.getProperty("user.name")+"jtext"+vid;
        int r = getScrnResolution();
	char ch;
	w += w * 0.05;
        try
        {
            PrintWriter os = new PrintWriter(new FileWriter(PlotConfig.tmpDirSystem+pname));
            if (os == null)
                return null;
            os.println("%!PS-Adobe-2.0 EPSF-2.0");
            os.println("%%EndComments");
            os.println("%%EndProlog");
            os.println("%%Page 1 1");
            os.println("gsave");
            os.println("0.2400 0.2400 scale");
	    int k = c.getRed();
	    Format.print(os, "%.3f ", (double) k / 255.0);
	    k = c.getGreen();
	    Format.print(os, "%.3f ", (double) k / 255.0);
	    k = c.getBlue();
	    Format.print(os, "%.3f ", (double) k / 255.0);
            os.println("setrgbcolor");
	    if (ftName.equals("Serif"))
               os.print("/Times");
            else
            {
               if (ftName.equals("SansSerif"))
                  os.print("/Helvetica");
               else
                  os.print("/Courier");
            }
	    if (ftStyle == Font.BOLD)
               os.print("-Bold");
            else if(ftStyle == Font.ITALIC)
               os.print("-Italic");
	    double f1 = (double) ftSize * 72.0 / (double) r;
	    int s = (int) (3.880 * f1+0.5) + 3;
            os.println(" findfont "+s+" scalefont setfont");
	    os.println("%%BoundingBox:  0  0 "+w+" "+h);
	    os.print("0 ");
	    Format.print(os, "%d ", s / 4);
	    os.println("moveto");
	    os.print("(");
	    for (k = 0; k < str.length(); k++) {
		ch = str.charAt(k);
		switch (ch) {
		   case '(':
		   case ')':
		   case '\\':
	    		os.print('\\');
			break;
		}
	    	os.print(ch);
	    }
	    os.println(") show");
	    os.println("showpage grestore");
	    os.println("%%Trailer");
	    os.println("%%EOF");
	    os.close();
	}
	catch(IOException er) { return null; }
	String gif = System.getProperty("user.name")+"img"+vid;
	String ppm = System.getProperty("user.name")+"ppm"+vid;
	Image img = null;
	vid++;
	if (vid > 60)
	    vid = 0;
        String d = "jplot('-jtext','"+PlotConfig.tmpDirUnix+pname+"','"+PlotConfig.tmpDirUnix+ppm+"','"+PlotConfig.tmpDirUnix+gif+"','"+
		r+"x"+r+"','"+w+"x"+h+"')\n";

        needWait = true;
        vnmrPort.send(d);
        waitCount = 1;
        waitTime = 100;
        while (needWait) {
            try {
               Thread.currentThread().sleep(waitTime);
               waitCount++;
               if (waitCount > 20)
                    break;
               if (waitCount > 5)
                   waitTime = 400;
            }
            catch (Exception e) {
                return null;
            }
        }
        MediaTracker tracker = new MediaTracker(mainClass);
        img = Toolkit.getDefaultToolkit().getImage(PlotConfig.tmpDirSystem+gif);
	if (img != null) {
             tracker.addImage(img, 0);
             try { tracker.waitForID(0); }
             catch (InterruptedException e) {}
	}
	File fd = new File(PlotConfig.tmpDirSystem+gif);
	if (fd.exists())
	    fd.delete();
	return img;
   }

}

