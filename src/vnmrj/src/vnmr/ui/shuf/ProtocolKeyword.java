/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

/********************************************************** <pre>
 * Summary: Container for a Single Protocol Keyword
 *
 * Use in relation to the Protocol Browser
</pre> **********************************************************/

public class ProtocolKeyword {
    protected String name;
    protected String value="All";
    // type = "system" or "keyword-search"
    // system values come from global vnmr variables
    // keyword-search values come from the panel
    protected String type;
    // For system keywords, the user may click a checkbox to skip using
    // this keyword in the search.
    protected boolean skip=false;

    public ProtocolKeyword(String key, String val, String typ) {
        name = key;
        value = val;
        type = typ;
    }

    public String getName() {
        return name;
    }
    public String getValue() {
        return value;
    }
    public String getType() {
        return type;
    }
    public boolean getSkip() {
        return skip;
    }
    public void setValue(String val) {
        value = val;
    }
    public void setType(String typ) {
        type = typ;
    }
    public void setSkip(boolean state) {
        skip = state;
    }
}
