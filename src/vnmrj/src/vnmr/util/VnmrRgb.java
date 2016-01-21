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
import java.util.StringTokenizer;

public final class VnmrRgb {

   public VnmrRgb() {
   }

    public static Color getColorByName(String name) {
        int   k;
        if(name==null)
            return Color.red;
        if (name.equalsIgnoreCase("null")) {
            return null;
        }
        for (k = 0; k < 41; k++)
        {
            if (name.equals(nameList[k]))
            {
                color = new Color(redList[k], greenList[k], blueList[k]);
                return color;
            }
        }
        try {
            // Not a known color name.
            int icolor =0;
            // check for rgb triplet syntax: red,green,blue
            if(name.contains(",")) {
                int red=0; 
                int grn=0;
                int blu=0;
                String symbol;
                StringTokenizer tok= new StringTokenizer(name, ",");
                if (tok.hasMoreTokens()){
                    symbol = tok.nextToken();
                    red=(Integer.parseInt(symbol, 10))&0xFF;
                    if (tok.hasMoreTokens()){
                        symbol = tok.nextToken();
                        grn=(Integer.parseInt(symbol, 10))&0xFF;
                        if (tok.hasMoreTokens()){
                            symbol = tok.nextToken();
                            blu=(Integer.parseInt(symbol, 10))&0xFF;
                        }
                    }
                }
                icolor=(red<<16)+(grn<<8)+blu;
            } else {
                // Maybe a hex RGB color number? 
                if (name.startsWith("0x") || name.startsWith("0X")) {
                    name = name.substring(2);
                }
                icolor = Integer.parseInt(name, 16);
            }
            color = new Color(icolor);
                
        } catch (NumberFormatException exception) {
            color = Color.red;
        }
        return color;
    }
    
    public static Color getColorByIndex(int i) {
        if(i<0 || i>40)
            return Color.red;
        int red=redList[i];
        int grn=greenList[i];
        int blu=blueList[i];
        Color c=new Color(red, grn, blu);
        return c;
    }

    public static String getNameByIndex(int i) {
        if(i<0 || i>40)
            return "red";
        return (nameList[i]);
    }

    public static String getColorName(int  n) {
        if (n == 0)
            name_num = 0;
        name_num++;
        if (name_num > 41)
            return null;
        return (nameList[name_num - 1]);
    }

    public static String[] getColorNameList() {
        return nameList;
    }

    public static String getNameByRGB(String str) {
        if(str.contains(",")) {
            try {
                 int red=0;
                 int grn=0;
                 int blu=0;
                 String symbol;
                 StringTokenizer tok= new StringTokenizer(str, ",");
                 if (tok.hasMoreTokens()){
                     symbol = tok.nextToken();
                     red=(Integer.parseInt(symbol, 10))&0xFF;
                     if (tok.hasMoreTokens()){
                          symbol = tok.nextToken();
                          grn=(Integer.parseInt(symbol, 10))&0xFF;
                          if (tok.hasMoreTokens()){
                                symbol = tok.nextToken();
                                blu=(Integer.parseInt(symbol, 10))&0xFF;
                          }
                     }
                 }
                 for (int k = 0; k < 41; k++)
                 {
                   if (red == redList[k] && grn == greenList[k] && blu == blueList[k])
		       return nameList[k];
                 }
	    } catch (NumberFormatException e ) {}
	}
        return str;
    }

    private static Color color;
    private static int   name_num = 0;

    static String[] nameList =
    {"beige", "bisque", "black", "blue", "brown", "burlywood",
     "chocolate", "coral", "cyan", "darkGray", "darkGreen", "darkRed",
     "gold", "goldenrod", "gray", "green", "ivory", "lightGray",
     "linen", "magenta", "maroon", "moccasin", "navy", "orange",
     "orchid", "peru", "pink", "plum", "purple", "red",
     "salmon", "seaGreen", "seashell", "sienna", "slateBlue", "snow",
     "tan", "tomato", "violet", "white", "yellow" };

    static int[] redList =
    { 245, 255, 0, 0, 165, 222,
      210, 255, 0, 150, 0, 139,
      255, 218, 190, 0, 240, 211,
      250, 255, 176, 255, 0, 255,
      218, 205, 255, 221, 160, 255,
      250, 46, 255, 160, 106, 255,
      210, 255, 238, 255, 255 };

    static int[] greenList =
    { 245, 228, 0, 0, 42, 184,
      105, 127, 255, 150, 100, 0,
      215, 165, 190, 255, 255, 211,
      240, 0, 48, 228, 0, 165,
      112, 133, 192, 160, 32, 0,
      128, 139, 245, 82, 90, 250,
      180, 99, 130, 255, 255 };

    static int[] blueList =
    { 220, 196, 0, 255, 42, 135,
      30, 80, 255, 150, 0, 0,
      0, 32, 190, 0, 240, 211,
      230, 255, 96, 181, 128, 0,
      214, 63, 203, 221, 240, 0,
      114, 87, 238, 45, 205, 250,
      140, 71, 238, 255, 0 };
}
