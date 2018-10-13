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

import vnmr.vplaf.jtattoo.LazyImageIcon;
import vnmr.vplaf.jtattoo.XPScrollButton;
import javax.swing.Icon;

/**
 * @author  Michael Hagen
 */
public class TextureScrollButton extends XPScrollButton {

    protected static Icon upArrowIcon = null;
    protected static Icon downArrowIcon = null;
    protected static Icon leftArrowIcon = null;
    protected static Icon rightArrowIcon = null;

    public TextureScrollButton(int direction, int width) {
        super(direction, width);
    }

    public Icon getUpArrowIcon() {
        if (upArrowIcon == null) {
            upArrowIcon = new LazyImageIcon("texture/icons/UpArrow.gif");
        }
        return upArrowIcon;
    }

    public Icon getDownArrowIcon() {
        if (downArrowIcon == null) {
            downArrowIcon = new LazyImageIcon("texture/icons/DownArrow.gif");
        }
        return downArrowIcon;
    }

    public Icon getLeftArrowIcon() {
        if (leftArrowIcon == null) {
            leftArrowIcon = new LazyImageIcon("texture/icons/LeftArrow.gif");
        }
        return leftArrowIcon;
    }

    public Icon getRightArrowIcon() {
        if (rightArrowIcon == null) {
            rightArrowIcon = new LazyImageIcon("texture/icons/RightArrow.gif");
        }
        return rightArrowIcon;
    }

}
