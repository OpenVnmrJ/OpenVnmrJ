/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.awt.*;
import java.util.Properties;

import javax.swing.*;
import javax.swing.border.*;
import javax.swing.plaf.*;
import javax.swing.plaf.metal.MetalLookAndFeel;
import javax.swing.plaf.basic.BasicComboBoxUI;
import vnmr.vplaf.jtattoo.AbstractLookAndFeel;
import vnmr.vplaf.jtattoo.hifi.HiFiDefaultTheme;
import vnmr.vplaf.jtattoo.hifi.HiFiComboBoxUI;
import vnmr.vplaf.jtattoo.BaseComboBoxUI;
import vnmr.vplaf.jtattoo.BaseIcons;
import vnmr.vplaf.jtattoo.VAbstractThemeIF;
import vnmr.vplaf.jtattoo.AbstractTheme;
import vnmr.util.Messages;
import vnmr.util.Util;
import vnmr.bo.VMenu;
import vnmr.bo.VFileMenu;


public class VLookAndFeelFactory  {
    public  static String VNMRJ = "Vnmrj";
    public  static String SYSTEM = "System";
    public  static String DARK = "Dark";
    public  static String LIGHT = "Light";
    private static String[] lfNameList;
    private static UIManager.LookAndFeelInfo[] sysLAFlists;
    private static String METAL = "Metal";
    private static String SYSTEM_LAF = "GTK+";
    private static Properties uiProps;
    private static boolean  bHiFi = false;
    private static boolean  bVnmrj = false;

    // private static String[] LFlist = { "Acryl", "Aero",
    //        "Aluminum", "Bernstein", "Fast", "HiFi",
    //        "Luna", "McWin", "Mint", "Noire", "Smart", "Texture" };

    private static String[] LFlist = { VNMRJ, SYSTEM, DARK, LIGHT, "Acryl", "Aero","Luna", "McWin", "Mint","Texture"};

    public VLookAndFeelFactory() {
    }

    private static void setDebug(String s){
        if(DebugOutput.isSetFor("laf"))
            Messages.postDebug("VLookAndFeelFactory : "+s);
    }
    public static String[] getLAFs() {
          if (lfNameList != null)
               return lfNameList;
          int i, num;
          String systemLaf = UIManager.getSystemLookAndFeelClassName();
          if (systemLaf != null) {
               if (systemLaf.contains("windows"))
                   SYSTEM_LAF = "Windows";
          }
          /*********
          sysLAFlists = UIManager.getInstalledLookAndFeels();
          lfNameList = new String[sysLAFlists.length + LFlist.length + 2];
          lfNameList[0] = VNMRJ;
          lfNameList[1] = SYSTEM;
          num = 2;
          for (i = 0; i < sysLAFlists.length; i++) {
                String laf = sysLAFlists[i].getName();
                if ((!laf.equals(METAL)) && (!laf.equals(SYSTEM_LAF))) {
                    if (!laf.contains("Motif")) {
                        lfNameList[num] = laf;
                        num++;
                    }
                }
          }
          *********/
          num = 0;
          lfNameList = new String[LFlist.length];
          for (i = 0; i < LFlist.length; i++) {
                lfNameList[num] = LFlist[i];
                num++;
          }

          return lfNameList;
    }

    public static boolean isHiFiLAF() {
         return bHiFi;
    }

    public static boolean isVnmrjLAF() {
         return bVnmrj;
    }

    public static void setArrowButton(String laf) {
         if (laf == null)
              return;
         JComboBox cb = new JComboBox();
         cb.addItem("abc");
         Dimension dim = cb.getPreferredSize();
         Font font = cb.getFont();
         if (font != null) {
             FontMetrics fm = cb.getFontMetrics(font);
             int w = fm.stringWidth("abc");
             w = dim.width - w;
             if (w < 18 || w > 36)
                 w = 26;
             VMenu.setMinWidth(w); // JComboBox's arrowbutton and insets
             VFileMenu.setMinWidth(w);
         }
         if (laf.equalsIgnoreCase("hifi") || laf.equalsIgnoreCase(DARK)) {
              HiFiComboBoxUI bui = new HiFiComboBoxUI();
              JButton btn = bui.createArrowButton();
              Util.setArrowButton(btn);
              return;
          }
          Icon  icon = Util.getImageIcon( "DownArrow.gif" );
          if (icon == null)
               icon = Util.getImageIcon( "down.gif" );
          JButton uiBtn = new JButton(icon);
          Color c = UIManager.getColor("Button.shadow");
          Border border = BorderFactory.createMatteBorder(0, 0, 1, 1, c);
          uiBtn.setBorder(border);
          Util.setArrowButton(uiBtn);
    }

    public static void setBumpIcons(String laf) {
         if (laf == null)
              return;
         ImageIcon icon;
         if (laf.equalsIgnoreCase("hifi") || laf.equalsIgnoreCase(DARK)) {
            icon = Util.getImageIcon("blackHorBumps.gif");
            if (icon != null)
                Util.setHorBumpIcon(icon);
            icon = Util.getImageIcon("blackVerBumps.gif");
            if (icon != null)
                Util.setVerBumpIcon(icon);
            return;
         }
         icon = Util.getImageIcon("grayHorBumps.gif");
         if (icon != null)
             Util.setHorBumpIcon(icon);
         icon = Util.getImageIcon("grayVerBumps.gif");
         if (icon != null)
             Util.setVerBumpIcon(icon);
    }

    public static String getLAFClassName(String laf) {
          bHiFi = false;
          bVnmrj = false;

          if (laf == null || laf.equalsIgnoreCase("system"))
              return UIManager.getSystemLookAndFeelClassName();
          if (laf.equalsIgnoreCase("vnmrj")) {
             bVnmrj = true;;
             return "vnmr.ui.VMetalLookAndFeel";
             //  laf = METAL;
          }
          
          if (sysLAFlists == null)
              sysLAFlists = UIManager.getInstalledLookAndFeels();
          for (int i = 0; i < sysLAFlists.length; i++) {
              if (laf.equals(sysLAFlists[i].getName()))
                   return sysLAFlists[i].getClassName(); 
          }
          if (laf.equals("Acryl"))
             return "com.jtattoo.plaf.acryl.AcrylLookAndFeel";
          if (laf.equals("Aero"))
             return "com.jtattoo.plaf.aero.AeroLookAndFeel";
          if (laf.equalsIgnoreCase("aluminum") || laf.equalsIgnoreCase(LIGHT)) {
              return "vnmr.vplaf.jtattoo.aluminium.AluminiumLookAndFeel";
              // return "com.jtattoo.plaf.aluminium.AluminiumLookAndFeel";
          }
          if (laf.equals("Bernstein"))
             return "com.jtattoo.plaf.bernstein.BernsteinLookAndFeel";
          if (laf.equals("Fast"))
             return "com.jtattoo.plaf.fast.FastLookAndFeel";
          if (laf.equals("Graphite"))
             return "com.jtattoo.plaf.graphite.GraphiteLookAndFeel";
          if (laf.equalsIgnoreCase("hifi") || laf.equalsIgnoreCase(DARK)) {
             bHiFi = true;
             return "vnmr.vplaf.jtattoo.hifi.HiFiLookAndFeel";
             // return "com.jtattoo.plaf.hifi.HiFiLookAndFeel";
          }
          if (laf.equals("Luna"))
             return "com.jtattoo.plaf.luna.LunaLookAndFeel";
          if (laf.equals("McWin")) {
             return "vnmr.vplaf.jtattoo.mcwin.McWinLookAndFeel";
             // return "com.jtattoo.plaf.mcwin.McWinLookAndFeel";
          }
          if (laf.equals("Mint"))
             return "com.jtattoo.plaf.mint.MintLookAndFeel";
          if (laf.equals("Noire"))
             return "com.jtattoo.plaf.noire.NoireLookAndFeel";
          if (laf.equals("Smart"))
             return "com.jtattoo.plaf.smart.SmartLookAndFeel";
          if (laf.equals("Texture"))
             // return "vnmr.vplaf.jtattoo.plaf.texture.TextureLookAndFeel";
             return "com.jtattoo.plaf.texture.TextureLookAndFeel";

          return UIManager.getSystemLookAndFeelClassName();
    }

    public static void setDefaultTheme(String laf) {
    	  Properties props = new Properties();
    	  String mlabel=Util.getLabel("dlMenuLabel","VnmrJ");
    	  // props.put("logoString", mlabel);
    	  props.put("logoString", "");

          if (laf == null || laf.equalsIgnoreCase("system")) {
              return;
          }
          if (laf.equalsIgnoreCase("vnmrj")) {  // metal
          //    javax.swing.plaf.metal.MetalLookAndFeel.setCurrentTheme(new javax.swing.plaf.metal.DefaultMetalTheme());
              return;
          }
          if (laf.equals("Acryl")) {
              com.jtattoo.plaf.acryl.AcrylLookAndFeel.setTheme(props);
              return;
          }
          if (laf.equals("Aero")) {
              com.jtattoo.plaf.aero.AeroLookAndFeel.setTheme(props);
              return;
          }
          if (laf.equalsIgnoreCase("aluminum") || laf.equalsIgnoreCase(LIGHT)) {
              // com.jtattoo.plaf.aluminium.AluminiumLookAndFeel.setTheme(props);
              addCustomTheme(laf);
              return;
          }
          if (laf.equals("Bernstein")) {
              com.jtattoo.plaf.bernstein.BernsteinLookAndFeel.setTheme(props);
              return;
          }
          if (laf.equals("Fast")) {
              com.jtattoo.plaf.fast.FastLookAndFeel.setTheme(props);
              return;
          }
          if (laf.equals("Graphite")) {
              com.jtattoo.plaf.graphite.GraphiteLookAndFeel.setTheme(props);
              return;
          }
          if (laf.equalsIgnoreCase("hifi") || laf.equalsIgnoreCase(DARK)) {
              // com.jtattoo.plaf.hifi.HiFiLookAndFeel.setTheme(props);
              addCustomTheme(laf);
              return;
          }
          if (laf.equals("Luna")) {
              com.jtattoo.plaf.luna.LunaLookAndFeel.setTheme(props); 
              return;
          }
          if (laf.equals("McWin")) {
              // vnmr.vplaf.jtattoo.mcwin.McWinLookAndFeel.setTheme(props);
              com.jtattoo.plaf.mcwin.McWinLookAndFeel.setTheme(props);
              return;
          }
          if (laf.equals("Mint")) {
              com.jtattoo.plaf.mint.MintLookAndFeel.setTheme(props);
              return;
          }
          if (laf.equals("Noire")) {
              com.jtattoo.plaf.noire.NoireLookAndFeel.setTheme(props);
              return;
          }
          if (laf.equals("Smart")) {
              com.jtattoo.plaf.smart.SmartLookAndFeel.setTheme(props);
              return;
          }
          if (laf.equals("Texture")) {
              // vnmr.vplaf.jtattoo.texture.TextureLookAndFeel.setTheme(props);
              com.jtattoo.plaf.texture.TextureLookAndFeel.setTheme(props);
              return;
          }
    }

    public static void setLookAndFeel(String laf, boolean defaultTheme) {
          LookAndFeel oldLAF = UIManager.getLookAndFeel();
          boolean oldDecorated = false;
          if (oldLAF instanceof MetalLookAndFeel) {
              oldDecorated = true;
          }
          if (oldLAF instanceof AbstractLookAndFeel) {
              oldDecorated = AbstractLookAndFeel.isWindowDecorationOn();
          }

          setDebug(" setLookAndFeel "+laf);
          if (defaultTheme) {
               setDefaultTheme(laf);
          }

          String className = getLAFClassName(laf);
          try {
                UIManager.setLookAndFeel(className);
          }
          catch (Exception e){
                System.out.println("error changing LAF :"+e);
                return;
          }

          setArrowButton(laf);
          setBumpIcons(laf);
          LookAndFeel newLAF = UIManager.getLookAndFeel();
          boolean newDecorated = false;
          if (newLAF instanceof MetalLookAndFeel) {
              newDecorated = true;
          }
          if (newLAF instanceof AbstractLookAndFeel) {
              newDecorated = AbstractLookAndFeel.isWindowDecorationOn();
          }
          if (oldDecorated != newDecorated) {
             // rebuild UI?
          }
    }

    public static void setLookAndFeel(String laf) {
           setLookAndFeel(laf, false);
    }

    public static void updateUI() {
           Window windows[] = Window.getWindows();
           setDebug(" updateUI start");
           for (int i = 0; i < windows.length; i++) {
                if (windows[i].isDisplayable()) {
                     SwingUtilities.updateComponentTreeUI(windows[i]);
                }
           }
           setDebug(" updateUI done ");
    }

    public static boolean isJavaInstalledLAF(String laf) {
          if (laf == null || laf.equalsIgnoreCase("system"))
              return true;
          if (laf.equalsIgnoreCase("vnmrj"))
              return true;
          
          if (sysLAFlists == null)
              sysLAFlists = UIManager.getInstalledLookAndFeels();
          for (int i = 0; i < sysLAFlists.length; i++) {
              if (laf.equals(sysLAFlists[i].getName()))
                   return true;
          }
          return false;
    }

    public static void setPanelBg(Color bg) {
           UIManager.put("Panel.background", bg);
           UIManager.put("ScrollPane.background", bg);
           UIManager.put("OptionPane.background", bg);
           UIManager.put("ProgressBar.background", bg);
           UIManager.put("Separator.background", bg);
           UIManager.put("ScrollBar.foreground", bg);
           UIManager.put("Viewport.background", bg);
           UIManager.put("Slider.background", bg);
           UIManager.put("Spinner.background", bg);
           UIManager.put("SplitPane.background", bg);
           UIManager.put("TabbedPane.background", bg);
           UIManager.put("TableHeader.background", bg);
           UIManager.put("ToolBar.background", bg);
    }

    public static ColorUIResource getUiColor(String name) {
           if (name == null)
               return null;
           Color c = DisplayOptions.getVJColor(name);
           if (c == null) {
               System.out.println("  ui color  "+name+"  not found ");
               return null;
           }
           int r = c.getRed();
           int g = c.getGreen();
           int b = c.getBlue();

           ColorUIResource uiColor = new ColorUIResource(r, g, b); 
           return uiColor;
    }

    public static void setCustomTheme(String laf) {
    	  Properties props = new Properties();
    	  props.put("logoString", "");
          if (laf.equalsIgnoreCase("hifi") || laf.equalsIgnoreCase(DARK)) {
              vnmr.vplaf.jtattoo.hifi.HiFiLookAndFeel.setTheme(props);
              return;
          }
          if (laf.equalsIgnoreCase("aluminum") || laf.equalsIgnoreCase(LIGHT)) {
              vnmr.vplaf.jtattoo.aluminium.AluminiumLookAndFeel.setTheme(props);
              return;
          }
    }

    public static void addCustomTheme(String laf) {
          VAbstractThemeIF theme = null;
          boolean bCustomUiColor = DisplayOptions.isCustomUIColorEnabled();

          AbstractTheme.enableCustomUIColor(bCustomUiColor);
          if (!bCustomUiColor)
              return;

          if (laf.equalsIgnoreCase("hifi") || laf.equalsIgnoreCase(DARK))
              theme = (VAbstractThemeIF) new vnmr.vplaf.jtattoo.hifi.HiFiDefaultTheme();
          else if (laf.equalsIgnoreCase("aluminum") || laf.equalsIgnoreCase(LIGHT))
              theme = (VAbstractThemeIF) new vnmr.vplaf.jtattoo.aluminium.AluminiumDefaultTheme();
          if (theme == null)
               return;

          ColorUIResource uiColor = getUiColor("VJBackground");
          if (uiColor != null)
               theme.setDefaultBg(uiColor);
          uiColor = getUiColor("VJTextFG");
          if (uiColor != null)
              theme.setDefaultFg(uiColor);
          uiColor = getUiColor("VJCtrlHighlight");
          if (uiColor != null)
               theme.setHilightFg(uiColor);
          uiColor = getUiColor("VJDisabledBG");
          if (uiColor != null)
               theme.setDisableBg(uiColor);
          uiColor = getUiColor("VJDisabledFG");
          if (uiColor != null)
               theme.setDisableFg(uiColor);
          uiColor = getUiColor("VJEntryBG");
          if (uiColor != null)
               theme.setInputBg(uiColor);
          uiColor = getUiColor("VJGrid");
          if (uiColor != null)
               theme.setGridFg(uiColor);
          uiColor = getUiColor("VJInactiveBG");
          if (uiColor != null)
               theme.setInactiveBg(uiColor);
          uiColor = getUiColor("VJInactiveFG");
          if (uiColor != null)
               theme.setInactiveFg(uiColor);
          uiColor = getUiColor("VJMenubarBG");
          if (uiColor != null)
               theme.setMenuBarBg(uiColor);
          uiColor = getUiColor("VJSelectedBtn");
          if (uiColor != null)
               theme.setSelectedBg(uiColor);
          uiColor = getUiColor("VJSelectedText");
          if (uiColor != null)
               theme.setSelectedFg(uiColor);
          uiColor = getUiColor("VJSeparator");
          if (uiColor != null)
               theme.setSeparatorBg(uiColor);
          uiColor = getUiColor("VJShadow");
          if (uiColor != null)
               theme.setShadowFg(uiColor);
          uiColor = getUiColor("VJTabPaneBG");
          if (uiColor != null)
               theme.setTabBg(uiColor);
          uiColor = getUiColor("VJFocused");
          if (uiColor != null)
               theme.setFocusBg(uiColor);
          // uiColor = getUiColor("VJToolbarBG");
          // if (uiColor != null)
          //     theme.setDefaultFg(uiColor);
          theme = null;
    }
}

