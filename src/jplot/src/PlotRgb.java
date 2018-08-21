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

import java.awt.*;

public final class PlotRgb
{
   public PlotRgb()
   {
   }

   public static Color getColorByName(String name)
   {
	int   k;

	for (k = 0; k < 41; k++)
	{
	    if (name.equals(nameList[k]))
	    {
		color = new Color(redList[k], greenList[k], blueList[k]);
		return color;
	    }
	}
	color = Color.black;
	return color;
   }

   public static String getColorName(int  n)
   {
	if (n == 0)
	    name_num = 0;
	name_num++;
	if (name_num > 41)
	   return null;
	return (nameList[name_num - 1]);
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
	{ 245, 255, 0, 0, 165, 222, 210, 255, 0, 169, 0,
	 139, 255, 218, 190, 0, 240, 211, 250, 255, 176, 255,
	 0, 255, 218, 205, 255, 221, 160, 255, 250, 46, 255, 160,
	 106, 255, 210, 255, 238, 255, 255 };

   static int[] greenList =
	{ 245, 228, 0, 0, 42, 184, 105, 127, 255, 169, 100,
	  0, 215, 165, 190, 255, 255, 211, 240, 0, 48, 228,
	  0, 165, 112, 133, 192, 160, 32, 0, 128, 139, 245, 82,
	  90, 250, 180, 99, 130, 255, 255 };

   static int[] blueList =
	{ 220, 196, 0, 255, 42, 135, 30, 80, 255, 169, 0,
	  0, 0, 32, 190, 0, 240, 211, 230, 255, 96, 181,
	  128, 0, 214, 63, 203, 221, 240, 0, 114, 87, 238, 45,
	  205, 250, 140, 71, 238, 255, 0 };
}

