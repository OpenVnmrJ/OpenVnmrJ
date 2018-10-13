/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * Copyright 2012 MH-Software-Entwicklung. All rights reserved.
 * Use is subject to license terms.
 */

package vnmr.vplaf.jtattoo.texture;

import vnmr.vplaf.jtattoo.BaseInternalFrameUI;
import javax.swing.JComponent;
import javax.swing.JInternalFrame;
import javax.swing.plaf.ComponentUI;

/**
 * @author Michael Hagen
 */
public class TextureInternalFrameUI extends BaseInternalFrameUI {

    public TextureInternalFrameUI(JInternalFrame b) {
        super(b); 
    }
    
    public static ComponentUI createUI(JComponent c) {
        return new TextureInternalFrameUI((JInternalFrame)c);
    }
    
    protected JComponent createNorthPane(JInternalFrame w)  {
        titlePane = new TextureInternalFrameTitlePane(w);
        return titlePane;
    }
    
}

