/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.sms;


public class GrkObj
{
   public String name;
   public float orgx, orgy, diam;
   public float x, y;

   public GrkObj(float x, float y, float d, String s) {
       this.orgx = x;
       this.orgy = y;
       this.diam = d;
       this.name = s;
   }
       
   public GrkObj(float x, float y, float d) {
       this(x,y, d, null);
   }

   public GrkObj(float x, float y, String s) {
       this(x, y, 0, s);
   }

}

