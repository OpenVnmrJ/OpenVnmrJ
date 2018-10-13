/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.images;

import java.awt.*;
import java.io.*;
import java.net.*;
import javax.swing.*;

import vnmr.util.Messages;

public  class VnmrImageUtil {
        Toolkit tk;

        public VnmrImageUtil() {
            tk = Toolkit.getDefaultToolkit();
        }

        // get image from jar file 
        public ImageIcon getVnmrJIcon(String f) {
           ImageIcon imageIcon = null;
           java.awt.image.ImageProducer I_P;
           URL imageURL;
           Image img = null;
           try {
             imageURL = getClass().getResource(f);
             if (imageURL == null)
                return null; 
             I_P = (java.awt.image.ImageProducer)imageURL.getContent();
             img = tk.createImage(I_P);
           }
           catch (IOException e) {
                return null; 
           }
           if (img == null)
                return null; 
           imageIcon = new ImageIcon(img);
           return imageIcon;
        }

        public Image getVnmrJImage(String f) {
           java.awt.image.ImageProducer I_P;
           URL imageURL = null;
           Image img = null;
           String name = f;
           try {
               for (String extn : vnmr.util.Util.IMG_TYPES) {
                   name = f + extn;
                   imageURL = getClass().getResource(name);
                   if (imageURL != null) {
                       break;
                   }
               }
               //imageURL = getClass().getResource(f);
               if (imageURL == null)
                   return null; 
               I_P = (java.awt.image.ImageProducer)imageURL.getContent();
               img = tk.createImage(I_P);
           }
           catch (IOException e) {
               return null;
           }
           return img;
        }

    /**
     * Open an InputStream for a given image file.
     * Typically, the image file is an entry in a jar file.
     * @param f The name of the image file, including extension.
     * @return The InputStream to read the file, or null.
     */
    public InputStream getVnmrJImageStream(String f) {
        URL imageURL = getClass().getResource(f);
        InputStream stream = null;
        if (imageURL != null) {
            try {
                stream = imageURL.openStream();
            } catch (IOException e) {
                Messages.postDebug("VnmrImageUtil.getVnmrJImageStream(): "
                                   + "Failed to open stream for URL \""
                                   + imageURL + "\", protocol=\""
                                   + imageURL.getProtocol() + "\"");
            }
        }
        return stream;
    }
}
