/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * 
 *
 */
package vnmr.templates;

import javax.swing.*;
import java.util.*;
import vnmr.ui.shuf.*;
import static vnmr.templates.ProtocolBuilder.*;


/**
 *  Action TreeNode element
 *  @author		Dean Sindorf
 */
public class VActionElement extends VTreeNodeElement
{
    public boolean getAllowsChildren () { return false; }
    
    public String toString() {
        StringBuffer sb = new StringBuffer("VActionElement<");
        sb.append(getAttribute(ATTR_ID)).append(",");
        sb.append(getAttribute(ATTR_TITLE)).append(",");
        sb.append(getAttribute(ATTR_STATUS)).append(",");
        sb.append(getAttribute(ATTR_TIME)).append(">");
        return sb.toString();
    }
}
