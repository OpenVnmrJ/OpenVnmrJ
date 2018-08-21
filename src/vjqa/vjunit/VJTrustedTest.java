/*
 * VJTrustedTest.java
 * JUnit based test
 *
 * Created on October 29, 2004, 1:49 PM
 */

import java.io.*;
import java.util.*;
import junit.framework.*;
import org.netbeans.jemmy.*;
import org.netbeans.jemmy.operators.*;

/**
 *
 * @author mrani
 */
public class VJTrustedTest extends TestCase {
    
    protected Thread threadVJ;
    protected JFrameOperator main;
    
    public VJTrustedTest(java.lang.String testName) {
        super(testName);
    }
    
    public static void main(java.lang.String[] args) {
         try {
	    new ClassReference("vnmr.ui.VNMRFrame").startApplication();

            //increase timeouts values to see what's going on
            //otherwise everything's happened very fast
            JemmyProperties.getCurrentTimeouts().loadDebugTimeouts();	
	} catch(Exception e) {
	    e.printStackTrace();
	}
        junit.textui.TestRunner.run(suite());
    }
    
    public static junit.framework.Test suite() {
        TestSuite suite = new TestSuite(VJTrustedTest.class);
        return suite;
    }
    
    public void testVJ()
    {
        threadVJ = new Thread(new Runnable()
        {
            public void run()
            {
                try
                {
                    main = new JFrameOperator("VnmrJ Admin");
                    
                }
                catch (Exception e)
                {
                    e.printStackTrace();
                }
            }
        });
        
        threadVJ.start();
        
    }
    
    public void log(String strTest)
    {
        System.out.println("\n****** Running Test: " + strTest + " *********\n");
    }
    
    // Add test methods here, they have to start with 'test' name.
    // for example:
    // public void testHello() {}
    
    
}
