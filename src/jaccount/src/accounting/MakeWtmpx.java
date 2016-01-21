/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package accounting;

import java.awt.*;
import java.io.*;
import java.text.*;
import java.util.*;
import javax.swing.*;
import javax.swing.text.*;

public class MakeWtmpx {

  private BufferedReader     wtmpxIn;
  private NumberFormat      df;
  private static MakeWtmpx   makeWtmpx=null;
  private AProps             aProps = AProps.getInstance();
  private String             caName,caHost,caId,caPid,caType,caSec;
  private Vector             wtmpxs = new Vector();
  private static int         record;

  public static void main(String[] args) {
    MakeWtmpx makeWtmpx1 = new MakeWtmpx();
  }
  private MakeWtmpx() {
    int nchars=0, k=0;
    Wtmpx oneWtmpx;
    char[] ca = new char[372];

    File out = new File("tmpx/wtmpx.Q");

    try {
      BufferedReader br = new BufferedReader ( new FileReader(out));
      while (true) {
        nchars = br.read(ca,0,372);
        if (nchars < 0) break;
        k++;
          for (int j=0; j<32; j++) {
            System.out.print(ca[j]);
          }
          System.out.println();
          for (int j=68; j<118; j++) {
            if (j%32==0) System.out.println();
            System.out.print(" "+(int)ca[j]+" ");
          }
          System.out.println();

        oneWtmpx = new Wtmpx(); oneWtmpx.fromChars(ca);
        wtmpxs.add(oneWtmpx);
      }
      br.close();
    } catch(IOException ioe) {
      ioe.getStackTrace();
    }
    System.out.print("Read "+k+" records from "+out.getAbsolutePath()+"'");
    System.out.println();
    df = new DecimalFormat("# 0.00");
    
    
    System.out.println("Frits: '"+onRecord("frits")+"'");
    System.out.println("       '"+offRecord()+"'");
    System.out.println("Frits: '"+onRecord("frits")+"'");
    System.out.println("       '"+offRecord()+"'");
  }

  public static MakeWtmpx getInstance() {
    if (makeWtmpx == null) {
      makeWtmpx = new MakeWtmpx();
    }
    record = 0;
    return(makeWtmpx);
  }

  int onPid, onTime;
  Wtmpx onWtmpx;
  Date onDate;
  public Date onRecordDate() {
    return(this.onDate);
  }

  public String onRecord(String name) {
    String caName=null, caId, resultStr;
    Wtmpx thisWtmpx = null;
    int caType;
    int k=record;
//    System.out.println("onRecord(): k="+k);
    while (k<wtmpxs.size()) {
      thisWtmpx = (Wtmpx)wtmpxs.get(k);
      caName = new String(thisWtmpx.ut_user).trim();
      caType = (new Integer(thisWtmpx.ut_type)).intValue();
      caId   = new String(thisWtmpx.ut_id).trim();
//      System.out.println("     caId="+caId);
      if ( (caName.compareTo(name))==0 && (caType==7) && ! caId.startsWith("ftp")) break;
      k++;
    }
    if (k>=wtmpxs.size()) return(null);
    record = k;
    onWtmpx = thisWtmpx;
    onTime = thisWtmpx.ut_tv.tv_sec;
    onDate = new Date((long)onTime*(long)1000);
    resultStr = new String(caName+" log on: " + onDate + "; ");
    onPid = new Integer(thisWtmpx.ut_pid).intValue();
    record++;
    return(resultStr);
  }

  public double offRecord() {
    String caName=null, tmpStr=null;
    StringBuffer sb = null;
    Wtmpx thisWtmpx=null;
    int tmp, k, pid, caType;
    k=record;
    while (k <wtmpxs.size()) {
      thisWtmpx = (Wtmpx)wtmpxs.get(k);
      caName = new String(thisWtmpx.ut_user).trim();
      caType = new Integer(thisWtmpx.ut_type).intValue();
      pid = new Integer(thisWtmpx.ut_pid).intValue();
      if ((caType==8) && (pid==onPid)) break;
      k++;
    }
    if (k>=wtmpxs.size()) {
      sb = new StringBuffer("logout record not found");
      return(0.0);
    }
    sb = new StringBuffer("log off " + new Date((long)thisWtmpx.ut_tv.tv_sec*(long)1000)+"; ");
    tmp = thisWtmpx.ut_tv.tv_sec - onTime;
//    tmpStr = df.format(tmp/60.0);
//    sb = new StringBuffer(tmpStr);
    return ((double)(tmp/60.));
  }

}