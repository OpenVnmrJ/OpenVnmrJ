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
 * ReadLast.java
 *
 * Created on April 6, 2005, 5:56 PM
 */

/**
 *
 * @author frits
 */

package accounting;

import java.awt.*;
import java.io.*;
import java.text.*;
import java.util.*;
import javax.swing.*;
import javax.xml.parsers.*;
import org.xml.sax.*;
import org.xml.sax.helpers.DefaultHandler;

public class ReadLast {

    static private int           thisYear;
    static private double        total;
    static private AccountInfo2  ai;
           private ArrayList     lines,oneUser;
           private ArrayList     goesLines, loginLines;
           private ArrayList     rateList;
           private Date          startDate, endDate, checkDate;
    static private SimpleDateFormat sdf;
    
    static private ReadLast      readLast;
    private String goesFile = null;
    private boolean bGoodList = true;
    private boolean bGoRec = true;
    private long    logFileDate = 0;
    private long    logFileSize = 0;
    private double  totalTime = 0.0;

//    public static void main(String[] args) {
//        AccountInfo2 ai = new AccountInfo2();
//        ReadLast rl = getInstance(ai);
//        rl.getLines("frits");
//    }

  public void startDate(Date startDate) {this.startDate = startDate;}
  public Date startDate()               {return(startDate);}

  public void endDate(Date endDate)     {this.endDate = endDate;}
  public Date endDate()                 {return(endDate);}

    public double total()                 {return (this.total);}
     
    static ReadLast getInstance(AccountInfo2 ai) {
      if (readLast == null)
        readLast = new ReadLast();
      readLast.ai = ai;
      return ( readLast );
    }

    static ReadLast getAndCheckInstance(AccountInfo2 ai) {
        if (readLast == null)
            readLast = new ReadLast();
        else {
            if (!readLast.isGoodRecords())
              readLast.readRecords(2);
        }
        readLast.ai = ai;
        return ( readLast );
    }

    /** Creates a new instance of ReadLast */
    private ReadLast() {
       //  File dir = new File(AProps.getInstance().getRootDir()+"/adm/accounting/accounts");
//        System.out.println("ReadLast: rootdir='"+dir+"'");
//        String [] fileList = dir.list();
//        if ( fileList != null) {
//            for(int i=0; i<fileList.length; i++)
//                System.out.println("Readlast: fileList["+i+"]='"+fileList[i]+"'");
//        }
        readRecords(2);
    }

    
    public void readRecords(int i) {
        String billMode = AProps.getInstance().getBillingMode();
        thisYear = (new GregorianCalendar()).get(Calendar.YEAR);
        bGoodList = true;
        if (billMode.compareToIgnoreCase("goes") == 0) {
            readGoesFile();
        }
        else if (billMode.compareToIgnoreCase("macros") == 0) {
            readLoginFile();
        }
        else if (billMode.compareToIgnoreCase("login") == 0) { 
            readGoesFile();
        }
        else {
            Accounting.showErrorMsg("Unknown option for 'billmode'.\nPlease correct accounting.prop");
            /*****
            JOptionPane.showMessageDialog(null,
                    "Unknown option for 'billmode'.\nPlease correct accounting.prop",
                    "Error",
                    JOptionPane.ERROR_MESSAGE);
            *****/
            System.exit(1);
        }
    }
    
    public boolean isGoodRecords() {
        return bGoodList;
    }

    public void reset() {
        readLast = null;
    }
    
    private void readGoesFile() {
        String  errMsg0 = null;
        String  errMsg = null;
        GoesContentHandler xmlHandler;

        goesFile = new String(AProps.getInstance().getRootDir() + "/adm/accounting/acctLog.xml");
        File xmlFile = new File(goesFile);
        sdf = new SimpleDateFormat("EEE MMM d HH:mm:ss zzz yyyy");
        if ( ! xmlFile.exists() ) {
            Accounting.showErrorMsg("Cannot find " + goesFile+ "\nPlease check Billing Mode setting in the Accounting Properties window.");
            /******
            JOptionPane.showMessageDialog(null,
                    "Cannot find " + goesFile+ "\nPlease check Billing Mode setting in the Accounting Properties window.",
                    "Error",
                    JOptionPane.ERROR_MESSAGE);
            ******/
            bGoodList = false;
            return;
        }
        if (logFileDate == xmlFile.lastModified()) {
           if (logFileSize == xmlFile.length())
                return;
        }
        goesLines = new ArrayList();
        loginLines = new ArrayList();
        xmlHandler = new GoesContentHandler();
        try {
            SAXParserFactory spf = SAXParserFactory.newInstance();
            SAXParser parser = spf.newSAXParser();
            parser.parse(xmlFile, xmlHandler);
        }
        catch (ParserConfigurationException pce) {
            errMsg = pce.getMessage();
            System.err.println(pce.getMessage());
        }
        catch (FactoryConfigurationError fce) {
            errMsg = fce.getMessage();
            System.err.println(fce.getMessage());
        }
        catch (Exception ioe) {
            String msg = ioe.getMessage();
            // If this is a msg about not having matching start and end lines
            // then don't print it.  Often the gorecords.xml file is missing the
            // closing portion.
            errMsg = ioe.getMessage();
            if(msg.indexOf("start and end") < 0)
                System.out.println("Exception:"+ ioe.getMessage());
        }
        errMsg0 = xmlHandler.getErrMessage();
        if (errMsg != null) {
            if (errMsg0 != null)
               errMsg0 = errMsg0 + errMsg;
            else
               errMsg0 = errMsg;
        }
        if (errMsg0 != null) {
            bGoodList = false;
            Accounting.showErrorMsg(errMsg0);
        }
        if (bGoodList) {
           logFileDate = xmlFile.lastModified();
           logFileSize = xmlFile.length();
        }
    }
       
    private void readLoginFile() {
        lines = new ArrayList();
        BufferedReader in = null;
        File loginRecords = new File(AProps.getInstance().getRootDir()+"/adm/accounting/loginrecords.txt");
        if ( ! loginRecords.exists() ) {
            Accounting.showErrorMsg("Cannot find "+AProps.getInstance().getRootDir()+"/adm/accounting/loginrecords.txt.\nPlease check Billing Mode setting in the Accounting Properties window.");
            lines.clear();
            bGoodList = false;
            return;
        }
        
        String oneLine= new String("  "); // non-null seed
        try {
            in = new BufferedReader(new FileReader(loginRecords));
            while (oneLine != null) {
                 oneLine = in.readLine();
                 if ( (oneLine!=null) && (oneLine.length()>5) )
                     lines.add(oneLine);
            }
        }
         catch (IOException ioe) {
             ioe.printStackTrace();
            bGoodList = false;
       }
       finally {
           try {
               if (in != null)
                   in.close();
           }
           catch (Exception e2) {}
       }
    }

/**********************************************************
    
    private void readLastFile() {
        // Check if Linux. 
        String os = System.getProperty("os.name");
        String cmd;
        BufferedReader in = null;
        StringBuilder cmdBldr = null;
        cmdBldr = new StringBuilder(AProps.getInstance().getRootDir());
        cmdBldr.append("/bin/vnmr_last");
        cmd = new String(cmdBldr);
        if (os.compareToIgnoreCase("Linux") == 0) {
            File wtmp = new File("/var/log/wtmp");
            if ( ! wtmp.exists() ) {
                Accounting.showErrorMsg("Cannot find /var/log/wtmp.\nPlease create as root. It is where Linux records are stored.");
                // JOptionPane.showMessageDialog(null,
                //     "Cannot find /var/log/wtmp.\nPlease create as root. It is where Linux records are stored.",
                //     "Error",
                //     JOptionPane.ERROR_MESSAGE);
                return;
            }
            try {
                Runtime rt = Runtime.getRuntime();
                Process proc = rt.exec(cmd);
                InputStream istrm = proc.getInputStream();
                BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));
                String str =  bfr.readLine();
                while ( ! (str.contains("Done") && str.contains("vnmr_last")) ) {
                    System.out.println(str);
                }
            } catch (IOException ioe) { 
               System.out.println("vnmr_last ioexception");
            }
            
        }
        StringBuilder fileBldr = null;
        fileBldr = new StringBuilder(AProps.getInstance().getRootDir());
        fileBldr.append("/adm/tmp/last.txt");
        String file = new String(fileBldr);
        File wtmp = new File(file);
        if ( ! wtmp.exists() ) {
            Accounting.showErrorMsg("Cannot find "+file+".\n Possibly the command "+cmd+" failed.");
            return;
        }
        sdf = new SimpleDateFormat("EEE MMM d HH:mm zzz yyyy");
        // thisYear = (new GregorianCalendar()).get(Calendar.YEAR);
        String oneLine= new String("  "); // non-null seed

        lines = new ArrayList();
        try {
            in = new BufferedReader(new FileReader(AProps.getInstance().getRootDir()+"/adm/tmp/last.txt"));
            while (oneLine != null) {
                 oneLine = in.readLine();
//  System.out.println(oneLine);
                 if ( (oneLine!=null) && (oneLine.length()!=0) )
                     lines.add(oneLine);
            }
        }
        catch (IOException ioe) {
             ioe.printStackTrace();
       }
       finally {
            try {
               if (in != null)
                   in.close();
            }
            catch (Exception e2) {}
       }

       // System.out.println(":lastRecords file read and closed:");
    }
**********************************************************/

    public ArrayList getLines(String owner, String operator) {
        String billMode = AProps.getInstance().getBillingMode();
        if (billMode.compareToIgnoreCase("goes") == 0) {
            getGoesLines(owner, operator);
        }
        else if (billMode.compareToIgnoreCase("login") == 0) {
            // This seems to work fine for getting the login time (GRS)
            getLoginLines(owner, operator);
        }
//        else if (billMode.compareToIgnoreCase("login") == 0) { 
//           getLastLines(accountName);
//        }
        else { // We should have had an error by now, but just in case
            Accounting.showErrorMsg("Unknown option for 'billmode'.\nPlease correct accounting.prop");
            System.exit(1);
        }
        return (oneUser);
    }

    public ArrayList getLines(String accountName) {
        return getLines(null, accountName);
    }

    public ArrayList getRateList() {
        return rateList;
    }

    public double getTotalTime() {
        return totalTime;
    }

    public ArrayList getGoesLines(String owner, String accountName) {
        oneUser = new ArrayList(); // this stores the result
        Double tmpD;
        Double minutes;
        RateEntry re;
        boolean bOwner;
        String oneLine = new String("");
        String name1, name2,tmp, path, date, howLong;
        StringTokenizer st;
        AProps props = AProps.getInstance();
        NumberFormat nf;
        rateList = new ArrayList();
        total=0;
        if (goesLines == null)
            return oneUser;

        nf = props.getNumberFormat();
        totalTime = 0.0;
        for (int i=0; i < goesLines.size(); i++) {
            oneLine = (String)goesLines.get(i);
            st = new StringTokenizer(oneLine);
            name1 = st.nextToken();
            name2 = st.nextToken();
            bOwner = true;
/*
            if (owner != null) {
                if (!owner.equals(name1))
                   bOwner = false;
            }
 */
            if (bOwner && (name2.compareTo(accountName)==0)) {
                date = st.nextToken("-").trim();
                if (isToBeIncluded(date)) {
                    howLong = st.nextToken();
                    minutes = props.toMinutes(howLong);
                    oneUser.add(name2);
                    oneUser.add(date);
                    re = ai.getRate(date);
                    tmpD = new Double(re.loginhr);
                    rateList.add(nf.format(tmpD));
                    oneUser.add(howLong);
                    tmpD = ai.getCharge(date,minutes.doubleValue());
                    oneUser.add(nf.format(tmpD));
                    total += tmpD.doubleValue();
                    totalTime += minutes.doubleValue();
                }
            }
        }
        return (oneUser);
  }
    
    // This seems to work fine for getting the login times.
    public ArrayList getLoginLines(String owner, String accountName) {
        sdf = new SimpleDateFormat("EEE MMM d HH:mm:ss zzz yyyy");
        oneUser = new ArrayList(); // this stores the result
        Double tmpD,minutes;
        RateEntry re;
        String quoteQuote = new String("");
        String oneLine,howLong,skip;
        String accountStart, ownerStart, nameStart, whenStart;
        String accountDone, ownerDone, nameDone, whenDone;
        StringTokenizer st, st2;
        AProps props = AProps.getInstance();
        NumberFormat nf;
        rateList = new ArrayList();
        total = 0.0;
        totalTime = 0.0;
        if (loginLines == null)
            return oneUser;
        nf = props.getNumberFormat();
        for (int i=0; i< loginLines.size(); i++) {
            oneLine = (String)loginLines.get(i);
            st = new StringTokenizer(oneLine,"=\" \t\n");
            // read past 'account=""'
            skip = st.nextToken();
            // If the first tok is not account, skip this line
            if(!skip.equalsIgnoreCase("account"))
                continue; 

            if (owner != null) {
                st2 = new StringTokenizer(oneLine,"=\" \t\n");
                st2.nextToken();
                ownerDone = null;
                nameDone = null;
                while (st2.hasMoreTokens()) {
                    skip = st2.nextToken();
                    if (skip.equalsIgnoreCase("operator")) {
                        if (st2.hasMoreTokens())
                            nameDone = st2.nextToken();
                    }
                    else if (skip.equalsIgnoreCase("owner")) {
                        if (st2.hasMoreTokens()) {
                            ownerDone = st2.nextToken();
                            if (nameDone != null)
                                break;
                        }
                    }
                }
/*
                if (ownerDone != null) {
                    if (!ownerDone.equalsIgnoreCase(owner))
                        continue;
                }
 */
            }

            accountStart = st.nextToken();
            if (accountStart.equalsIgnoreCase("operator"))
                accountStart = quoteQuote;
            else
                skip = st.nextToken();

            nameStart = st.nextToken();

            // if nameStart ends up with "owner" as its values, it
            // means that the operator value was empty.  In that case
            // we want to use the name of the owner instead of the
            // operator.  Also, the tokenizer will be missing one
            // token due to the empty string, so account for that
            if(nameStart.equals("owner")) {
                nameStart = st.nextToken();
            }
            else {
                // read past 'owner="xyz"'
                skip = st.nextToken();
                ownerStart = st.nextToken();
            }

            // If operator is not accountName, then don't work with this line
            if (nameStart.compareTo(accountName) == 0) {

                skip = st.nextToken(); // skip start or done string
                // Get start time
                // Need to get time in english format which is enclosed in 
                // double quotes
                whenStart = st.nextToken("=\"");
                // If another start follows this one, ignore the first one and
                // take the second one.  When you login to vnmrj and have
                // operators, you get a login panel.  The "start"
                // will be added to the file at that time even though
                // no one is actually logged in.  Another "start" occurs
                // when the operator logs in.  we need to skip the first
                // one and take the second.  Thus, look ahead and see if
                // there is another line and if it is also a start.
                boolean skipLine=false;
                for(int k=1; i+k < loginLines.size(); k++) {
                    String lineAhead = (String)loginLines.get(i+k);
                    String operator = getOperatorName(lineAhead);
                    // Need to get the operator name for this new line
                    // If not correct operator, don't check for start, just
                    // go to next line.

                    // Only check for "start" if this line is the same operator
                    if(accountName.compareTo(operator) == 0) {
                        if(lineAhead.contains("start=") ) {
                            // Found another start, Skip the current oneLine
                            // and let the loop continue to the next line which
                            // we have determined will be a start line.
                            skipLine = true;
                            break;
                        }
                        else {
                            skipLine = false;
                            break;
                        }
                    }
                    else {
                        // Not the same operator, try another line forward.  We are looking
                        // for the first line with the same operator which is NOT "start"
                        continue;
                    }
                }
                
                if(skipLine)
                    continue;
                // The next line by this operator is a "done" line

                if (!nameStart.equals(accountName))
                    continue;

                // System.out.println("whenStart = '"+whenStart+"'");
                if (!isToBeIncluded(whenStart))
                    continue;
                if (skip.equalsIgnoreCase("start")) {
                    int k = i + 1;
                    while (k < loginLines.size()) { // find the "done"
                        oneLine = (String) loginLines.get(k);
                        st = new StringTokenizer(oneLine, "=\" \t\n");
                        skip = st.nextToken();
                        accountDone = st.nextToken();
                        if (accountDone.equalsIgnoreCase("operator"))
                            accountDone = quoteQuote;
                        else
                            skip = st.nextToken();
                        nameDone = st.nextToken();
                        // if nameDone ends up with "owner" as its values, it
                        // means that the operator value was empty.  In that case
                        // we want to use the name of the owner instead of the
                        // operator.  Also, the tokenizer will be missing one
                        // token due to the empty string, so account for that
                        if(nameDone.equals("owner")) {
                            nameDone = st.nextToken();
                        }
                        else {
                            // read past 'owner="xyz"'
                            skip = st.nextToken();
                            ownerDone = st.nextToken();
                        }
                        skip = st.nextToken(); // start or done
                        whenDone = st.nextToken("=\"");
                        // System.out.println("whenDone = '"+whenDone+"'");

                        if (nameStart.equals(nameDone)
                                && skip.equalsIgnoreCase("done")) {
                            // System.out.println("whenDone = '"+whenDone+"'");
                            oneUser.add(nameStart);
                            oneUser.add(whenStart);
                            // Get the total time logged in.  Format is
                            //    days+hr:min:sec
                            howLong = timeDiff(whenStart, whenDone);
                            oneUser.add(howLong);
                            // Get the rate when login started
                            re = ai.getRate(whenStart);
                            tmpD = new Double(re.loginhr);
                            rateList.add(nf.format(tmpD));
                            // Get the login time in minutes
                            minutes = props.toMinutes(howLong);
                            // Calculate the charge for this login session
                            tmpD = ai.getCharge(whenStart, minutes.doubleValue());
                            oneUser.add(nf.format(tmpD));
                            total += tmpD.doubleValue();
                            totalTime += minutes.doubleValue();
                            break;
                        }
                        k++;
                    }
                    if ((k - i) < 3)
                        i = k;
                }
            }
        }
        return (oneUser);
    }

    
    private String getOperatorName(String line) {
        StringTokenizer st;
        String skip, operator="";
        st = new StringTokenizer(line,"=\" \t\n");
        // read past 'account=""'
        skip = st.nextToken();   
        skip = st.nextToken();
        if (!skip.equalsIgnoreCase("operator"))
            skip = st.nextToken();

        operator = st.nextToken();

        // if operator ends up with "owner" as its values, it
        // means that the operator value was empty.  In that case
        // we want to use the name of the owner instead of the
        // operator. 
        if(operator.equals("owner")) {
            operator = st.nextToken();
        }
        return operator;
    }

    // getLastLines() does not seem to have been debugged AND it does
    // not seem to be needed since getLoginLines() works for th login time.

   /********************************************
      private void getLastLines(String accountName) {
        oneUser = new ArrayList(); // this stores the result
        ArrayList userLasts = new ArrayList();
        Double tmpD;
        Double minutes;
        RateEntry re;
        String oneLine = new String("");
        String name,tmp, path, date, howLong;
        StringTokenizer st;
        NumberFormat nf = AProps.getInstance().getNumberFormat();
        total=0;
        for (int i=0; i<lines.size(); i++) {
            oneLine = (String)lines.get(i);
            if (oneLine.length() < 10) continue;
//            System.out.println(oneLine);
            name = oneLine.substring(0,9).trim();
            if (name.compareTo(accountName)==0) {
                path = oneLine.substring(9,18).trim();
                date = oneLine.substring(39,56).trim();
//                  System.out.println(date);
                if (isToBeIncluded(date)) {
                    howLong = oneLine.substring(64,oneLine.length()-1).trim();
                    if ( ! howLong.startsWith("(")) continue;
                    LastInfo li = new LastInfo();
                    li.minutes = AProps.getInstance().toMinutes(howLong);
//                    System.out.println("---"+name+" "+date+" "+howLong+" "+minutes);
                    li.startDate = new GregorianCalendar();
                    li.startDate.setTime(checkDate);
                    li.endDate = (Calendar)li.startDate.clone();
                    li.endDate.add(Calendar.MINUTE,(int)li.minutes);
                    li.start = date;
                    re = ai.getRate(date);
                    tmpD = new Double(re.loginhr);
                    li.howlong = howLong;
                    tmpD = ai.getCharge(date,li.minutes);
                    li.cost = tmpD;
                    li.costStr = nf.format(tmpD);
                    userLasts.add(li);
                }
            }
        }
        int n = userLasts.size();
        LastInfo iTmpLI, jTmpLI;
        long iStart, jStart, iEnd, jEnd, dStart, dEnd;
        int k=0;
        for (int i=0; i<n; i++) {
            iTmpLI = (LastInfo) userLasts.get(i);
            boolean found = false;
            for (int j=(i+1); j<n; j++) {
                jTmpLI = (LastInfo) userLasts.get(j);
 //               if ( (jTmpLI.startDate.compareTo(iTmpLI.startDate) <= 0) &&
 //                    (jTmpLI.endDate.compareTo(iTmpLI.endDate) >=0) ){
                iStart = iTmpLI.startDate.getTimeInMillis();
                jStart = jTmpLI.startDate.getTimeInMillis();
                dStart = iStart - jStart;
                iEnd   = iTmpLI.endDate.getTimeInMillis();
                jEnd   = jTmpLI.endDate.getTimeInMillis();
                dEnd   = iEnd - jEnd;
                if ( ((jTmpLI.startDate.getTimeInMillis() - iTmpLI.startDate.getTimeInMillis()) < 61000) &&
                      ((iTmpLI.endDate.getTimeInMillis() - jTmpLI.endDate.getTimeInMillis()) < 61000) ) {
                    found = true;
                    k=i;
                    break;
                }
                if ( ((iTmpLI.startDate.getTimeInMillis() - jTmpLI.startDate.getTimeInMillis()) < 61000) &&
                      ((jTmpLI.endDate.getTimeInMillis() - iTmpLI.endDate.getTimeInMillis()) < 61000) ) {
                    found = true;
                    k=j;
                    break;
                }
            }
            if (found) {
                userLasts.remove(k);
                n--; i--;
            }
            else {
                oneUser.add(accountName);
                oneUser.add(iTmpLI.start);
                oneUser.add(iTmpLI.howlong);
                oneUser.add(iTmpLI.costStr);
                total += iTmpLI.cost;
            }        
        }
//         System.out.println("total="+total+" minutes");
  }

      private class LastInfo {
          Calendar startDate;
          Calendar endDate;
          String start;
          String howlong;
          String costStr;
          double minutes;
          double cost;
      }
   ********************************************/

    public boolean isToBeIncluded(String dateStr) {
       
        ParsePosition pp = new ParsePosition(0);
        if (dateStr==null) return false;
        String newDateStr = new String(dateStr+" "+Integer.toString(thisYear)); // this year?
        checkDate = sdf.parse(newDateStr, pp);
        int i = pp.getErrorIndex();
        int j = pp.getIndex();
        String xx = checkDate.toString();
        for (int k=1; k<7; k++) {
//            System.out.println("isToBeIncluded() "+k+" "+xx.substring(0,3));
            if ( (xx.substring(0,3)).compareTo(dateStr.substring(0,3)) != 0) {  
                newDateStr = new String(dateStr + " "+Integer.toString(thisYear-k)); //no, then last year
                pp.setIndex(0);
                checkDate = sdf.parse(newDateStr, pp);
                xx = checkDate.toString();
                i = pp.getErrorIndex();
                j = pp.getIndex();
             }
            else
                break;
        }
        if ( (xx.substring(0,3)).compareTo(dateStr.substring(0,3)) != 0) return(false);
        if ((checkDate.compareTo(startDate)>0) && (checkDate.compareTo(endDate)<0) )
            return (true);
        else
            return (false);
     }
    public String timeDiff(String date1, String date2) {
            ParsePosition pp = new ParsePosition(0);
            Date from = sdf.parse(date1,pp);
            // System.out.println("TimeDiff: from ="+from);
            GregorianCalendar cFrom = new GregorianCalendar();
            cFrom.setTime(from);
            pp.setIndex(0);
            Date to = sdf.parse(date2,pp);
            // System.out.println("TimeDiff: to ="+to);
            GregorianCalendar cTo = new GregorianCalendar();
            cTo.setTime(to);
            long mills = cTo.getTimeInMillis() - cFrom.getTimeInMillis();
            if (mills < 0)
                mills = 0;
            // System.out.println("TimeDiff = "+mills);
            int days = (int)(mills/1000/60/60/24);
            int hr = (int)(mills/1000/60/60)%24;
            int min = (int)(mills/1000/60)%60;
            int sec = (int)(mills/1000)%60;
        
            StringBuilder sb = new StringBuilder("(");
            if (days>0)
                sb.append(days).append("+");
            if (hr < 10)
                sb.append("0");
            sb.append(hr).append(":");
            if (min < 10)
                sb.append("0");
            sb.append(min).append(":");
            if (sec < 10)
                sb.append("0");
            sb.append(sec).append(")");
            return( sb.toString() );
        }

    public String calTimeDiff(String start, String end) {
        double time0, time1;
        double dtime;

        time0 = 0.0;
        time1 = 0.0;
        try {
            time0 = Double.parseDouble(start);
            time1 = Double.parseDouble(end);
        }
        catch (NumberFormatException e) {
        }
        dtime = time1 - time0;
        if (dtime < 0.0)
            dtime = 0.0;
        int days = (int) (dtime / 60.0 / 60.0 /24.0);
        int hr = (int)(dtime / 60.0 / 60.0)%24;
        int min = (int)(dtime / 60.0) % 60;
        int sec = (int)dtime % 60;
        StringBuilder sb = new StringBuilder("(");
        if (days>0)
              sb.append(days).append("+");
        if (hr < 10)
              sb.append("0");
        sb.append(hr).append(":");
        if (min < 10)
             sb.append("0");
        sb.append(min).append(":");
        if (sec < 10)
             sb.append("0");
        sb.append(sec).append(")");
        return( sb.toString() );
    }

    
    
    class GoesContentHandler extends DefaultHandler  {
        private StringBuffer strBuf = null;

        public void endDocument() {
            //System.out.println("GoesContentHandler: endDocument");
        }
        
        public void endElement(String name) {
            // System.out.println("GoesContentHandler: endElement" + name);
        }
        public void startDocument() {
            //System.out.println("GoesContentHandler: startDocument");
        }
        
        public void startElement(String uri, String locale, String name, Attributes atts ) {
            if (name.compareToIgnoreCase("accounting")==0) 
                return;

            String type = atts.getValue("type");
            String startSec;
            String endSec;
            Date dateObj;
            double dtime;
            String owner;
            String operator;
            String start;
            String end;
            String date;
            // Skip all entries that are not gorecords
            if (type.equals("gorecord")) {
                operator = atts.getValue("operator");
                owner = atts.getValue("owner");
                start = atts.getValue("start");
                end = atts.getValue("end");
                startSec = atts.getValue("startsec");
                endSec = atts.getValue("endsec");

                if(operator == null | start == null | end == null) {
                    System.out.println("acctLog.xml gorecords entry missing required field. Skipping.");
                    return;
                }
                if (startSec == null || endSec == null)
                    return;

                if (owner == null || owner.length() < 1)
                    owner = operator;
                dtime = Double.parseDouble(startSec) * 1000.0;
                dateObj = new Date((long)dtime);
               /********
                String date = start;
                date = date.substring(0,date.length()-4);
                String howLong = timeDiff(start, end);
                String s = new String(operator+" "+date+"-"+howLong);
                *********/
                String howLong = calTimeDiff(startSec, endSec);
                start = dateObj.toString();
                String s = new String(owner+" "+operator+" "+start+"-"+howLong);
                goesLines.add(s);
            }
            else {
              if (type.equals("login") ) {
                operator = atts.getValue("operator");
                owner = atts.getValue("owner");
                start = atts.getValue("start");
                date = atts.getValue("date");
                startSec = atts.getValue("startsec");

                if (startSec != null) {
                    dtime = Double.parseDouble(startSec) * 1000.0;
                    dateObj = new Date((long)dtime);
                    start = dateObj.toString();
                }
                String s = new String("account=\"\" operator=\""+operator+"\" owner=\""+owner+
                                      "\" start=\""+start+"\"");
                loginLines.add(s);
              }
              else if(type.equals("logout")) {
                operator = atts.getValue("operator");
                owner = atts.getValue("owner");
                end = atts.getValue("end");
                date = atts.getValue("date");
                endSec = atts.getValue("endsec");

                if (endSec != null) {
                    dtime = Double.parseDouble(endSec) * 1000.0;
                    dateObj = new Date((long)dtime);
                    end = dateObj.toString();
                }
                String s = new String("account=\"\" operator=\""+operator+"\" owner=\""+owner+
                                      "\" done=\""+end+"\"");
                loginLines.add(s);
              }
            }
        }
        
        public void setErrMessage(SAXParseException spe){
           if (strBuf == null)
              strBuf = new StringBuffer("File: ").append(goesFile).append("\n");
           strBuf.append("Error at line ").append(spe.getLineNumber())
                  .append(" column ").append(spe.getColumnNumber()).append("\n");
        }

        public String getErrMessage() {
           if (strBuf == null)
               return null;
           return strBuf.toString();
        }

        public void error(SAXParseException spe){
           // System.out.println("SAX error at line "+spe.getLineNumber()+" column "+spe.getColumnNumber());
           setErrMessage(spe);
        }
        
        public void fatalError(SAXParseException spe){
            String msg = spe.getMessage();
            // If this is a msg about not having matching start and end lines
            // then don't print it.  Often the gorecords.xml file is missing the
            // closing portion.
            // if(msg.indexOf("start and end") < 0)
            //    System.out.println("SAX fatal error at line "+spe.getLineNumber()+" column "+spe.getColumnNumber());
           setErrMessage(spe);
        }
        
        public void warning(SAXParseException spe){
           // System.out.println("SAX warning at line "+spe.getLineNumber()+" column "+spe.getColumnNumber());
           setErrMessage(spe);
        }
    }
}
