/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
/**
 *
 * @author Frits
 */
package accounting;

import java.awt.*;
import java.io.*;
import java.text.*;
import java.util.*;
import javax.swing.*;



public class CSVOneBill {
    private AccountInfo2 ai;
    private AProps      aProps;
    private ArrayList   lines;
    private String[]    _summaryLine = new String[3];
    private String      xGlobal = null, xGlobal2 = null;
    public  StringTokenizer st = null;
    
    private double      total;
    private int         k=0, nOAndOs=0;
    
    public CSVOneBill(BufferedWriter bw, AccountInfo2 ai) {
        ArrayList tmpList;
        String str;
        this.ai = ai;
        aProps = AProps.getInstance();
        st = new StringTokenizer(ai.oAndOs());
        st.nextToken(); // skip "operator "
        nOAndOs = st.countTokens();
        total = 0.0;
        if (nOAndOs != 0) {
          lines = new ArrayList();
          for (int i=0; i<nOAndOs; i++) {
            str = st.nextToken();
            tmpList = ReadLast.getInstance(ai).getLines(str);
            total += ReadLast.getInstance(ai).total();
//            System.out.println("Adding "+tmpList.size()+" entries to lines");
            lines.addAll(tmpList);
          }
        }
        placeAddress(bw);
        placeSummary(bw);
        _summaryLine[0] = new String(ai.account());
        
        try {
            bw.newLine();
            bw.write("\"logged in at\",");
            bw.write("\"for [dd+hh:mm:ss]\",");
            bw.write("\"charge\"");
            bw.newLine();
            
            if ( (lines == null) || (lines.size() == 0) ) {
                bw.write("----------,----------,----------,----------");
                bw.newLine();
                return;
            }
            String y,onDate;
            Double tmpD;
            while ( k < lines.size() ) {
                xGlobal = (String)lines.get(k);     k++;    //name
                onDate = (String)lines.get(k);      k++;   // date one
                bw.write("\""+xGlobal+": "+onDate+"\",");
                y = (String)lines.get(k);     k++;    // how long
                bw.write("\""+y+"\",");
                tmpD = new Double((ai.getRate(onDate)).loginhr);
                y = (String)lines.get(k);     k++;
                bw.write("\""+y+"\"");
                bw.newLine();
          //      System.out.println(xGlobal+" x "+y + " " + k + "  " +lines.size()+ " "+nOAndOs);
            }
            bw.write("----------,----------,----------,----------");
            bw.newLine();
        } catch (IOException ioe) {
            ioe.getStackTrace();
        }
    }
    
    private void placeAddress(BufferedWriter bw) {
        try {
            bw.write("\""+ai.name()+"\"");
            bw.newLine();
            bw.write("\""+ai.address1()+"\"");
            bw.newLine();
            if (ai.address2().compareTo("") != 0) {
              bw.write("\""+ai.address2()+"\"");
              bw.newLine();
            }
            bw.write("\""+ai.city()+", "+ai.zip()+"\"");
            bw.newLine();
            bw.newLine();
        } catch (IOException ioe) {
            ioe.getStackTrace();
        }
    }
    
    private void placeSummary(BufferedWriter bw) {
        try {
            bw.write("\"ACCOUNT NUMBER\",");
            bw.write("\"BILLING DATE\",");
            bw.write("\"DUE DATE\",");
            bw.write("\"AMOUNT DUE\"");
            bw.newLine();

            bw.write("\""+ai.account()+"\",");
            StringBuffer date = new StringBuffer();
            GregorianCalendar gc = new GregorianCalendar();
            SimpleDateFormat sdf = new SimpleDateFormat("MMM d,yyyy");
            sdf.format(gc.getTime(),date,new FieldPosition(0));
            bw.write("\""+date.toString()+"\",");
            _summaryLine[1] = date.toString();
            gc.add(Calendar.DATE,31);
            date = new StringBuffer();
            sdf.format(gc.getTime(),date,new FieldPosition(0));
            bw.write("\""+date.toString()+"\",");
            placeTotal(bw);
            bw.newLine();
        } catch (IOException ioe) {
            ioe.getStackTrace();
        }
    }
    
    private void placeTotal(BufferedWriter bw) throws IOException {
        DecimalFormat df = new DecimalFormat((String)aProps.get("numberFormat"));
        String y, str;
        double c, m, r;

        String country = (String)aProps.get("currency");
        Locale l = new Locale( country );
        String lang = l.getLanguage();
        l = new Locale(lang,country);
        NumberFormat nf = NumberFormat.getCurrencyInstance(l);
    //    System.out.println("df2 has '"+nf.getCurrency()+"'");
        Currency newC = Currency.getInstance(nf.getCurrency().toString());
        nf.setCurrency(newC);
        y = nf.format( total );
        _summaryLine[2] = y;
        bw.write("\""+y+"\"");
  }
    
  public  String[]    summaryLine() {
      return _summaryLine; 
  }
    
}
