/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.sms;

import java.io.*;
import java.util.*;

public class GrkParser implements SmsDef
{
   private GrkCompose rootNode;
   private GrkCompose curNode;
   private String     curName;

   public GrkParser() {
   }

   
   public GrkCompose build(BufferedReader f) {
      String data, d;
      int p1;

      rootNode = new GrkCompose(null, "xyRoot");
      curNode = rootNode;
      curName = null;
      try {
        while ((data = f.readLine()) != null) {
           d = data.trim();
           if (d.length() < 2)
               continue;
           if (d.startsWith("[Geometry")) {
               if ( ! d.endsWith("1.00]") && ! d.endsWith("1.00p]") ) {
                  System.out.println("Geometry version not 1.00");
                  return null;
               } 
               continue;
           }
           if (d.startsWith("[REM]") || d.startsWith("[Rem]"))
               continue;
           if (d.startsWith("[")) {
               if (curName != null) {
                  if (d.startsWith(curName) && curNode != null) {
                      curNode = curNode.getParent();
                      curName = curNode.name;
                      continue;
                  }
               }
               p1 = d.indexOf(']');
               if (p1 <= 1)    
                   continue;
               curName = d.substring(0, p1+1);
               GrkCompose newNode = new GrkCompose(curNode, curName);
               curNode.addCompose(newNode);
               curNode = newNode;
               continue;
           }
           if (curNode == null)
               continue;
           if (d.indexOf('[') < 0 )
               createHole(d);
           else
               createRow(d);
        }
      }
      catch(IOException e)
      {}
       return rootNode;
   }

   private void createHole(String s) {
       float x, y, diam;
       StringTokenizer tok = new StringTokenizer(s, " ,\t\r\n");
       if (!tok.hasMoreTokens())
            return;
       String d = tok.nextToken();
       try {
          x = Float.parseFloat(d);
          if (!tok.hasMoreTokens())
               return;
          d = tok.nextToken();
          y = Float.parseFloat(d);
          if (!tok.hasMoreTokens())
               return;
          d = tok.nextToken();
          diam = Float.parseFloat(d);
       }
       catch (NumberFormatException nfe ) {
          return;
       }

       curNode.addObj(x, y, diam);
   }

   private void createRow(String s) {
       float x, y;
       int p1, p2;
       p1 = s.indexOf('[');
       p2 = s.indexOf(']');
       if (p2 <= p1)
          return;
       StringTokenizer tok = new StringTokenizer(s, " ,\t\r\n");
       if (!tok.hasMoreTokens())
            return;
       String d = tok.nextToken();
       try {
           x = Float.parseFloat(d);
           if (!tok.hasMoreTokens())
                return;
           d = tok.nextToken();
           y = Float.parseFloat(d);
       }
       catch (NumberFormatException nfe ) {
          return;
       }

       d = s.substring(p1, p2+1);
       curNode.addObj(x, y, d);
   }
}

