/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.MediaTracker;
import java.awt.image.BufferedImage;
import javax.swing.JPanel;

import vnmr.admin.ui.*;
import vnmr.admin.util.*;


/**
 * Static methods that operate on images.
 *
 */
public class ImageUtil {
    public static final String[] imageForamts = {"bmp", "eps", "gif", "jpg",
           "jpeg", "pcl", "pdf", "png", "ppm", "ps", "svg", "tif"};

    public static final String[] gsDevices = {"bmp256","ps2write","null","jpeg",
           "jpeg", "cljet5c","pdfwrite","png16m","ppm","ps2write","svg","tiff24nc"};

	/**
	 * Given an Image, returns another Image that is a copy rotated
	 * 90 degrees clockwise.
	 * @param inputImage The Image to be rotated.
	 * @return The rotated Image.
	 */
    static public Image rotateImage(Image inputImage) {
		JPanel c = new JPanel();
		
		// Make sure image is complete
        try {
            MediaTracker tracker = new MediaTracker(c);
            tracker.addImage(inputImage, 0);
            tracker.waitForID(0);
        } catch ( Exception e ) {}
        
        // Copy image into an integer array
		int width = inputImage.getWidth(c);
		int height = inputImage.getHeight(c);
        BufferedImage refBufImg = new BufferedImage(width, height,
                			BufferedImage.TYPE_INT_ARGB_PRE);
        Graphics2D big = refBufImg.createGraphics();
        big.drawImage(inputImage, 0, 0, c);
        int[] inputArray = refBufImg.getRGB(0, 0, width, height,
      					null, 0, width);
        
        // Copy to output array with rotated orientation
        int outWidth = height;
        int outHeight = width;
        int len = inputArray.length;
        int[] outputArray = new int[len];
        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
        	int k = height - 1 - j;
        	try {
        		outputArray[i * outWidth + k] = inputArray[j * width + i];
        	} catch (ArrayIndexOutOfBoundsException aioobe) {
                        Messages.postError("ImageUtil.rotateImage: "
                             + "index out of bounds: i=" + i
                             + ", j=" + j + ", k=" + k);
        		break;
                }
            }
        }
        
        // Put the rotated data into a BufferedImage for output
        BufferedImage outputImage = new BufferedImage
        		(outWidth, outHeight, BufferedImage.TYPE_INT_ARGB_PRE);
        outputImage.setRGB(0, 0, outWidth, outHeight, outputArray, 0, outWidth);
		return outputImage;
    }

    static private String convertFile(String inFile, String outFile, String outFormat) {
         if (inFile == null || outFile == null)
             return null;

         String[] cmds = {
               WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION,
               FileUtil.SYS_VNMR + "/bin/convert" + " \"" + inFile + "\" \""
               + outFormat+":"+outFile+ "\""};
         WUtil.runScript(cmds, false);

         return outFile;
    }

    static public String convertPSFile(String inFile, String outFile, String outFormat) {
         String gsFormat = null;
         String formatStr = outFormat.toLowerCase();

         if (inFile == null || outFile == null)
              return null;

         for (int i = 0; i < imageForamts.length; i++) {
             if (formatStr.equals(imageForamts[i])) {
                 gsFormat = gsDevices[i];
                 break;
             }
         }
         if (gsFormat == null || gsFormat.equals("null")) {
             convertFile(inFile, outFile, outFormat);
             return outFile;
         }

         String[] cmds = {
               WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION,
               FileUtil.SYS_VNMR + "/bin/vnmr_gs -dSAFER -dBATCH -dNOPAUSE -sDEVICE="
               + gsFormat + " -sOutputFile="+outFile +" -q "+inFile };
         if (gsFormat.equals("svg"))
             WUtil.runScriptNoWait(cmds, false);
         else
             WUtil.runScript(cmds, false);

         return outFile;
    }

    static public String convertImageFile(String inFile, String outFile, String outFormat) {
         String inFormat;
         int n;

         inFormat = "";
         if (outFormat == null || outFormat.length() < 1) {
             Messages.postError("Error: image format was empty.");
             return null;
         }
         if (inFile == null) {
             Messages.postError("Error: image format was empty.");
             return null;
         }
         outFormat = outFormat.toLowerCase();
         n = inFile.lastIndexOf('.');
         if (n > 0)
             inFormat = inFile.substring(n + 1).toLowerCase();
         if (outFile == null || outFile.length() < 1)
             outFile = inFile.substring(0, n + 1)+ outFormat;
         if (inFormat.equals("ps") || inFormat.equals("eps"))
             return convertPSFile(inFile, outFile, outFormat);

         convertFile(inFile, outFile, outFormat);
         return outFile;
    }
}
