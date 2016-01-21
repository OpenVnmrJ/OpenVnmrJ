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
// import java.util.*;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.text.DecimalFormat;

import vnmr.ui.*;

/**
 * This program takes a list of images and convert them into
 * a QuickTime movie.
 */

public class ImagesToJpeg {
    public static ImagesToJpeg imagesToJpeg = null;

    int nextImage = 0;  // index of the next image to be read.
    int numImages = 100;
    int width = 100;
    int height = 100;
    int frameRate = 5;
    private String moviePath;
    private String tmpPath;
    Dimension dimCanvas;
    Point pointCanvas;
    private BufferedImage bufImg;
    private Graphics bufGc;
    DecimalFormat formatter = new DecimalFormat("0000");

    public static ImagesToJpeg get() {
        if(imagesToJpeg == null) {
           imagesToJpeg = new ImagesToJpeg();
        }
        return imagesToJpeg;
    }

     public void start(String path, int w, int h, int nimages, int rate) {
        numImages = nimages;
        width=w;
        height=h;
        frameRate=rate;
        moviePath=path;
        nextImage = 0;  // index of the next image to be read.

        if (width < 0) width=100;
        if (height < 0) height=100;
        if (frameRate < 1) frameRate = 1;

         // Get the size and the location for the image to be captured
        VnmrCanvas vcanvas = Util.getActiveView().getCanvas();
        dimCanvas = new Dimension(width,height);
        pointCanvas = vcanvas.getLocationOnScreen();
        getImagePath(1);

        Util.sendToVnmr("aipMovie('next')");
    }

    public void next() {
        nextImage++;
        if(nextImage > numImages) {
           Util.sendToVnmr("aipMovie('stop')");
	   return;
        }
	String strFile = getImagePath(nextImage);
        // capture the image
        CaptureImage.doCapture(strFile, dimCanvas, pointCanvas);

        Util.sendToVnmr("aipMovie('next')");
    }

    public void next(BufferedImage img, Color bg) {
        nextImage++;
        if(nextImage > numImages) {
           Util.sendToVnmr("aipMovie('stop')");
	   return;
        }
        if (bufImg == null || bufImg.getWidth() != width
                   || bufImg.getHeight() != height) {
            bufImg = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);
            bufGc = bufImg.createGraphics();
        }
        bufGc.setColor(bg);
        bufGc.fillRect(0, 0, width, height);
        bufGc.drawImage(img, 0, 0, width, height, 0, 0, width, height, null);
	String strFile = getImagePath(nextImage);
        CaptureImage.write(strFile, bufImg);

        Util.sendToVnmr("aipMovie('next')");
    }

    public void done() {

        numImages = nextImage;
        if (nextImage < 1)
           return;
        if (!moviePath.endsWith(".mov") && !moviePath.endsWith(".MOV")) {
           moviePath = moviePath + ".mov";
           Util.sendToVnmr("aipMoviePath='"+moviePath+"'");
        }
        Util.sendToVnmr("simpleMovie('make','"+moviePath+"',"+numImages+","+width+","+height+","+frameRate+","+"'"+tmpPath+"'"+")");
    }

    private String getImagePath(int id) {
         if (tmpPath == null) {
             tmpPath = FileUtil.savePath("USER/PERSISTENCE/tmp/jmfframe000x1.jpeg");
             if (tmpPath != null) {
                 File file = new File(tmpPath);
                 tmpPath = file.getParent();
             }
             if (tmpPath == null)
                 tmpPath = "/tmp";
         }
         // return "/tmp/jmfframe"+formatter.format(id)+".jpeg";

         String path = tmpPath+"/jmfframe"+formatter.format(id)+".jpeg";
         return path;
    }
}
