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
package vnmr.vplaf.jtattoo.hifi;

import vnmr.vplaf.jtattoo.AbstractTheme;
import vnmr.vplaf.jtattoo.ColorHelper;
import vnmr.vplaf.jtattoo.VAbstractThemeIF;
import java.awt.Color;
import java.awt.Font;
import javax.swing.plaf.ColorUIResource;
import javax.swing.plaf.FontUIResource;

/**
 * @author Michael Hagen
 */
public class HiFiDefaultTheme extends AbstractTheme implements VAbstractThemeIF {
    protected static Color TAB_ROLLOVER_COLORS[] = null;
    public static ColorUIResource defFg;
    public static ColorUIResource defBg;
    public static ColorUIResource defLightBg;
    public static ColorUIResource defLighterBg;
    public static ColorUIResource defDarkBg;
    public static ColorUIResource defDarkerBg;
    public static ColorUIResource buttonBg;
    public static ColorUIResource disableFg;
    public static ColorUIResource disableBg;
    public static ColorUIResource focusBg;
    public static ColorUIResource gridFg;
    public static ColorUIResource hilightBg;
    public static ColorUIResource hilightLight;
    public static ColorUIResource inactiveBg;
    public static ColorUIResource inactiveLight;
    public static ColorUIResource inactiveFg;
    public static ColorUIResource inputBg;
    public static ColorUIResource menuBarBg;
    public static ColorUIResource selectedBg;
    public static ColorUIResource selectedLight;
    public static ColorUIResource selectedFg;
    public static ColorUIResource separatorBg;
    public static ColorUIResource shadowFg;
    public static ColorUIResource tabBg;
    public static ColorUIResource tabLightBg;
    public static ColorUIResource tabDarkBg;
    public static ColorUIResource tabDarkerBg;
    public static ColorUIResource listSelectionBackgroundColor;
    public static ColorUIResource listBackgroundColor;


    public HiFiDefaultTheme() {
        super();
        // Setup theme with defaults
        setUpColor();
        // Overwrite defaults with user props
        loadProperties();
        // Setup the color arrays
        setUpColorArrs();
    }

    public String getPropertyFileName() {
        return "HiFiTheme.properties";
    }

    public static ColorUIResource lighterColor(Color c, double factor) {
        if (c == null)
             return null;
        int r = c.getRed();
        int g = c.getGreen();
        int b = c.getBlue();
        int i = 30;

        if ( r == 0 && g == 0 && b == 0)
           return new ColorUIResource(i, i, i);
        if (r < i ) r = i;
        if (g < i ) g = i;
        if (b < i ) b = i;

        r = Math.min((int)(r/factor), 255);
        g = Math.min((int)(g/factor), 255);
        b = Math.min((int)(b/factor), 255);
        return new ColorUIResource(r, g, b);
    }

    public static ColorUIResource darkerColor(Color c, double factor) {
        if (c == null)
             return null;
        int r = c.getRed();
        int g = c.getGreen();
        int b = c.getBlue();

        r = (Math.max((int)(r * factor), 0));
        g = (Math.max((int)(g * factor), 0));
        b = (Math.max((int)(b * factor), 0));
        return new ColorUIResource(r, g, b);
    }

    public void setDefaultFg(ColorUIResource c) {
        defFg = c;
    }

    public void setDisableBg(ColorUIResource c) {
        disableBg = c;
    }

    public void setDisableFg(ColorUIResource c) {
        disableFg = c;
    }

    public void setTabBg(ColorUIResource c) {
        tabBg = c;
        if (c == null)
            return;
        tabLightBg = lighterColor(tabBg, 0.5);
        tabDarkBg = darkerColor(tabBg, 0.5);
        tabDarkerBg = darkerColor(tabBg, 0.3);
    }

    public void setShadowFg(ColorUIResource c) {
        shadowFg = c;
    }

    public void setSeparatorBg(ColorUIResource c) {
        separatorBg = c;
    }

    public void setInputBg(ColorUIResource c) {
        inputBg = c;
    }

    public void setSelectedFg(ColorUIResource c) {
        selectedFg = c;
    }

    public void setSelectedBg(ColorUIResource c) {
        selectedBg = c;
        selectedLight = lighterColor(selectedBg, 0.3);;
    }

    public void setMenuBarBg(ColorUIResource c) {
        menuBarBg = c;
    }

    public void setInactiveFg(ColorUIResource c) {
        inactiveFg = c;
    }

    public void setInactiveBg(ColorUIResource c) {
        inactiveBg = c;
        inactiveLight = lighterColor(inactiveBg, 0.3);
    }

    public void setGridFg(ColorUIResource c) {
        gridFg = c;
    }

    public void setFocusBg(ColorUIResource c) {
        focusBg = c;
    }

    public void setHilightFg(ColorUIResource c) {
        hilightBg = c;
    }

    public void setButtonBg(ColorUIResource c) {
        buttonBg = c;
    }

    public void setDefaultBg(ColorUIResource c) {
        defBg = c;
        if (c == null)
             return;
        defLightBg = lighterColor(defBg, 0.8);
        defLighterBg = lighterColor(defBg, 0.3);
        defDarkBg = darkerColor(defBg, 0.8);
        defDarkerBg = darkerColor(defBg, 0.3);
    }


    public static void setCustomColor() {
         if (defBg == null)
             return;
         double level1 = 0.8;
         double level2 = 0.3;

         if (disableFg == null)
             disableFg = gray;
         if (disableBg == null)
             disableBg = disabledBackgroundColor;
         if (focusBg == null)
             focusBg = orange;
         if (gridFg == null)
             gridFg = defFg;
         if (hilightBg == null)
             hilightBg = defLightBg;
         if (inactiveBg == null)
             inactiveBg = defBg;
         if (inactiveFg == null)
             inactiveFg = defFg;
         if (inputBg == null)
             inputBg = white;
         if (menuBarBg == null)
             menuBarBg = defBg;
         if (selectedBg == null)
             selectedBg = defBg;
         if (selectedFg == null)
             selectedFg = defFg;
         if (separatorBg == null)
             separatorBg = defBg;
         if (shadowFg == null)
             shadowFg = defFg;
         if (tabBg == null)
             tabBg = defBg;

         disabledForegroundColor = disableFg;
         disabledBackgroundColor = disableBg;

         backgroundColor = defBg;
         backgroundColorLight = defBg;
         backgroundColorDark = darkerColor(defBg, 0.8);

         hilightLight = lighterColor(hilightBg, 0.2);

         selectionForegroundColor = selectedFg;
         selectionBackgroundColor = selectedBg;
         selectionBackgroundColorLight = lighterColor(selectedBg, level2);
         selectionBackgroundColorDark = selectedBg;
         listSelectionBackgroundColor = selectedBg;

         focusColor = focusBg;
         focusCellColor = focusBg;

         gridColor = gridFg;

         inputBackgroundColor = inputBg;
         listBackgroundColor = inputBg;

         rolloverColor = lighterColor(focusBg, level1);
         rolloverColorLight = lighterColor(focusBg, level2);
         rolloverColorDark = focusBg;

         buttonBackgroundColor = defLightBg;
         buttonColorLight = defLighterBg;
         buttonColorDark = defDarkBg;

         controlBackgroundColor = defBg;
         controlColorLight = defLighterBg;
         controlColorDark = defDarkBg;

         controlHighlightColor = hilightBg;
         controlShadowColor = shadowFg;
         controlDarkShadowColor = darkerColor(shadowFg, level2);

         menuBackgroundColor = defBg;
         menuSelectionBackgroundColor = selectedBg;
         menuSelectionForegroundColor = selectedFg;
         menuColorLight = defLighterBg;
         menuColorDark = defDarkBg;

         toolbarBackgroundColor = defLightBg;
         toolbarColorLight = defLighterBg;
         toolbarColorDark = defDarkBg;

         tabAreaBackgroundColor = defBg;
         desktopColor = defLightBg;

         comboxBackkgroundColor = backgroundColor;
         separatorBackgroundColor = separatorBg;
         tableBackgroundColor = inputBackgroundColor;
         treeBackgroundColor = backgroundColor;
    }

    public void setUpColor() {
        super.setUpColor();

        // Defaults for HiFiLookAndFeel
        textShadow = false;
        if (defFg == null)
             defFg = extraLightGray;

        foregroundColor = defFg;
        disabledForegroundColor = gray;
        disabledBackgroundColor = new ColorUIResource(48, 48, 48);

        backgroundColor = new ColorUIResource(48, 48, 48);
        backgroundColorLight = new ColorUIResource(48, 48, 48);
        backgroundColorDark = new ColorUIResource(16, 16, 16);
        alterBackgroundColor = new ColorUIResource(64, 64, 64);
        selectionForegroundColor = white;
        // selectionBackgroundColor = new ColorUIResource(40, 40, 40);
        selectionBackgroundColor = new ColorUIResource(129, 129, 129);
        frameColor = black;
        gridColor = black;
        focusCellColor = orange;
        if (focusBg == null)
           focusBg = orange; 

        if (separatorBg == null)
            separatorBg = backgroundColor;

        inputBackgroundColor = new ColorUIResource(80, 80, 80);
        inputForegroundColor = foregroundColor;

        rolloverColor = new ColorUIResource(112, 112, 112);
        rolloverColorLight = new ColorUIResource(128, 128, 128);
        rolloverColorDark = new ColorUIResource(96, 96, 96);

        buttonForegroundColor = foregroundColor;
        buttonBackgroundColor = new ColorUIResource(96, 96, 96);
        buttonColorLight = new ColorUIResource(96, 96, 96);
        buttonColorDark = new ColorUIResource(32, 32, 32);

        controlForegroundColor = foregroundColor;
        controlBackgroundColor = new ColorUIResource(64, 64, 64); // netbeans use this for selected tab in the toolbar
        controlColorLight = new ColorUIResource(96, 96, 96);
        controlColorDark = new ColorUIResource(32, 32, 32);
        controlHighlightColor = new ColorUIResource(96, 96, 96);
        controlShadowColor = new ColorUIResource(32, 32, 32);
        controlDarkShadowColor = black;


        windowTitleForegroundColor = foregroundColor;
        windowTitleBackgroundColor = new ColorUIResource(96, 96, 96);
        windowTitleColorLight = new ColorUIResource(96, 96, 96);
        windowTitleColorDark = new ColorUIResource(32, 32, 32);//new ColorUIResource(16, 16, 16);
        windowBorderColor =  black;
        windowIconColor = lightGray;
        windowIconShadowColor = black;
        windowIconRolloverColor = orange;

        windowInactiveTitleForegroundColor = new ColorUIResource(196, 196, 196);
        windowInactiveTitleBackgroundColor = new ColorUIResource(64, 64, 64);
        windowInactiveTitleColorLight = new ColorUIResource(64, 64, 64);
        windowInactiveTitleColorDark = new ColorUIResource(32, 32, 32);
        windowInactiveBorderColor = black;

        menuForegroundColor = foregroundColor;
        menuBackgroundColor = new ColorUIResource(32, 32, 32);
        menuSelectionForegroundColor = white;
        menuSelectionBackgroundColor = new ColorUIResource(96, 96, 96);
        menuColorLight = new ColorUIResource(96, 96, 96);
        menuColorDark = new ColorUIResource(32, 32, 32);

        listBackgroundColor = new ColorUIResource(60, 60, 60);
        listSelectionBackgroundColor = new ColorUIResource(80, 80, 80);

        toolbarBackgroundColor = new ColorUIResource(64, 64, 64);
        toolbarColorLight = new ColorUIResource(96, 96, 96);
        toolbarColorDark = new ColorUIResource(32, 32, 32);

        tabAreaBackgroundColor = backgroundColor;
        desktopColor = new ColorUIResource(64, 64, 64);

        controlFont = new FontUIResource("Dialog", Font.BOLD, 12);
        systemFont = new FontUIResource("Dialog", Font.BOLD, 12);
        userFont = new FontUIResource("Dialog", Font.BOLD, 12);
        menuFont = new FontUIResource("Dialog", Font.BOLD, 12);
        windowTitleFont = new FontUIResource("Dialog", Font.BOLD, 12);
        smallFont = new FontUIResource("Dialog", Font.PLAIN, 10);

        comboxBackkgroundColor = inputBackgroundColor;
        separatorBackgroundColor = backgroundColor;
        tableBackgroundColor = inputBackgroundColor;
        treeBackgroundColor = inputBackgroundColor;
        hilightBg = new ColorUIResource(ColorHelper.brighter(controlColorDark, 15));
        hilightLight = new ColorUIResource(ColorHelper.brighter(controlColorLight, 15));
        inactiveBg = new ColorUIResource(32, 32, 32);
        inactiveLight = new ColorUIResource(64, 64, 64);
        selectedBg = hilightBg;
        selectedLight = hilightLight;

        if (defBg != null)
             setCustomColor();
    }

    public void setUpColorArrs() {
        super.setUpColorArrs();
        DEFAULT_COLORS = ColorHelper.createColorArr(controlColorLight, controlColorDark, 20);
       // HIDEFAULT_COLORS = ColorHelper.createColorArr(ColorHelper.brighter(controlColorLight, 15), ColorHelper.brighter(controlColorDark, 15), 20);
        // INACTIVE_COLORS = ColorHelper.createColorArr(new Color(64, 64, 64), new Color(32, 32, 32), 20);
        HIDEFAULT_COLORS = ColorHelper.createColorArr(hilightLight, hilightBg, 20);
        ACTIVE_COLORS = DEFAULT_COLORS;
        INACTIVE_COLORS = ColorHelper.createColorArr(inactiveLight, inactiveBg, 20);
        SELECTED_COLORS = ColorHelper.createColorArr(selectedLight, selectedBg, 20);
        BUTTON_COLORS = ColorHelper.createColorArr(buttonColorLight, buttonColorDark, 20);
        ROLLOVER_COLORS = HIDEFAULT_COLORS;
        PRESSED_COLORS = ColorHelper.createColorArr(black, controlColorDark, 20);
        DISABLED_COLORS = ColorHelper.createColorArr(ColorHelper.darker(controlColorLight, 10), ColorHelper.darker(controlColorDark, 10), 20);
        WINDOW_TITLE_COLORS = ColorHelper.createColorArr(windowTitleColorLight, windowTitleColorDark, 20);
        WINDOW_INACTIVE_TITLE_COLORS = ColorHelper.createColorArr(windowInactiveTitleColorLight, windowInactiveTitleColorDark, 20);
        MENUBAR_COLORS = DEFAULT_COLORS;
        TOOLBAR_COLORS = MENUBAR_COLORS;
        TRACK_COLORS = ColorHelper.createColorArr(ColorHelper.darker(backgroundColor, 10), ColorHelper.brighter(backgroundColor, 5), 20);
        SLIDER_COLORS = DEFAULT_COLORS;
        PROGRESSBAR_COLORS = DEFAULT_COLORS;
        THUMB_COLORS = DEFAULT_COLORS;
        TAB_COLORS = DEFAULT_COLORS;
        TAB_ROLLOVER_COLORS = ROLLOVER_COLORS;
        COL_HEADER_COLORS = DEFAULT_COLORS;

        TAB_INACTIVE_COLORS = TAB_COLORS;
    }

    public ColorUIResource getListSelectionBackgroundColor() {
        return listSelectionBackgroundColor;
    }

    public ColorUIResource getListBackgroundColor() {
        return listBackgroundColor;
    }
}
