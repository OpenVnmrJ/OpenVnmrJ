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

import vnmr.vplaf.jtattoo.AbstractToolBarUI;
import java.awt.Graphics;
import javax.swing.JComponent;
import javax.swing.border.Border;
import javax.swing.plaf.ComponentUI;

/**
 * @author Michael Hagen
 */
public class TextureToolBarUI extends AbstractToolBarUI
{
   public static ComponentUI createUI(JComponent c) {
       return new TextureToolBarUI();
   }

   public Border getRolloverBorder() {
       return TextureBorders.getRolloverToolButtonBorder();
   }

   public Border getNonRolloverBorder() {
       return TextureBorders.getToolButtonBorder();
   }

   public boolean isButtonOpaque() {
       return false;
   }

    public void paint(Graphics g, JComponent c) {
        TextureUtils.fillComponent(g, c, TextureUtils.getTextureType(c));
    }

}

