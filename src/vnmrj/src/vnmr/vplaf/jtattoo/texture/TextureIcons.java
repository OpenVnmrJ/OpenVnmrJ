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

import vnmr.vplaf.jtattoo.*;
import java.awt.Color;
import java.awt.Insets;
import javax.swing.Icon;

/**
 * @author Michael Hagen
 */
public class TextureIcons extends BaseIcons {

    public static void setUp() {
        iconIcon = null;
        maxIcon = null;
        minIcon = null;
        closeIcon = null;
        splitterHorBumpIcon = null;
        splitterVerBumpIcon = null;
        thumbHorIcon = null;
        thumbVerIcon = null;
        thumbHorIconRollover = null;
        thumbVerIconRollover = null;
    }

    public static Icon getIconIcon() {
        if (iconIcon == null) {
            Color iconColor = AbstractLookAndFeel.getTheme().getWindowIconColor();
            Color iconShadowColor = AbstractLookAndFeel.getTheme().getWindowIconShadowColor();
            Color iconRolloverColor = AbstractLookAndFeel.getTheme().getWindowIconRolloverColor();
            iconIcon = new BaseIcons.IconSymbol(iconColor, iconShadowColor, iconRolloverColor, new Insets(1, 1, 1, 1));
        }
        return iconIcon;
    }

    public static Icon getMinIcon() {
        if (minIcon == null) {
            Color iconColor = AbstractLookAndFeel.getTheme().getWindowIconColor();
            Color iconShadowColor = AbstractLookAndFeel.getTheme().getWindowIconShadowColor();
            Color iconRolloverColor = AbstractLookAndFeel.getTheme().getWindowIconRolloverColor();
            minIcon = new BaseIcons.MinSymbol(iconColor, iconShadowColor, iconRolloverColor, new Insets(1, 1, 1, 1));
        }
        return minIcon;
    }

    public static Icon getMaxIcon() {
        if (maxIcon == null) {
            Color iconColor = AbstractLookAndFeel.getTheme().getWindowIconColor();
            Color iconShadowColor = AbstractLookAndFeel.getTheme().getWindowIconShadowColor();
            Color iconRolloverColor = AbstractLookAndFeel.getTheme().getWindowIconRolloverColor();
            maxIcon = new BaseIcons.MaxSymbol(iconColor, iconShadowColor, iconRolloverColor, new Insets(1, 1, 1, 1));
        }
        return maxIcon;
    }

    public static Icon getCloseIcon() {
        if (closeIcon == null) {
            Color iconColor = AbstractLookAndFeel.getTheme().getWindowIconColor();
            Color iconShadowColor = AbstractLookAndFeel.getTheme().getWindowIconShadowColor();
            Color iconRolloverColor = AbstractLookAndFeel.getTheme().getWindowIconRolloverColor();
            closeIcon = new BaseIcons.CloseSymbol(iconColor, iconShadowColor, iconRolloverColor, new Insets(1, 1, 1, 1));
        }
        return closeIcon;
    }

    public static Icon getSplitterHorBumpIcon() {
        if (splitterHorBumpIcon == null) {
            splitterHorBumpIcon = new LazyImageIcon("texture/icons/SplitterHorBumps.gif");
        }
        return splitterHorBumpIcon;
    }

    public static Icon getSplitterVerBumpIcon() {
        if (splitterVerBumpIcon == null) {
            splitterVerBumpIcon = new LazyImageIcon("texture/icons/SplitterVerBumps.gif");
        }
        return splitterVerBumpIcon;
    }

    public static Icon getThumbHorIcon() {
        if ("Default".equals(AbstractLookAndFeel.getTheme().getName())) {
            if (thumbHorIcon == null) {
                thumbHorIcon = new LazyImageIcon("texture/icons/thumb_hor.gif");
            }
            return thumbHorIcon;
        } else {
            return BaseIcons.getThumbHorIcon();
        }
    }

    public static Icon getThumbVerIcon() {
        if ("Default".equals(AbstractLookAndFeel.getTheme().getName())) {
            if (thumbVerIcon == null) {
                thumbVerIcon = new LazyImageIcon("texture/icons/thumb_ver.gif");
            }
            return thumbVerIcon;
        } else {
            return BaseIcons.getThumbVerIcon();
        }
    }

    public static Icon getThumbHorIconRollover() {
        if ("Default".equals(AbstractLookAndFeel.getTheme().getName())) {
            if (thumbHorIconRollover == null) {
                thumbHorIconRollover = new LazyImageIcon("texture/icons/thumb_hor_rollover.gif");
            }
            return thumbHorIconRollover;
        } else {
            return BaseIcons.getThumbHorIconRollover();
        }
    }

    public static Icon getThumbVerIconRollover() {
        if ("Default".equals(AbstractLookAndFeel.getTheme().getName())) {
            if (thumbVerIconRollover == null) {
                thumbVerIconRollover = new LazyImageIcon("texture/icons/thumb_ver_rollover.gif");
            }
            return thumbVerIconRollover;
        } else {
            return BaseIcons.getThumbVerIconRollover();
        }
    }
}
