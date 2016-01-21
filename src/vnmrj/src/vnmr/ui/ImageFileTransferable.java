/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
 * CanvasTransferable.java
 *
 */

package vnmr.ui;

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;
import java.awt.datatransfer.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.util.*;

/**
 * Used to Copy an Image in a file to the clipboard.
 */
public class ImageFileTransferable
        extends TransferHandler
        implements Transferable {

    protected String m_filepath;


    /** Creates a new instance of ImageFileTransferable */
    public ImageFileTransferable(String filepath)
    {
        m_filepath = filepath;
    }

    public int getSourceActions(JComponent c) {
        return TransferHandler.COPY;
    }

    public Transferable createTransferable(JComponent comp)
    {
        return this;
    }

    public Object getTransferData(DataFlavor flavor)
    {
        File file = new File(m_filepath);
        Image image = null;
        try
        {
            image = javax.imageio.ImageIO.read(file);
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.postError("Error reading file: " + m_filepath);
            Messages.writeStackTrace(e);
        }

        // delete the file when vnmrj exits
        file.deleteOnExit();

        return image;
    }

    public DataFlavor [] getTransferDataFlavors()
    {
        //return new DataFlavor[]{DataFlavor.javaFileListFlavor};
        return new DataFlavor[]{DataFlavor.imageFlavor};
    }

    public boolean isDataFlavorSupported(DataFlavor flavor)
    {
        //return flavor.equals(DataFlavor.javaFileListFlavor);
        return flavor.equals(DataFlavor.imageFlavor);
    }

}
