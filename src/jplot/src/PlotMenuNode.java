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
/*
 * 
 *
 */

public class PlotMenuNode
{
   public PlotMenuNode(String s, String v)
   {
	cmd = s;
	value = v;
   }

   public String getCommand()
   {
	return cmd;
   }

   public String getValue()
   {
	return value;
   }

   private String cmd, value;
}


