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

import vnmr.vplaf.jtattoo.BaseRootPaneUI;
import vnmr.vplaf.jtattoo.BaseTitlePane;
import javax.swing.JComponent;
import javax.swing.JRootPane;
import javax.swing.plaf.ComponentUI;

/**
 * @author  Michael Hagen
 */
public class TextureRootPaneUI extends BaseRootPaneUI
{
   public static ComponentUI createUI(JComponent c) { 
       return new TextureRootPaneUI();
   }
   
   public BaseTitlePane createTitlePane(JRootPane root) { 
       return new TextureTitlePane(root, this);
   }   
}
