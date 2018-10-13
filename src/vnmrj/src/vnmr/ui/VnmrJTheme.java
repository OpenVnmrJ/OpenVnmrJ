/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import vnmr.util.*;
import vnmr.ui.*;

import java.awt.*;
import java.net.URL;
import java.util.*;

import javax.swing.*;
import javax.swing.plaf.*;
import javax.swing.border.Border;

// import com.sun.java.swing.SwingUtilities2;
// import sun.swing.PrintColorUIResource;
// import sun.swing.SwingLazyValue;

import javax.swing.plaf.metal.*;

/**
 * This class describes a theme based on OceanTheme.
 */
public class VnmrJTheme extends OceanTheme {
    private static ColorUIResource PRIMARY1   = new ColorUIResource(0xFF0000);
    private static ColorUIResource PRIMARY2   = new ColorUIResource(0xFF0000);
    private static ColorUIResource PRIMARY3   = new ColorUIResource(0xFF0000);
    
    private static ColorUIResource SECONDARY1 = new ColorUIResource(0xFF0000);
    private static ColorUIResource SECONDARY2 = new ColorUIResource(0xFF0000);
    private static ColorUIResource SECONDARY3 = new ColorUIResource(0xFF0000);
    
    private static ColorUIResource GRADIENT1 = new ColorUIResource(0xC8DDF2);
    private static ColorUIResource GRADIENT2 = new ColorUIResource(0xB8CFE5);
    
    private static ColorUIResource TABPANE1 = new ColorUIResource(0xC8DDF2);
    private static ColorUIResource TABPANE2 = new ColorUIResource(0xC8DDF2);
    
    private static ColorUIResource SELECTBOX = new ColorUIResource(0xA3B8CC);
    private static ColorUIResource SELECTEDTEXT = new ColorUIResource(0xFFFF00);
    private static ColorUIResource MENUBG1 = new ColorUIResource(0xCCCCCC);
    private static ColorUIResource MENUBG2 = new ColorUIResource(0xCCCCCC);
    private static ColorUIResource SELECTEDBTN = new ColorUIResource(0xA3B8CC);
 
    private static ColorUIResource SEPARATOR = new ColorUIResource(0xEEEEEE);
    private static ColorUIResource DEFAULTBG = new ColorUIResource(0xCCCCCC);
    private static ColorUIResource SHADOW1 = new ColorUIResource(0xCCCCCC);
    private static ColorUIResource SHADOW2 = new ColorUIResource(0x555555);
    private static ColorUIResource HIGHLIGHT = new ColorUIResource(0x555555);
    private static ColorUIResource CRTLHILIGHT = new ColorUIResource(0xFFFFFF);
    private static ColorUIResource FOCUS = new ColorUIResource(0x555555);

    private static ColorUIResource ENTRYBG = new ColorUIResource(0x7A8A99);
    private static ColorUIResource DISABLEDFG = new ColorUIResource(0x555555);
    private static ColorUIResource DISABLEDBG = new ColorUIResource(0x222222);
    private static ColorUIResource INACTIVEFG = new ColorUIResource(0x555555);
    private static ColorUIResource INACTIVEBG = new ColorUIResource(0x222222);
    private static ColorUIResource GRID = new ColorUIResource(0x222222);


    private static ColorUIResource WHITE = new ColorUIResource(0xFFFFFF);
    private static ColorUIResource BLACK = new ColorUIResource(0x000000);

    public VnmrJTheme() {
    }

    boolean isSystemTheme() {
        return false;
    }

    public String getName() {
        return "VnmrJ";
    }

    public static void setPrimary1(Color c) {       
        if(c==null)
            return;
        PRIMARY1= new ColorUIResource(c);
    }
    public static void setPrimary2(Color c) {       
        if(c==null)
            return;
        PRIMARY2= new ColorUIResource(c);
    }
    public static void setPrimary3(Color c) {       
        if(c==null)
            return;
        PRIMARY3= new ColorUIResource(c);
    }

    public static void setSecondary1(Color c) {       
        if(c==null)
            return;
        SECONDARY1= new ColorUIResource(c);
     }
    public static void setSecondary2(Color c) {       
        if(c==null)
            return;
        SECONDARY2= new ColorUIResource(c);
     }
    public static void setSecondary3(Color c) {       
        if(c==null)
            return;
        SECONDARY3= new ColorUIResource(c);
     }
  
    public static void setFocusedColor(Color c) {       
        if(c==null)
            return;
        FOCUS= new ColorUIResource(c);
    }

    public static void setSelectBoxColor(Color c) {       
        if(c==null)
            return;
        SELECTBOX= new ColorUIResource(c);
     }

    public static void setGridColor(Color c) {       
        if(c==null)
            return;
        GRID= new ColorUIResource(c);
    }

    public static void setSelectedTextColor(Color c) {       
        if(c==null)
            return;
        SELECTEDTEXT= new ColorUIResource(c);
    }

    public static void setMenuBarBGColor(Color c) {       
        if(c==null)
            return;
        MENUBG1= new ColorUIResource(c);
     }

    public static void setMenuBGColor(Color c) {       
        if(c==null)
            return;
        MENUBG2= new ColorUIResource(c);
    }

    public static void setSelectedBtnColor(Color c) {       
        if(c==null)
            return;
        SELECTEDBTN= new ColorUIResource(c);
    }

    public static void setGradient1(Color c) {
        if(c==null)
            return;
        GRADIENT1= new ColorUIResource(c);
    }
    public static void setGradient2(Color c) {
        if(c==null)
            return;
        GRADIENT2= new ColorUIResource(c);
    }
    public static void setSeparator(Color c) {
        if(c==null)
            return;
        SEPARATOR= new ColorUIResource(c);
    }

    public static void setDefaultBG(Color c) {
        if(c==null)
            return;
        DEFAULTBG= new ColorUIResource(c);
        //UIManager.getLookAndFeelDefaults().put("Panel.background", DEFAULTBG);
    }

    public static void setTabPaneBG(Color c) {
        if(c==null)
            return;
        TABPANE1= new ColorUIResource(c);
    }

    public static void setTabPaneTabs(Color c) {
        if(c==null)
            return;
        TABPANE2= new ColorUIResource(c);
    }
    public static Color getTabPaneBG() {
        return TABPANE1;
   }

    public static Color getTabPaneTabs() {
         return TABPANE2;
    }

    public static void setDisabledFG(Color c) {
        if(c==null)
            return;
        DISABLEDFG = new ColorUIResource(c);
    }

    public static void setDisabledBG(Color c) {
        if(c==null)
            return;
        DISABLEDBG = new ColorUIResource(c);
    }

    public static void setInactiveFG(Color c) {
        if(c==null)
            return;
        INACTIVEFG = new ColorUIResource(c);
    }

    public static void setInactiveBG(Color c) {
        if(c==null)
            return;
        INACTIVEBG = new ColorUIResource(c);
    }

    public static void setEntryBG(Color c) {
        if(c==null)
            return;
        ENTRYBG = new ColorUIResource(c);
    }

    public static void setShadowColor(Color c) {
        if(c==null)
            return;
        SHADOW1 = new ColorUIResource(c);
    }

    public static void setDarkShadowColor(Color c) {
        if(c==null)
            return;
        SHADOW2= new ColorUIResource(c);
    }

    public static void setHighlightColor(Color c) {
        if(c==null)
            return;
        HIGHLIGHT = new ColorUIResource(c);
    }

    public static void setControlLightHighlight(Color c) {
        if(c==null)
            return;
        CRTLHILIGHT = new ColorUIResource(c);
    }

    public static Object[] getDefaults() {
        Object focusBorder = new UIDefaults.ProxyLazyValue(
                "javax.swing.plaf.BorderUIResource$LineBorderUIResource",
                new Object[] { FOCUS });
        java.util.List buttonGradient = Arrays.asList(
                 new Object[] {new Float(.3f), new Float(0.2f),
                         GRADIENT1, CRTLHILIGHT , GRADIENT2 });

        java.util.List checkBoxGradient = Arrays.asList(
                 new Object[] {new Float(.3f), new Float(0.2f),
                         ENTRYBG, ENTRYBG , ENTRYBG });

        java.util.List sliderGradient = Arrays.asList(new Object[] {
                new Float(.3f), new Float(.2f),
                GRADIENT1, CRTLHILIGHT, GRADIENT2 });
        
        Object entryBorder = new UIDefaults.ProxyLazyValue(
                "javax.swing.plaf.BorderUIResource$BevelBorderUIResource",
                new Object[] {javax.swing.border.BevelBorder.LOWERED});

        Object[] defaults = new Object[] {
                "ToolTip.background",                   DEFAULTBG,
                "ToolTip.foreground",                   BLACK,
                "ToolTip.foregroundInactive",           BLACK,
                "window",                               DEFAULTBG,
                "windowBorder",                         DEFAULTBG,
                "Panel.background",                     DEFAULTBG,
                "OptionPane.background",                DEFAULTBG,
                "ProgressBar.background",               DEFAULTBG,
                "menu",                                 DEFAULTBG,
                "Menu.background",                  	DEFAULTBG,
                "PopupMenu.background",                 DEFAULTBG,
                "MenuItem.background",                  DEFAULTBG,
                "Menu.selectionBackground",		SELECTEDTEXT,
                "MenuItem.selectionBackground",		SELECTEDTEXT,
                "MenuBar.borderColor",                  DEFAULTBG,
                "MenuBar.background",                   MENUBG1,
                // "CheckBox.gradient",                    buttonGradient,
                "CheckBox.gradient",                    checkBoxGradient,
                "CheckBox.background",                  DEFAULTBG,
                "CheckBoxMenuItem.selectionBackground", SELECTEDTEXT,
                "CheckBoxMenuItem.background",          DEFAULTBG,
                "CheckBoxMenuItem.gradient",            buttonGradient,
                "textHighlight",                        SELECTEDTEXT,
                "TextField.selectionBackground",        SELECTEDTEXT,
                "Button.background",                    DEFAULTBG,
                "Button.gradient",                      buttonGradient,
                "Button.toolBarBorderBackground",       DISABLEDFG,
                "Button.disabledBackground",            DISABLEDBG,
                "Button.disabledToolBarBorderBackground",DEFAULTBG,
                "Label.disabledForeground",             DISABLEDFG,
                "Label.background",                     DEFAULTBG,
                "Slider.altTrackColor",                 GRADIENT2,
                "Slider.background",                    DEFAULTBG,
                "Slider.highlight",                     CRTLHILIGHT,
                "Slider.gradient",                      sliderGradient,
                "Slider.focusGradient",                 sliderGradient,
                "ToggleButton.gradient",                buttonGradient,
                "Spinner.background",                   DEFAULTBG,
                "RadioButton.background",               DEFAULTBG,
                "RadioButton.gradient",                 buttonGradient,
                "RadioButtonMenuItem.selectionBackground", SELECTEDTEXT,
                "RadioButtonMenuItem.gradient",         buttonGradient,
                "InternalFrame.activeTitleGradient",    buttonGradient,
                "ToggleButton.gradient",                buttonGradient,
                "TabbedPane.highlight",                 HIGHLIGHT,
                "TabbedPane.darkShadow",                SHADOW2,
                "TabbedPane.contentAreaColor",          TABPANE1,
                "TabbedPane.selected",                  TABPANE1,
                "TabbedPane.background",                TABPANE2,
                "TabbedPane.unselectedBackground",		TABPANE2,
                "TabbedPane.borderHightlightColor",     HIGHLIGHT,
                // "SplitPane.dividerFocusColor",          BLACK,
                // "SplitPane.background",                 DEFAULTBG,
                "Table.focusCellHighlightBorder",       focusBorder,
                "Table.gridColor",                      GRID,
                "ToolBar.border",                  		SEPARATOR,
                "ToolBar.shadow",                       SHADOW1,
                "ToolBar.darkShadow",                   SHADOW2,
                "ToolBar.dockingBackground",            SHADOW2,
                "ToolBar.background",            		SHADOW2,
                "ToolBar.highlight",            		SHADOW2,
                "PopupMenu.background",                 DEFAULTBG,
                "ComboBox.disabledForeground",          DISABLEDFG,
                "ComboBox.disabledBackground",          DISABLEDBG,
                "ComboBox.inactiveBackground",          INACTIVEBG,
                "ComboBox.background",                  DEFAULTBG,
                "ComboBox.selectionBackground",         SELECTEDTEXT,
                // "List.background",                      DEFAULTBG,
                "List.selectionBackground",             SELECTEDTEXT,
                "TextField.background",                 ENTRYBG,
                "TextField.border",                 entryBorder,
                "TextField.disabledBackground",         DISABLEDBG,
                "TextField.inactiveForeground",         DISABLEDFG,
                "TextField.inactiveBackground",         INACTIVEBG,
                "ToggleButton.select",                  SELECTEDBTN,
                "ToggleButton.disabledText",            DISABLEDFG,
                "ScrollPane.background",                DEFAULTBG,
                "ScrollBar.track",                      DEFAULTBG,
                "ScrollBar.foreground",                 DEFAULTBG,
                "ScrollBar.background",                 DEFAULTBG,
                "ScrollBar.gradient",                   buttonGradient,
                "ScrollBar.shadow",                     DEFAULTBG,
                "ScrollBar.darkShadow",                 SHADOW1,
                "ScrollBar.thumb",                      DEFAULTBG,
                "ScrollBar.thumbShadow",                SHADOW1,
                "ScrollBar.thumbHighlight",             DEFAULTBG,
                "Viewport.background",                 DEFAULTBG,
               };
         return defaults;
    }
    /**
     * Add this theme's custom entries to the defaults table.
     * 
     * @param table
     *            the defaults table, non-null
     * @throws NullPointerException
     *             if the parameter is null
     */
    public void addCustomEntriesToTable(UIDefaults table) {
        super.addCustomEntriesToTable(table);
        Object[] defaults=getDefaults();
        table.putDefaults(defaults);
    }
 
    protected ColorUIResource getPrimary1() {return PRIMARY1;}
    protected ColorUIResource getPrimary2() {return PRIMARY2;}
    protected ColorUIResource getPrimary3() {return PRIMARY3;}
    protected ColorUIResource getSecondary1() {return SECONDARY1;}
    protected ColorUIResource getSecondary2() {return SECONDARY2;}
    protected ColorUIResource getSecondary3() {return SECONDARY3;}

    // overrides from MetalTheme class
    
    public ColorUIResource getFocusColor() {return FOCUS;}                              // getBlack()
    public ColorUIResource getDesktopColor() { return DEFAULTBG; }                      // getPrimary2()
    public ColorUIResource getControl() { 
    	return DEFAULTBG; 
    	}                           // getSecondary3() 
    public ColorUIResource getControlShadow() { return DEFAULTBG; }                     // getSecondary2()
    public ColorUIResource getControlDarkShadow() { return SHADOW1; }                   // getSecondary1()
    public ColorUIResource getControlInfo() { return getBlack(); }                      // getBlack()
    public ColorUIResource getControlHighlight() { return CRTLHILIGHT; }                 // getWhite()
    public ColorUIResource getControlDisabled() { return DISABLEDFG; }                  // getSecondary2()
    public ColorUIResource getPrimaryControl() { return FOCUS; }                        // getPrimary3() 
    public ColorUIResource getPrimaryControlShadow() { return SHADOW2; }                // getPrimary2()
    public ColorUIResource getPrimaryControlDarkShadow() { return HIGHLIGHT; }          // getPrimary1()
    public ColorUIResource getPrimaryControlInfo() { return getBlack(); }               // getBlack()
    public ColorUIResource getPrimaryControlHighlight() { return CRTLHILIGHT; }           // getWhite()
    public ColorUIResource getSystemTextColor() { return getBlack(); }                  // getBlack()
    public ColorUIResource getControlTextColor() { return getControlInfo(); }           // getControlInfo()
    public ColorUIResource getInactiveControlTextColor() { return DISABLEDFG; }         // getControlDisabled()
    public ColorUIResource getInactiveSystemTextColor() { return INACTIVEFG; }          // getSecondary2()
    public ColorUIResource getUserTextColor() { return getBlack(); }                    // getBlack()
    public ColorUIResource getTextHighlightColor() { return SELECTEDTEXT; }             // getPrimary3()
    public ColorUIResource getHighlightedTextColor() { return getControlTextColor(); }  // getControlTextColor()
    public ColorUIResource getWindowBackground() { return getWhite(); }                 // getWhite()
    public ColorUIResource getWindowTitleBackground() { return FOCUS; }                 // getPrimary3()
    public ColorUIResource getWindowTitleForeground() { return getBlack(); }            // getBlack()  
    public ColorUIResource getWindowTitleInactiveBackground() { return INACTIVEBG; }    // getSecondary3()
    public ColorUIResource getWindowTitleInactiveForeground() { return getBlack(); }    // getBlack()
    public ColorUIResource getMenuBackground() { return MENUBG2; }                      // getSecondary3()
    public ColorUIResource getMenuForeground() { return  getBlack(); }                  // getBlack()
    public ColorUIResource getMenuSelectedBackground() { return SELECTEDTEXT; }         // getPrimary2()
    public ColorUIResource getMenuSelectedForeground() { return getBlack(); }           // getBlack()
    public ColorUIResource getMenuDisabledForeground() { return getControlDisabled(); } // getSecondary2()
    public ColorUIResource getSeparatorBackground() { return getWhite(); }              // getWhite()
    public ColorUIResource getSeparatorForeground() { return SEPARATOR; }               // getPrimary1()
    public ColorUIResource getAcceleratorForeground() { return SEPARATOR; }             // getPrimary1()
    public ColorUIResource getAcceleratorSelectedForeground() { return getBlack(); }    // getBlack()

}
