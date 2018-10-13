/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.util;

import java.awt.*;

/**
 * Put global constants here.  It is probably a bad idea to put
 * variables or mutable objects here.
 *
 */
public class Global {
    // ==== constants
    /** default background color */
    public static Color BGCOLOR = Color.lightGray;

    /** active(on) state color */
    public static Color ONCOLOR = Color.cyan;

    /** not present state color */
    /** public static Color NPCOLOR = new Color(140, 140, 180); */
    public static Color NPCOLOR = new Color(200, 200, 210);

    /** inactive color */
    public static Color IDLECOLOR = new Color(160, 160, 160);

    /** off state  color */
    public static Color OFFCOLOR = Color.yellow;

    /** ready state  color */
    public static Color READYCOLOR = new Color(0,150,0);

    /** ready state  color */
    public static Color INTERACTIVECOLOR = new Color(255,64,64);

    /** highlight color */
    public static Color HIGHLIGHTCOLOR =  new Color(128, 128, 128);

    /** use text color to show Infostat status if true (else color bg) */
    public static boolean STATUS_TO_FG = true;

    /** transitional state  color */
    public static Color TRANSCOLOR = Color.green;
 
    /** id for admin interface */
    public static String ADMINIF = "Administrative";
    /** id for experiment interface */
    public static String WALKUPIF = "Spectroscopy";
    /** id for imaging interface */
    public static String IMGIF = "Imaging";
    /** id for lc interface */
    public static String LCIF = "LC-NMR/MS";

    /** directory name and file extension for imaging interface */
    public static final String IMAGING   = "imaging";
    /** directory name and file extension for walkup interface */
    public static final String WALKUP    = "walkup";
    /** directory name and file extension for lc interface */
    public static final String LC        = "lc";

	public static String VIEWPORT = "vp";
	public static String PLAN = "plan";
	public static String CURRENT = "current";
	public static String REVIEW = "review";

    public static final String PART11_CONFIG_FILE = "part11Config";

    /**
     * constructor (not meant to be used)
     */
    private Global() {}

} // class Global
