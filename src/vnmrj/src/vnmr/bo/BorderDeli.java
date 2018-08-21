/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import javax.swing.border.*;
import javax.swing.BorderFactory;
import vnmr.util.VTitledBorder;

/**
 * Provides an convenient way to get a Border for a JComponent from
 * a string description of its properties.
 */
public class BorderDeli extends Object {

    private BorderDeli() {}

    /**
     * Returns a border, optionally with a title.  The border properties
     * are mostly specified by Strings.  These are case insensitive and
     * may contain "extra" trailing characters.  Thus, a lowered, beveled
     * border is specified by <i>lower</i> but <i><B>Lower</B>edBeveled</i> is
     * also accepted.  Required characters are in <B>bold</B> below.
     *
     * @param type
     * The type of border.  The default (matching none of the recognized
     * types) is no border.  The following types are recognized:
     * <pre>
     * <B>Etch</B>ed
     * <B>Raise</B>dBeveled
     * <B>Lower</B>dBeveled
     * </pre>
     * @param title
     * A title for the group, or an empty string for no title.
     * @param titlePosition
     * The vertical location on the title on the frame. The default
     * is <i>Top</i>. Choices are:
     * <pre>
     * <B>Top</B>
     * <B>AboveT</B>op
     * <B>BelowT</B>op
     * <B>Bot</B>tom
     * <B>AboveB</B>ottom
     * <B>BelowB</B>ottom
     * </pre>
     * @param titleJustification
     * The horizontal position of the title. The Default is <i>Left</i>.
     * Choices are:
     * <pre>
     * <B>L</B>eft
     * <B>R</B>ight
     * <B>C</B>enter
     * </pre>
     * @param fontColor  Required: The color used to draw the title.
     * @param font Required: The font of the title.
     */
    public static Border createBorder(String type,
				      String title,
				      String titlePosition,
				      String titleJustification,
				      Color fontColor,
				      Font font) {
	int titleJust = TitledBorder.DEFAULT_JUSTIFICATION;
	int titlePosn = TitledBorder.DEFAULT_POSITION;
	Border border = null;

	/* Set basic border */
	if (type == null || type.trim().length() == 0) {
	    return border;
	}
	type = type.trim().toLowerCase();
	if (type.startsWith("etch")) {
	    border = BorderFactory.createEtchedBorder();
	} else if (type.startsWith("raise") || type.startsWith("bevel")) {
	    border = BorderFactory.createRaisedBevelBorder();
	} else if (type.startsWith("low")) {
	    border = BorderFactory.createLoweredBevelBorder();
	} else
        border = BorderFactory.createEmptyBorder();

	/* Set title */
	if (title != null && title.trim() != null && border != null) {
	    /* Set position */
	    if (titlePosition != null && titlePosition.trim().length()>0) {
		titlePosition = titlePosition.trim().toLowerCase();
		if (titlePosition.startsWith("top")) {
		    titlePosn = TitledBorder.TOP;
		} else if (titlePosition.startsWith("abovet")) {
		    titlePosn = TitledBorder.ABOVE_TOP;
		} else if (titlePosition.startsWith("belowt")) {
		    titlePosn = TitledBorder.BELOW_TOP;
		} else if (titlePosition.startsWith("bot")) {
		    titlePosn = TitledBorder.BOTTOM;
		} else if (titlePosition.startsWith("aboveb")) {
		    titlePosn = TitledBorder.ABOVE_BOTTOM;
		} else if (titlePosition.startsWith("belowb")) {
		    titlePosn = TitledBorder.BELOW_BOTTOM;
		}
	    }

	    /* Set justification */
	    if (titleJustification != null
		&& titleJustification.trim().length()>0)
	    {
		titleJustification = titleJustification.trim().toLowerCase();
		if (titleJustification.startsWith("l")) {
		    titleJust = TitledBorder.LEFT;
		} else if (titleJustification.startsWith("c")) {
		    titleJust = TitledBorder.CENTER;
		} else if (titleJustification.startsWith("r")) {
		    titleJust = TitledBorder.RIGHT;
		}
	    }

	    /* Put title on border */
	    border = new VTitledBorder(border,
				       title,
				       titleJust,
				       titlePosn,
				       font,
				       fontColor);
	}
	return border;
    }
}


