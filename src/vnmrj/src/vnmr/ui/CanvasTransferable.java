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
 *
 * @author Mamta
 */
public class CanvasTransferable extends TransferHandler implements Transferable 
{
    
    protected CanvasIF m_canvas;
    protected Rectangle m_rectSelect;
    protected String m_strPrevPath;
            
    
    /** Creates a new instance of CanvasTransferable */
    public CanvasTransferable(CanvasIF canvas) 
    {
        this(canvas, null);
    }
    
    public CanvasTransferable(CanvasIF canvas, Rectangle rect)
    {
        m_canvas = canvas;
        m_rectSelect = rect;
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
        String strWriteFile = new StringBuffer().append("USER/PERSISTENCE/imageFile").
                                  append(System.currentTimeMillis()).append(".jpeg").toString();
        strWriteFile = FileUtil.savePath(strWriteFile);

        Dimension dim = m_canvas.getWinSize();
        Point point = ((JComponent)m_canvas).getLocationOnScreen();
        if (m_rectSelect != null)
        {
            dim = m_rectSelect.getSize();
            --dim.height;
            --dim.width;
            Rectangle rect = m_rectSelect.getBounds();
            point.x = point.x + rect.x + 1;
            point.y = point.y + rect.y + 1;
        }
        
        //remove the previous file
        if (m_strPrevPath != null)
        {
            WUtil.deleteFile(m_strPrevPath);
        }
        
        // capture the image
        boolean bOk = CaptureImage.doCapture(strWriteFile, dim, point);
        File file = new File(strWriteFile);
        m_strPrevPath = strWriteFile;
        Image image = null;
        try
        {
            image = javax.imageio.ImageIO.read(file);
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.postError("Error copying selected area");
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
