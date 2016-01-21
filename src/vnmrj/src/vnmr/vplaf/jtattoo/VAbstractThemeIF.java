/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.vplaf.jtattoo;

import java.awt.Color;
import javax.swing.plaf.ColorUIResource;

public interface VAbstractThemeIF {

    public void setDefaultBg(ColorUIResource c);

    public void setDefaultFg(ColorUIResource c);

    public void setDisableBg(ColorUIResource c);

    public void setDisableFg(ColorUIResource c);

    public void setTabBg(ColorUIResource c);

    public void setShadowFg(ColorUIResource c);

    public void setSeparatorBg(ColorUIResource c);

    public void setInputBg(ColorUIResource c);

    public void setSelectedFg(ColorUIResource c);

    public void setSelectedBg(ColorUIResource c);

    public void setMenuBarBg(ColorUIResource c);

    public void setInactiveFg(ColorUIResource c);

    public void setInactiveBg(ColorUIResource c);

    public void setGridFg(ColorUIResource c);

    public void setFocusBg(ColorUIResource c);

    public void setHilightFg(ColorUIResource c);

}

