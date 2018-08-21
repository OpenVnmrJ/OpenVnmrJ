/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.sms;

import java.util.*;


public class GrkCompose implements SmsDef
{
   public String name;
   public GrkCompose pnode;
   public Vector<GrkCompose> compseList;
   private Vector<GrkObj> objList;

   public GrkCompose(GrkCompose p, String s) {
       this.pnode = p;
       this.name = s;
       this.objList = new Vector<GrkObj>();
   }

   public GrkCompose(String s) {
       this(null, s);
   }

   public void setParent(GrkCompose p) {
       pnode = p;
   }

   public GrkCompose getParent() {
       return pnode;
   }

   public void addCompose(GrkCompose comp) {
       if (compseList == null)
          compseList = new Vector<GrkCompose>();
       compseList.add(comp);
   }

   public Vector<GrkCompose> getComposeList() {
       return compseList;
   }

   public void addObj(float x, float y, String str) {
       GrkObj obj = new GrkObj(x, y, str);
       objList.add(obj);
   }

   public void addObj(float x, float y, float d) {
       GrkObj obj = new GrkObj(x, y, d);
       objList.add(obj);
   }

   public Vector<GrkObj> getObjList() {
       return objList;
   }
}

