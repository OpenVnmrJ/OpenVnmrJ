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

public class ZoneCreater
{
   private GrkCompose rootNode;
   private GrkZone    zoneObj;

   public ZoneCreater() {
   }

   
   public GrkZone buildZone(int size, GrkObj gobj, GrkCompose compse) {
       float x = gobj.orgx;
       float y = gobj.orgy;
       rootNode = compse;
       zoneObj = new GrkZone(size, x, y);
       if (gobj.name == null)
           return zoneObj;
       Vector<GrkCompose> cList = compse.getComposeList();
       if (cList != null) {
          for (int i = 0; i < cList.size(); i++) {
              GrkCompose compose = (GrkCompose) cList.elementAt(i);
              if (compose.name != null && compose.name.equals(gobj.name)) {
                  addComposeObj(compose, 0, 0);
                  break;
              }
          }
       }
       return zoneObj;
   }

   public void addComposeObj(GrkCompose c, float x, float y) {
       Vector<GrkObj> oList = c.getObjList();
       if (oList == null)
           return;

       for (int i = 0; i < oList.size(); i++) {
           GrkObj obj = oList.elementAt(i);
           if (obj.name != null) {
               Vector<GrkCompose> cList = c.getComposeList();
               boolean found = false;
               if (cList != null) {
                  for (int k = 0; k < cList.size(); k++) {
                     GrkCompose compose = cList.elementAt(k);
                     if (compose.name != null && compose.name.equals(obj.name)) {
                        addComposeObj(compose, obj.orgx + x, obj.orgy + y);
                        found = true;
                        break;
                     }
                  }
               }
               if (!found) {
                  cList = rootNode.getComposeList();
                  if (cList != null) {
                     for (int m = 0; m < cList.size(); m++) {
                        GrkCompose comp = cList.elementAt(m);
                        if (comp.name != null && comp.name.equals(obj.name)) {
                           addComposeObj(comp, obj.orgx + x, obj.orgy + y);
                           break;
                        }
                     }
                  }
               }
           }
           else {
               addObj(obj, x, y);
           }
       }
   }

   public void addObj(GrkObj o, float x, float y) {
       SmsSample sample = new SmsSample(1);
       sample.orgx = o.orgx + x;
       sample.orgy = o.orgy + y;
       sample.diam = o.diam;
       zoneObj.sampleList.add(sample);
   }
}

