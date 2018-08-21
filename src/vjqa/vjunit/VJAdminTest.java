/*
 * VJAdminTest.java
 * JUnit based test
 *
 * Created on June 30, 2004, 10:22 AM
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
public class VJAdminTest extends TestCase {
    
    protected Thread threadUpdate;
    
    public static final int MIN = 60000;
    
    public VJAdminTest(java.lang.String testName) {
        super(testName);
    }
    
    public static junit.framework.Test suite() {
        TestSuite suite = new TestSuite(VJAdminTest.class);
        return suite;
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
    
    public void testUpdateUsers()
    {
        threadUpdate = new Thread(new Runnable()
        {
            public void run()
            {
                JFrameOperator main = new JFrameOperator("VnmrJ Admin");
                JMenuBarOperator menu = new JMenuBarOperator(main);
                menu.pushMenu("Configure|Users|Update Users", "|");
                JDialogOperator frame = new JDialogOperator("Update");
                JListOperator list = new JListOperator(frame, 0);
                int nLength = list.getModel().getSize();
                int[] nAUsers = new int[nLength];
                for (int i = 0; i < nLength; i++)
                {
                    nAUsers[i] = i;
                }
                list.selectItems(nAUsers);
                for (int i = 0; i < 10; i++)
                {
                    JButtonOperator btn = new JButtonOperator(frame, i);
                    String txt = btn.getText();
                    if (txt == null || txt.equals(""))
                    {
                        btn.clickMouse();
                        break;
                    }
                }
                JButtonOperator btn = new JButtonOperator(frame, "Update Users");
                btn.clickMouse();
                
                try
                {
                    threadUpdate.sleep(nLength*2*MIN);
                }
                catch (Exception e)
                {
                    e.printStackTrace(System.out);
                }
                
                main.close();
            }
        });
        threadUpdate.start();
    }
    
    // Add test methods here, they have to start with 'test' name.
    // for example:
    // public void testHello() {}
    
    
}
