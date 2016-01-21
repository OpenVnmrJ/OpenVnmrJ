/*
 * XmlTest.java
 *
 * Created on February 25, 2007, 11:24 AM
 *
 */
import java.io.*;
import java.util.*;
import java.text.*;

/**
 *
 * @author frits
 */
public class XmlTest {    
    
    private final String baseDir =
            new String("./");
    private final String xmlTestResultsFile =    new String("XmlTestResults.txt");
    private final String xmlDirListFile     =    new String("XmlDirsList.txt");
    
    
    BufferedReader xmlDirsList = null;
    BufferedWriter xmlResults = null;
    DecimalFormat df;
    String vnmrsystem;
    String oneDir;
    StringBuilder sb = null;
    String msg;
    
    XmlChecker xc;
    
   
    public static void main(String[] args) {
        XmlTest xt = new XmlTest();
        System.exit(0);
    }
    
    public XmlTest() {
              
        // Open the Results file $HOME/XmlTestResults.txt
        try {
            xmlResults = new BufferedWriter( new FileWriter(baseDir+xmlTestResultsFile) );
            msg = new String( new Date().toString() );
            xmlResults.write(msg, 0, msg.length());
            xmlResults.newLine();
            xmlResults.newLine();
        }
        catch (IOException ioe) {
            System.out.println("Cannot open '"+baseDir+xmlTestResultsFile+"'");
            System.exit(0);
        }
        // Open the fiel with the list of directoies to check
        // One directory per line
        // $HOME/vjunit/XmlDirList.txt 
        try {
             xmlDirsList = new BufferedReader( new FileReader(baseDir+xmlDirListFile) );    
        }
        catch (IOException ioe) {
            msg = new String("Cannot open '"+baseDir+xmlDirListFile+"'");
            System.out.println(msg);
            try {
               xmlResults.write(msg, 0, msg.length());
               xmlResults.newLine();
               xmlResults.close();
            }
            catch (IOException ioe2) {
                System.out.println("Cannot write to '"+baseDir+xmlTestResultsFile+"'");
            }
            System.exit(0);
        }
        // report result of opening file
        try {
            sb = new StringBuilder("Opened '"+baseDir+xmlDirListFile+"' ").append( new Date().toString() );
            msg = sb.toString();
            xmlResults.write(msg, 0, msg.length());
            xmlResults.newLine();
            xmlResults.newLine();
        }
        catch (IOException ioe) {
            System.out.println("Cannot write to '"+baseDir+xmlTestResultsFile+"'");
        }
        // Specify the bumber format for later use
        df = new DecimalFormat("000");
        // Open on e directory
        xc = new XmlChecker(xmlResults);
        try {
            while (true) {
                oneDir = xmlDirsList.readLine();
                if (oneDir == null) break;
                xmlResults.write("Checking directory '"+oneDir+"'");
                xmlResults.newLine();
                if (new File(oneDir).exists()) 
                    searchOneDir( new File(oneDir) );
                else
                    System.out.println( new File(oneDir).getAbsolutePath() + " does not exists");
            } 
        }
        catch (IOException ioe) {
            System.out.println("Cannot open/read '"+baseDir+xmlDirListFile+"'");
        }
        finally {
            try {
              xmlResults.close();
            }
            catch (IOException ioe) {
               System.out.println("Cannot close to '"+baseDir+xmlTestResultsFile+"'");
            }
        }
        System.exit(0);
    }
    
    public void searchOneDir( File dir ) {
        File[] files = dir.listFiles();
        for (int i=0; i<files.length; i++) {
            sb = new StringBuilder("  ")
                .append( df.format((long)i))
                .append("  ")
                .append(files[i].getName());
            msg = sb.toString();
            try {
                xmlResults.write(msg, 0, msg.length());
                if (files[i].getAbsolutePath().endsWith("xml")) {
                    xmlResults.write("  parsing...",0,12);
                    xc.xmlParser(files[i]);
                }
                xmlResults.newLine();
            }
            catch(IOException ioe) {
                System.out.println("Problems write to '"+baseDir+xmlTestResultsFile+"'");
            }
        }
    }        
}
