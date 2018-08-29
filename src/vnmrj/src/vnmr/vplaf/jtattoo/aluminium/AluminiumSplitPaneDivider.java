/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * Copyright 2005 MH-Software-Entwicklung. All rights reserved.
 * Use is subject to license terms.
 */
package vnmr.vplaf.jtattoo.aluminium;

import vnmr.vplaf.jtattoo.BaseSplitPaneDivider;
import java.awt.*;

/**
 * @author Michael Hagen
 */
public class AluminiumSplitPaneDivider extends BaseSplitPaneDivider {

    public AluminiumSplitPaneDivider(AluminiumSplitPaneUI ui) {
        super(ui);
    }

    public void paint(Graphics g) {
        AluminiumUtils.fillComponent(g, this);
        Graphics2D g2D = (Graphics2D) g;
        Composite composite = g2D.getComposite();
        AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.3f);
        g2D.setComposite(alpha);
        super.paint(g);
        g2D.setComposite(composite);
    }
}
