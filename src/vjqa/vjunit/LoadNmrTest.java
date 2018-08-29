/*
 * LoadNmrTest.java
 * JUnit based test
 *
 * Created on June 25, 2004, 10:57 AM
 */

import java.text.*;
import javax.swing.*;
import javax.swing.border.Border;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.io.*;
import javax.swing.event.*;
import java.net.*;
import junit.framework.*;
import org.netbeans.jemmy.*;
import org.netbeans.jemmy.operators.*;

/**
 *
 * @author mrani
 */
public class LoadNmrTest extends TestCase {
    
    protected static String m_strDir = "";
    protected static String m_strPassword =  "";
    protected static String osVersion = "";
    protected static Thread loadnmrThread;
    public static final int MIN = 60000;
    
    public LoadNmrTest(java.lang.String testName) {
        super(testName);
    }
    
    public static void main(java.lang.String[] args) {
         try {
            if (args != null && args.length-1 > 0)
            {
                m_strDir = args[0];
                m_strPassword = args[args.length-1];
                osVersion = args[5];
            }
            new ClassReference("LoadNmr").startApplication(args);
            
            //increase timeouts values to see what's going on
            //otherwise everything's happened very fast
            JemmyProperties.getCurrentTimeouts().loadDebugTimeouts();	
	} catch(Exception e) {
	    e.printStackTrace();
	}
        junit.textui.TestRunner.run(suite());
    }
    
    public static junit.framework.Test suite() {
        TestSuite suite = new TestSuite(LoadNmrTest.class);
        return suite;
    }
    
    public void testLoadVJ()
    {
        loadnmrThread = new Thread(new Runnable()
        {
            public void run()
            {
                JFrameOperator main = new JFrameOperator("Load VnmrJ Software");
                if (m_strPassword != null && !m_strPassword.equals("") &&
                    m_strPassword.startsWith("password"))
                {
                    String strPath = m_strDir+".passwords";
                    String strPassword = m_strPassword.substring(9);
                    StringTokenizer strTok = new StringTokenizer(strPassword, ",\n");
                    System.out.println("cmd " + m_strPassword);
                    HashMap hmPassword = getPasswords(strPath);
           
                    int i = 0;
                    while (strTok.hasMoreTokens())
                    {
                        String strValue = strTok.nextToken();
                        JCheckBoxOperator chk = new JCheckBoxOperator(main, strValue);
                        chk.clickMouse();
      
                        JTextFieldOperator txf = new JTextFieldOperator(main, i);
                        String password = (String)hmPassword.get(strValue);
                        txf.setText(password);
                        System.out.println("cmd " + password);
                        i = i+1;
                    }
                }
                
                String strtype = System.getProperty("installtype");
                if (strtype != null && !strtype.equals("") )
                {
                   int ntype = 0;
                   ntype = Integer.parseInt(strtype);
                   JTabbedPaneOperator tabPane = new JTabbedPaneOperator(main);
                   tabPane.selectPage(ntype);
                }
                
                // Install Software
                JButtonOperator btn = new JButtonOperator(main, "Install");
                btn.clickMouse();
                
                // Wait for installation to complete
                try
                {
                    loadnmrThread.sleep(30*MIN);
                }
                catch (Exception e)
                {
                    e.printStackTrace(System.out);
                }
                
                JFrameOperator frame = new JFrameOperator("Installing");
                btn = new JButtonOperator(frame, "Done");
                btn.clickMouse();
            }
        });
        loadnmrThread.start();
    }
    
    protected HashMap getPasswords(String strPath)
    {
        HashMap hmPassword = new HashMap();
        String strline;
        try
        {
            BufferedReader reader = new BufferedReader(new FileReader(strPath));
            if (reader == null)
                return hmPassword;
        
            while ((strline = reader.readLine()) != null)
            {
                StringTokenizer strTok = new StringTokenizer(strline);
                if (strTok.countTokens() == 2)
                {
                    String strkey = strTok.nextToken();
                    String strvalue = strTok.nextToken();
                    hmPassword.put(strkey, strvalue);
                }
            }
            reader.close();
        }
        catch (Exception e)
        {
            e.printStackTrace(System.out);
        }   
        
        return hmPassword;
    }
    
    /** Test of addWindowListener method, of class LoadNmr. */
    /*public void testAddWindowListener() {
        System.out.println("testAddWindowListener");
        
        // Add your test code below by replacing the default call to fail.
        fail("The test case is empty.");
    }*/
    
    /** Test of chkAvailSpace method, of class LoadNmr. */
    /*public void testChkAvailSpace() {
        System.out.println("testChkAvailSpace");
        
        // Add your test code below by replacing the default call to fail.
        fail("The test case is empty.");
    }*/
    
    /** Test of getVnmrVersion method, of class LoadNmr. */
    /*public void testGetVnmrVersion() {
        System.out.println("testGetVnmrVersion");
        
        // Add your test code below by replacing the default call to fail.
        fail("The test case is empty.");
    }*/
    
    /** Test of getNIC method, of class LoadNmr. */
    /*public void testGetNIC() {
        System.out.println("testGetNIC");
        
        // Add your test code below by replacing the default call to fail.
        fail("The test case is empty.");
    }*/
    
    /** Test of getItem method, of class LoadNmr. */
    /*public void testGetItem() {
        System.out.println("testGetItem");
        
        // Add your test code below by replacing the default call to fail.
        fail("The test case is empty.");
    }*/
    
    /** Test of setSumLabel method, of class LoadNmr. */
    /*public void testSetSumLabel() {
        System.out.println("testSetSumLabel");
        
        // Add your test code below by replacing the default call to fail.
        fail("The test case is empty.");
    }*/
    
    /** Test of getTOC method, of class LoadNmr. */
    /*public void testGetTOC() {
        System.out.println("testGetTOC");
        
        // Add your test code below by replacing the default call to fail.
        fail("The test case is empty.");
    }*/
    
    // Add test methods here, they have to start with 'test' name.
    // for example:
    // public void testHello() {}
    
    
}
