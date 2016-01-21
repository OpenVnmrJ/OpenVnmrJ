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

import vnmr.vplaf.jtattoo.AbstractTheme;
import vnmr.vplaf.jtattoo.ColorHelper;
import vnmr.vplaf.jtattoo.VAbstractThemeIF;
import java.awt.Color;
import javax.swing.plaf.ColorUIResource;

public class AluminiumDefaultTheme extends AbstractTheme implements VAbstractThemeIF {
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


    public AluminiumDefaultTheme() {
        super();
        // Setup theme with defaults
        setUpColor();
        // Overwrite defaults with user props
        loadProperties();
        // Setup the color arrays
        setUpColorArrs();
    }

    public String getPropertyFileName() {
        return "AluminiumTheme.properties";
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
        backgroundColorLight = defLightBg;
        backgroundColorDark = defBg;
        alterBackgroundColor = defLightBg;
         
        hilightLight = lighterColor(hilightBg, 0.2);

        selectionForegroundColor = selectedFg;
        selectionBackgroundColor = selectedBg;

        focusColor = focusBg;
        focusCellColor = focusBg;

        rolloverColor = lighterColor(focusBg, level1);
        rolloverColorLight = lighterColor(focusBg, level2);
        rolloverColorDark = focusBg;

        buttonBackgroundColor = defLightBg;
        buttonColorLight = defLighterBg;
        buttonColorDark = defDarkBg;

        controlBackgroundColor = defLightBg;
        controlColorLight = defLighterBg;
        controlColorDark = defDarkBg;
        controlHighlightColor = hilightBg;
        controlShadowColor = shadowFg;
        controlDarkShadowColor = darkerColor(shadowFg, level2);

        menuBackgroundColor = defBg;
        menuSelectionForegroundColor = selectedBg;
        menuSelectionBackgroundColor = selectedFg;
        
        menuColorLight = controlColorLight;
        menuColorDark = controlColorDark;

        toolbarBackgroundColor = backgroundColor;
        toolbarColorLight = defLighterBg;
        toolbarColorDark = defDarkBg;

        tabAreaBackgroundColor = backgroundColor;
        desktopColor = backgroundColor;

        gridColor = gridFg;

        inputForegroundColor = defFg;
        inputBackgroundColor = inputBg;

        separatorBackgroundColor = separatorBg;

        hilightLight = lighterColor(hilightBg, 0.2);

    }


    public void setUpColor() {
        super.setUpColor();
        // Defaults for AluminiumLookAndFeel
        backgroundColor = new ColorUIResource(200, 200, 200);
        backgroundColorLight = new ColorUIResource(240, 240, 240);
        backgroundColorDark = new ColorUIResource(200, 200, 200);
        alterBackgroundColor = new ColorUIResource(220, 220, 220);

        frameColor = new ColorUIResource(140, 140, 140);
        backgroundPattern = true;
        selectionForegroundColor = black;
        selectionBackgroundColor = new ColorUIResource(224, 227, 206);

        focusColor = new ColorUIResource(255, 128, 96);
        focusCellColor = focusColor;

        rolloverColor = new ColorUIResource(196, 203, 163);
        rolloverColorLight = new ColorUIResource(220, 224, 201);
        rolloverColorDark = new ColorUIResource(196, 203, 163);

        buttonBackgroundColor = extraLightGray;
        buttonColorLight = white;
        buttonColorDark = new ColorUIResource(210, 212, 214);

        controlBackgroundColor = extraLightGray;
        controlColorLight = new ColorUIResource(244, 244, 244);
        controlColorDark = new ColorUIResource(224, 224, 224);
        controlHighlightColor = new ColorUIResource(240, 240, 240);
        controlShadowColor = new ColorUIResource(180, 180, 180);

        windowTitleForegroundColor = new ColorUIResource(32, 32, 32);
        windowTitleBackgroundColor = new ColorUIResource(200, 200, 200);
        windowTitleColorLight = new ColorUIResource(200, 200, 200);
        windowTitleColorDark = new ColorUIResource(160, 160, 160);
        windowBorderColor = new ColorUIResource(120, 120, 120);
        windowIconColor = new ColorUIResource(32, 32, 32);
        windowIconShadowColor = new ColorUIResource(208, 208, 208);
        windowIconRolloverColor = new ColorUIResource(196, 0, 0);

        windowInactiveTitleForegroundColor = black;
        windowInactiveTitleBackgroundColor = new ColorUIResource(220, 220, 220);
        windowInactiveTitleColorLight = new ColorUIResource(220, 220, 220);
        windowInactiveTitleColorDark = new ColorUIResource(200, 200, 200);
        windowInactiveBorderColor = new ColorUIResource(140, 140, 140);

        menuBackgroundColor = extraLightGray;
        menuSelectionForegroundColor = selectionForegroundColor;
        menuSelectionBackgroundColor = new ColorUIResource(202, 208, 172);
        
        menuColorLight = controlColorLight;
        menuColorDark = controlColorDark;

        toolbarBackgroundColor = backgroundColor;
        toolbarColorLight = new ColorUIResource(240, 240, 240);
        toolbarColorDark = new ColorUIResource(200, 200, 200);

        tabAreaBackgroundColor = backgroundColor;
        desktopColor = backgroundColor;

        // if (defBg != null)
        //      setCustomColor();
    }

    public void setUpColorArrs() {
        super.setUpColorArrs();
        DEFAULT_COLORS = ColorHelper.createColorArr(controlColorLight, controlColorDark, 20);
        HIDEFAULT_COLORS = new Color[DEFAULT_COLORS.length];
        for (int i = 0; i < DEFAULT_COLORS.length; i++) {
            HIDEFAULT_COLORS[i] = ColorHelper.brighter(DEFAULT_COLORS[i], 20);
        }

        ACTIVE_COLORS = DEFAULT_COLORS;
        INACTIVE_COLORS = ColorHelper.createColorArr(new Color(240, 240, 240), new Color(220, 220, 220), 20);

        PRESSED_COLORS = ColorHelper.createColorArr(ColorHelper.darker(selectionBackgroundColor, 5), ColorHelper.brighter(selectionBackgroundColor, 20), 20);
        DISABLED_COLORS = ColorHelper.createColorArr(Color.white, Color.lightGray, 20);
        BUTTON_COLORS = new Color[]{
                    new Color(240, 240, 240),
                    new Color(235, 235, 235),
                    new Color(232, 232, 232),
                    new Color(230, 230, 230),
                    new Color(228, 228, 228),
                    new Color(225, 225, 225),
                    new Color(220, 220, 220),
                    new Color(215, 215, 215),
                    new Color(210, 210, 210),
                    new Color(205, 205, 205),
                    new Color(210, 210, 210),
                    new Color(215, 215, 215),
                    new Color(220, 220, 220),
                    new Color(225, 225, 225),
                    new Color(228, 228, 228),
                    new Color(230, 230, 230),
                    new Color(232, 232, 232),
                    new Color(235, 235, 235),};
        ROLLOVER_COLORS = ColorHelper.createColorArr(rolloverColorLight, rolloverColorDark, 20);
        WINDOW_TITLE_COLORS = ColorHelper.createColorArr(windowTitleColorLight, windowTitleColorDark, 20);
        WINDOW_INACTIVE_TITLE_COLORS = ColorHelper.createColorArr(windowInactiveTitleColorLight, windowInactiveTitleColorDark, 20);
        MENUBAR_COLORS = ColorHelper.createColorArr(menuColorLight, menuColorDark, 20);
        TOOLBAR_COLORS = ColorHelper.createColorArr(toolbarColorLight, toolbarColorDark, 20);
        TAB_COLORS = DEFAULT_COLORS;
        COL_HEADER_COLORS = BUTTON_COLORS;

        SELECTED_COLORS = ColorHelper.createColorArr(ColorHelper.brighter(selectionBackgroundColor, 40), selectionBackgroundColor, 20);

        TRACK_COLORS = ColorHelper.createColorArr(new Color(210, 210, 210), new Color(230, 230, 230), 20);
        //THUMB_COLORS = ColorHelper.createColorArr(new Color(202, 208, 172), new Color(180, 188, 137), 20);
        THUMB_COLORS = ColorHelper.createColorArr(new Color(200, 200, 200), new Color(170, 170, 170), 20);
        //SLIDER_COLORS = DEFAULT_COLORS;
        SLIDER_COLORS = ColorHelper.createColorArr(new Color(180, 180, 180), new Color(150, 150, 150), 10);
        PROGRESSBAR_COLORS = SLIDER_COLORS;
    }
}
