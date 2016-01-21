/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.print;

import java.awt.Component;
import java.io.*;
import java.util.ResourceBundle;
import javax.print.*;
import javax.swing.JComboBox;
import javax.print.attribute.*;
import javax.swing.BorderFactory;
import javax.swing.border.Border;
import javax.swing.border.EtchedBorder;

import vnmr.util.FileUtil;


public class VjPrintUtil {

   private static VjPlotterTable plotterTable;
   private static VjPlotterType plotterType;
   private static VjPlotterConfig plotterConfig;
   private static String[] psList;
   private static String defaultPrinterName = null;
   private static ResourceBundle messageRB;
   private static boolean bVerbose = false;
   private static boolean bTest = false;
   private static boolean bWaitForPrinterServer = false;
   private static PrintService[] printSrvs = null;
   private static long lastQueryTime = 0;
   private static long lastQueryTime_all = 0;

   public static Object getAttributeValues(PrintService ps,
                             Class<? extends Attribute> category,
                             DocFlavor flavor, AttributeSet attributes)
    {
        Object obj = null;

        if (ps == null || !isGoodPrinterName(ps.getName()))
            return obj;
        try {
             obj = ps.getSupportedAttributeValues(category, flavor,attributes);
        }
        catch (Exception e) {
            obj = null;
        }
        return obj;
    }

    public static boolean  isValueSupported(PrintService ps, Attribute attr,
                              DocFlavor flavor, AttributeSet attributes)
    {
        boolean retVal = false;

        if (ps == null || !isGoodPrinterName(ps.getName()))
            return retVal;

        try {
             retVal = ps.isAttributeValueSupported(attr,flavor,attributes);
        }
        catch (Exception e) {
            retVal = false;
        }
        return retVal;
    }

    public static boolean isCategorySupported(PrintService ps, 
                                Class<? extends Attribute> category)
    {
        boolean retVal = false;
        if (ps == null || !isGoodPrinterName(ps.getName()))
            return retVal;
        try {
             retVal = ps.isAttributeCategorySupported(category);
        }
        catch (Exception e) {
            retVal = false;
        }
        return retVal;
    }

    public static boolean setComboxSelectItem(JComboBox cb, String s)
    {
        int num = cb.getItemCount();
        if (s == null || num < 1)
            return false;
        String v = (String) cb.getSelectedItem();
        if (v.equals(s))
            return false;

        for (int i = 0; i < num; i++) {
            v = (String) cb.getItemAt(i);
            if (v.equals(s)) {
               cb.setSelectedIndex(i);
               return true;
            }
        }
        return false;
    }

    public static boolean isIntegerString(String s) {
        if (s == null || s.length() < 1)
             return false;
        try {
        } catch (NumberFormatException er) {
            return false;
        }
        return true;
    }

    public static boolean isDoubleString(String s) {
        if (s == null || s.length() < 1)
             return false;
        try {
        } catch (NumberFormatException er) {
            return false;
        }
        return true;
    }

    public static double getDouble(String s) {
        double d = 0.0;
        if (s == null || s.length() < 1)
            return d;

        try {
             d = Double.parseDouble(s);
        } catch (NumberFormatException er) {
             d = 0.0;
        }
        return d;
    }

    public static int getInteger(String s) {
        int d = 0;
        if (s == null || s.length() < 1)
            return d;

        try {
             d = Integer.parseInt(s);
        } catch (NumberFormatException er) {
             d = 0;
        }
        return d;
    }

    // set to one digit precision
    public static double getDouble(double d) {
       int n = (int) ((d + 0.05)*10.0);
       double d1 = ((double) n) / 10.0;
       return d1;
    }

    public static double getDouble2(double d) {
       int n = (int) ((d + 0.005)*100.0);
       double d1 = ((double) n) / 100.0;
       return d1;
    }

    public static double getDouble3(double d) {
       int n = (int) ((d + 0.0005)*1000.0);
       double d1 = ((double) n) / 1000.0;
       return d1;
    }

    // set to one digit precision
    public static double getDouble(float f) {
       int n = (int) ((f + 0.05f) * 10.0f);
       double d = ((double) n) / 10.0;
       return d;
    }

    public static boolean isGoodPrinterName(String name) {
        if (name == null)
            return false;
        if (name.equals("jeff_pl") || name.equals("jeff_pr"))
            return false;
        if (name.equals("steve_pl") || name.equals("steve_pr"))
            return false;

        return true;
    }

    public static Object getDefaultAttributeValue(PrintService ps,
                                Class<? extends Attribute> category) {
        if (ps == null || !isGoodPrinterName(ps.getName()))
            return null;
        return ps.getDefaultAttributeValue(category);
    }

    public static ServiceUIFactory getServiceUIFactory(PrintService ps) {
        if (ps == null || !isGoodPrinterName(ps.getName()))
            return null;
        return ps.getServiceUIFactory();
    }

    public static PrintService[] lookupPrintServices() {
        if (bWaitForPrinterServer)
            return printSrvs;

        bWaitForPrinterServer = true;
        if (printSrvs == null || printSrvs.length < 1) {
            long t = System.currentTimeMillis() / 1000;
            if ((t - lastQueryTime_all) < 120) {
                bWaitForPrinterServer = false;
                return printSrvs;
            }
            printSrvs = PrintServiceLookup.lookupPrintServices(null, null);
            lastQueryTime_all = System.currentTimeMillis() / 1000;
        }
        bWaitForPrinterServer = false;
        return printSrvs;
    }

    public static <T extends PrintServiceAttribute> T getAttribute(
                       PrintService ps, Class<T> category) {
        if (ps == null || !isGoodPrinterName(ps.getName()))
            return null;
        return ps.getAttribute(category);
    }

    public static String getDefaultPrinterName() {
        if (bWaitForPrinterServer)
            return defaultPrinterName;
        bWaitForPrinterServer = true;
        if (defaultPrinterName == null) {
            long t = System.currentTimeMillis() / 1000;
            if ((t - lastQueryTime) < 120) {
                bWaitForPrinterServer = false;
                return defaultPrinterName;
            }
            PrintService ps = PrintServiceLookup.lookupDefaultPrintService();
            lastQueryTime = System.currentTimeMillis() / 1000;
            String name = null;
            if (ps != null) {
               name = ps.getName();
               if (name != null && name.length() > 0)
                   defaultPrinterName = name;
            }
        }
        bWaitForPrinterServer = false;
        return defaultPrinterName;
    }

    public static String getSaveDefaultPrinterName() {
        getDefaultPrinterName();
        lookupPrintServices();

        if (defaultPrinterName == null)
            return null;

        String path = FileUtil.savePath("USER/PERSISTENCE/"+".defaultprinter", false);
        if (path == null)
            return defaultPrinterName;
        PrintWriter os = null;
        try {
            os = new PrintWriter(path);
            os.println(defaultPrinterName);
        }
        catch(IOException er) { }
        finally {
            if (os != null)
               os.close();
        }

        return defaultPrinterName;
    }

    public static String[] lookupPrintNames(PrintService[] prtSrvs) {
        int i, n;
        int total = 0;
        String[] psnames;
        int nums = 6;
        String name;

        if (prtSrvs != null) {
            nums += prtSrvs.length;
        }
        psnames = new String[nums];
        if (prtSrvs != null) {
            for (n = 0; n < prtSrvs.length; n++) {
               if (prtSrvs[n] != null) {
                   name = prtSrvs[n].getName();
                   if (name != null) {
                        if (isGoodPrinterName(name))
                            psnames[total++] = name;
                   }
               }
            }
        }
        String cupsPath = new StringBuffer().append(File.separator).append("etc").
            append(File.separator).append("cups").append(File.separator).
            append("ppd").toString();
        File dir = new File(cupsPath);
        File files[] = null;
        if (dir.exists() && dir.isDirectory())
            if (dir.canRead())
                files = dir.listFiles();
        if (files != null) {
            for (i = 0; i < files.length; i++) {
                if (files[i].isFile()) {
                    String fname = files[i].getName();
                    if (fname.endsWith(".ppd")) {
                        name = fname.substring(0, fname.indexOf(".ppd"));
                        if (name.length() > 0) {
                           for (n = 0; n < nums; n++) {
                               if (psnames[n] != null) {
                                   if (name.equals(psnames[n])) {
                                       name = null;
                                       break;
                                   }
                               }
                           }
                           if (name != null && total < nums) {
                               psnames[total++] = name;
                           }
                        }
                    }
                    if (total >= nums)
                        break;
                }
            }
        }
        if (total > 0) {
            psList = new String[total];
            for (i = 0; i < total; i++)
               psList[i] = psnames[i];
        }
      
        return psList;
    }

    public static String[] lookupPrintNames() {
        lookupPrintServices();
        return lookupPrintNames(printSrvs);
    }

    public static void setPlotterTable(VjPlotterTable table) {
        plotterTable = table;
    }

    public static VjPlotterTable getPlotterTable() {
        return plotterTable;
    }

    public static void setVnmrPlotter(String name) {
        if (plotterTable != null)
            plotterTable.setVnmrPlotter(name);
    }

    public static void setVnmrPrinter(String name) {
        if (plotterTable != null)
            plotterTable.setVnmrPrinter(name);
    }

    public static void setPlotterType(VjPlotterType table) {
         plotterType = table;
    }

    public static VjPlotterType getPlotterType() {
         return plotterType;
    }
   

    public static void setPlotterConfig(VjPlotterConfig pan) {
        plotterConfig = pan;
    }

    public static VjPlotterConfig getPlotterConfig() {
        return plotterConfig;
    }

    public static void showConfigTab(Component comp) {
        if (plotterConfig != null)
            plotterConfig.showTab(comp);
    }

    public static String getResourceValue(String key) {
        if (key == null)
            return null;
        if (messageRB == null) {
            messageRB = ServicePopup.messageRB;
            if (messageRB == null)
                return null;
        }
        String newkey = key.replace(' ', '-');
        newkey = newkey.replace('#', 'n');
        try {
            return messageRB.getString(newkey);
        } catch (java.util.MissingResourceException e) {
           // throw new Error("Fatal: Resource for ServiceUI is broken; " +
           //                 "there is no " + key + " key in resource");
        }
        return null;
    }


    public static String getMediaResource(String key, String replaceKey) {
        String name = getResourceValue(key);
        if (name == null)
            return replaceKey;
        return name;
    }

    public static VjMediaSizeObj setMediaComboxSelectItem(JComboBox cb, String s)
    {
        int num = cb.getItemCount();
        if (s == null || num < 1)
            return null;
        VjMediaSizeObj obj = (VjMediaSizeObj) cb.getSelectedItem();
        if (obj.getMediaName().equals(s) || obj.getName().equals(s))
            return obj;

        for (int i = 0; i < num; i++) {
            obj = (VjMediaSizeObj) cb.getItemAt(i);
            if (obj.getMediaName().equals(s) || obj.getName().equals(s)) {
               cb.setSelectedIndex(i);
               return obj;
            }
        }
        return null;
    }

    public static void setVerbose(boolean b) {
        bVerbose = b;
    }

    public static void printMessage(String str) {
        if (bVerbose)
            System.out.println(str);
    }
    
    public static double getPclDpi(double d) {
        int n;
        int m = VjPlotDef.pclResolutions.length;

        if (m <= 1)
            return d;
        for (n = 0; n < m; n++) {
            if (d <= VjPlotDef.pclResolutions[n])
                break;
        }
        if (n >= m)
            n = m - 1;
        if (n <= 0)
            return VjPlotDef.pclResolutions[n];
        double d1 = Math.abs(VjPlotDef.pclResolutions[n] - d);
        double d2 = Math.abs(VjPlotDef.pclResolutions[n-1] - d);
        if (d1 > d2)
            n = n - 1;
        return VjPlotDef.pclResolutions[n];
        
    }

    public static Border createTitledBorder(String title) {
        return BorderFactory.createTitledBorder(new EtchedBorder(), title);
    }

    public static boolean isTestMode() {
        return bTest;
    }

    public static void setTestMode(boolean b) {
        bTest = b;
    }

} // end of VjPrintUtil

