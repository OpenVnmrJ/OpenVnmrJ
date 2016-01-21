/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * Goes.java
 *
 * Created on September 3, 2005, 9:58 AM
 */

package accounting;

import java.io.*;
import java.text.*;
import java.util.*;
import org.xml.sax.*;
import org.xml.sax.helpers.*;

/**
 * @author Frits Vosman
 */
public class Goes {
    
    ArrayList goEntries;
  
    public static void main(String[] args) {
        Goes goes = new Goes();
    }
    
    /** Creates a new instance of Goes */
    public Goes() {
        XMLReader myReader;
        try {
            myReader = XMLReaderFactory.createXMLReader();
            MyHandler myContentEar = new MyHandler();
            myReader.setContentHandler(myContentEar);
            myReader.parse( new InputSource("C:/Documents and Settings/frits/My Documents/NetBeans/Adm/lastout/goes.xml"));
        } catch (IOException ioe) {
            System.err.println( ioe.getMessage());
        } catch (SAXException e) {
            System.err.println(e.getMessage());
        }
        
        for (int i=0; i<goEntries.size(); i++)
            System.out.println( "  "+i+" op='"+((Entry)goEntries.get(i)).operator+
                                "' go='"+((Entry)goEntries.get(i)).goflag+
                                "' sf='"+((Entry)goEntries.get(i)).seqfil+
                                "' sm='"+((Entry)goEntries.get(i)).submit+
                                "' st='"+((Entry)goEntries.get(i)).start+
                                "' dn='"+((Entry)goEntries.get(i)).done
                              );
        sum("vnmr1", "Fri Sep  2 12:19", "Fri Sep  2 14:10");
        
        
    }
    
    private void sum(String who, String from, String to) {
        if (from==null) return;
        if (to == null) return;
        SimpleDateFormat sdf = new SimpleDateFormat("EEE MMM d kk:mm yyyy");;
        ReadLast rl = ReadLast.getInstance(new AccountInfo2());
        ParsePosition pp = new ParsePosition(0);
        int thisYear = (new GregorianCalendar()).get(Calendar.YEAR);
        
        String newDateStr = new String(from+" "+Integer.toString(thisYear)); // this year?
        System.out.println("From '"+newDateStr+"'");
        Date fromDate = sdf.parse(newDateStr, pp);
        String xx = fromDate.toString();
        int i = pp.getErrorIndex();
        int j = pp.getIndex();
        if ( (xx.substring(0,3)).compareTo(from.substring(0,3)) != 0) {  
          newDateStr = new String(from + " "+Integer.toString(thisYear-1)); //no, then last year
          pp.setIndex(0);
          fromDate = sdf.parse(newDateStr, pp);
          xx = fromDate.toString();
          i = pp.getErrorIndex();
          j = pp.getIndex();
        }
        if ( (xx.substring(0,3)).compareTo(from.substring(0,3)) != 0) return; // neither year
//        System.out.println("From '"+xx+"'");
        
        
        newDateStr = new String(to+" "+Integer.toString(thisYear)); // this year?
//        System.out.println("To '"+newDateStr+"'");
        pp.setIndex(0);
        Date toDate = sdf.parse(newDateStr, pp);
        String yy = toDate.toString();
        i = pp.getErrorIndex();
        j = pp.getIndex();
        if ( (yy.substring(0,3)).compareTo(to.substring(0,3)) != 0) {  
          newDateStr = new String(to + " "+Integer.toString(thisYear-1)); //no, then last year
          pp.setIndex(0);
          toDate = sdf.parse(newDateStr, pp);
          yy = toDate.toString();
          i = pp.getErrorIndex();
          j = pp.getIndex();
        }
        if ( (yy.substring(0,3)).compareTo(to.substring(0,3)) != 0) return; // neither year
        System.out.println("From '"+xx+"' To '"+yy+"'");
        
        sdf = new SimpleDateFormat("EEE MMM d kk:mm:ss yyyy");
        Date submitDate, startDate, doneDate;
        int numGos=0;
        long numSecs=0, deltaSecs;
        for (i=0; i<goEntries.size(); i++) {
            Entry e = (Entry) goEntries.get(i);
            if (who.compareTo(e.operator)==0) {
//                System.out.println("Submit '"+e.submit+"'");
                pp.setIndex(0);
                submitDate = sdf.parse(e.submit,pp);
//                System.out.println("Submit2 '"+submitDate+"' "+pp.getErrorIndex());
                if ( (submitDate.compareTo(fromDate) > 0) && (submitDate.compareTo(toDate) <= 0) ) {
                     System.out.println( "  "+i+" op='"+((Entry)goEntries.get(i)).operator+
                                "' go='"+((Entry)goEntries.get(i)).goflag+
                                "' sf='"+((Entry)goEntries.get(i)).seqfil+
                                "' sm='"+((Entry)goEntries.get(i)).submit+
                                "' st='"+((Entry)goEntries.get(i)).start+
                                "' dn='"+((Entry)goEntries.get(i)).done
                                );
                    pp.setIndex(0);
                    startDate = sdf.parse(e.start,pp);
                    pp.setIndex(0);
                    doneDate  = sdf.parse(e.done, pp);
//                    System.out.println("Start "+startDate+" Done "+doneDate);
                    deltaSecs = (doneDate.getTime()-startDate.getTime())/1000;
                    System.out.println("  Diff "+ deltaSecs+" seconds");
                    numGos++; numSecs += deltaSecs;
                }
            }
        }
        System.out.println("Number of gos: "+numGos+" for "+numSecs+" seconds ");
    }
    private class MyHandler extends DefaultHandler {
        
      public void endDocument() {
        System.out.println("End Of Document");
      }
        
      public void endElement(String uri, String localName, String name) {
//        System.out.println("End of Element '"+name+"'");      
      }
        
      public void error(SAXParseException spe) {
        System.out.println("Recoverable Parse Exception");
      }
        
      public void fatalError(SAXParseException spe) {
        System.out.println("Fatal ERror");
      }
        
      public void startDocument() {
        goEntries = new ArrayList();
        System.out.println("Start of Document");
      }
        
      public void startElement(String uri, String localName, String qName, Attributes attributes) {
//        System.out.println("Start of Element '"+qName+"' '"+attributes.getValue(1)+"'");
        Entry e = new Entry();
//        for (int i=0; i<attributes. getLength(); i++) {
//          System.out.println("    "+i+"  '"+attributes.getQName(i)+"'='"+attributes.getValue(i)+"'");
//        }
        if (qName.compareTo("entry") == 0) {    
          e.account  = attributes.getValue(0);
          e.operator = attributes.getValue(1);
          e.goflag   = attributes.getValue(2);
          e.loc      = attributes.getValue(3);
          e.result   = attributes.getValue(4);
          e.seqfil   = attributes.getValue(5);
          e.submit   = attributes.getValue(6);
          e.start    = attributes.getValue(7);
          e.done     = attributes.getValue(8);
        
          goEntries.add(e);
        }
            
      }
    }
    
    public class Entry {
      String account;
      String operator;
      String goflag;
      String loc;
      String result;
      String seqfil;
      String submit;
      String start;
      String done;
    }
      
}
