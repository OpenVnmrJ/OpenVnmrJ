/*
 * VnmrJTest.java
 * JUnit based test
 *
 * Created on June 30, 2004, 3:49 PM
 */

import java.io.*;
import java.util.*;
import java.lang.reflect.*;
import javax.xml.parsers.*;
import org.xml.sax.*;
import org.xml.sax.helpers.*;
import junit.framework.*;
import org.netbeans.jemmy.*;
import org.netbeans.jemmy.operators.*;

import vnmr.ui.*;
import vnmr.util.*;

/**
 *
 * @author mrani
 */
public class VnmrJTest extends TestCase implements MessageListenerIF {
    
    protected Thread threadVJ;
    protected JFrameOperator main;
    protected HashMap vobjs = new HashMap();
    public static final int MIN = 60000;
    
    public VnmrJTest(java.lang.String testName) {
        super(testName);
        Messages.addMessageListener(this);
    }
    
    public static junit.framework.Test suite() {
        TestSuite suite = new TestSuite(VnmrJTest.class);
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
    
    public void testVnmrbg()
    {
        threadVJ = new Thread(new Runnable()
        {
            public void run()
            {
                fillHashMap();
                main = new JFrameOperator("OpenVnmrJ");
                try
                {
                    threadVJ.sleep(2000);
       
                        Util.sendToVnmr("owner?");
                        JTextComponentOperator lbl = new JTextComponentOperator(main, "owner =");
                        Util.sendToVnmr("instrument?");
                        JTextComponentOperator lbl2 = new JTextComponentOperator(main, "instrument =");
                        /*JButtonOperator btn = new JButtonOperator(main, "Acquire");
                        btn.clickMouse();
                        threadVJ.sleep(MIN/4);
                        JListOperator list = new JListOperator(main);
                        list.selectItem("VJUnit");*/
                        Util.sendToVnmr("config");
                        JDialogOperator config = new JDialogOperator(main, "Configuration");
                        JButtonOperator btn = new JButtonOperator(config, "OK");
                        btn.clickMouse();
                        runVnmrjtest();
                    main.close();
                }
                catch (Exception e)
                {
                    e.printStackTrace(System.out);
                    main.close();
                }
            }
        });
        threadVJ.start();
    }
    
    protected void runVnmrjtest()
    {
        String strPath = System.getProperty("vjunit");
        try
        {
            SAXParserFactory spf = SAXParserFactory.newInstance();
            spf.setValidating(false);  // set to true if we get DOCTYPE
            spf.setNamespaceAware(false); // set to true with referencing
            SAXParser parser = spf.newSAXParser();
            parser.parse(strPath, new VSaxHandler());
        }
        catch (Exception e)
        {
            e.printStackTrace(System.out);
        }
    }
    
    protected void compTest(JComponentOperator comp, Attributes attr)
    {
        String strValue = attr.getValue("action");
        if (strValue == null)
            strValue = "";
        log(attr.getValue("value"), false);
       
        if (comp instanceof JTextFieldOperator  && 
                 strValue.equalsIgnoreCase("type"))
        {
            JTextFieldOperator txf = (JTextFieldOperator)comp;
            txf.clearText();
            txf.typeText(attr.getValue("value2"));
        }
        else if (comp instanceof JComboBoxOperator)
        {
            JComboBoxOperator cmb = (JComboBoxOperator)comp;
            cmb.showPopup();
            cmb.selectItem(attr.getValue("value2"));
        }
        else if (comp != null && strValue.equalsIgnoreCase("click"))
            comp.clickMouse();
        else if (comp == null)
        {
            strValue = attr.getValue("value");
            if (strValue != null && !strValue.equals(""))
                Util.sendToVnmr(strValue);
        }
                    
        strValue = attr.getValue("time");
        if (strValue != null && !strValue.equals(""))
        {
            int nMin = 0;
            try
            {
                nMin = (int)(Double.parseDouble(strValue)*MIN);
                if (nMin > 0)
                    threadVJ.sleep(nMin);
            }
            catch (Exception e)
            {
                e.printStackTrace(System.out);
            }
        }
        
        strValue = attr.getValue("pass");
        if (strValue != null && !strValue.equals(""))
        {
            JTextComponentOperator lbl = new JTextComponentOperator(main, strValue);
        }
        log(attr.getValue("value"), true);
        
    }
    
    protected Constructor getTool(String strKey)
    {
        return ((Constructor)vobjs.get(strKey));
    }
    
    protected void fillHashMap()
    {
        addTool("button",               JButtonOperator.class);
        addTool("entry",                JTextFieldOperator.class);
        addTool("check",                JCheckBoxOperator.class);
        addTool("menu",                 JComboBoxOperator.class);
        addTool("radio",                JRadioButtonOperator.class);
        addTool("toggle",               JToggleButtonOperator.class);
    }
    
    protected void addTool(String strComp, Class classComp)
    {
        Class[] constructArgs = new Class[2];
        constructArgs[0] = ContainerOperator.class;
        constructArgs[1] = String.class;
        
        try {
            Constructor c = classComp.getConstructor(constructArgs);
            vobjs.put(strComp,c);
        }
        catch (NoSuchMethodException nse) {
            Messages.postError("Problem initiating " + strComp + " area.");
            Messages.writeStackTrace(nse, classComp +  " not found.");
        }
        catch (SecurityException se) {
            Messages.postError("Problem initiating " + strComp + " area.");
            Messages.writeStackTrace(se);
        }
    }
    
    public void log(String strTest, boolean bComp)
    {
        if (!bComp)
            System.out.println("********Running Test: " + strTest + "*******\n");
        else 
            System.out.println("********Completed Test: " + strTest + "******\n");
    }
    
    public void newMessage(int type, String s) {
        System.out.println(s);
    }
    
    // Add test methods here, they have to start with 'test' name.
    // for example:
    // public void testHello() {}
    
    class VSaxHandler extends DefaultHandler
    {
        JComponentOperator comp = null;

        public void endDocument() {
            // System.out.println("End of Document");
        }

        public void endElement(String uri, String localName, String qName) {
            // System.out.println("End of Element '"+qName+"'");
        }
        public void error(SAXParseException spe) {
            System.out.println("Error at line "+spe.getLineNumber()+
                                   ", column "+spe.getColumnNumber());
        }
        public void fatalError(SAXParseException spe) {
            System.out.println("Fatal error at line "+spe.getLineNumber()+
                                         ", column "+spe.getColumnNumber());
        }
        public void startDocument() {
            // System.out.println("Start of Document");
        }
        public void startElement(String uri,   String localName,
                                 String qName, Attributes attr) {
            //System.out.println("Start of Element '"+qName+"'");
            //int numOfAttr = attr.getLength();
            //System.out.println("   Number of Attributes is "+numOfAttr);
            //for (int i=0; i<numOfAttr; i++) {
            // System.out.println("   with attr["+i+"]='"+attr.getValue(i)+"'");
            //}
            if ( ! qName.equals("comp")) return;

            String strValue = attr.getValue("name");
            Constructor c = (Constructor)getTool(strValue);
            Object[] vargs = new Object[2];
            if (strValue.equalsIgnoreCase("command"))
                compTest(null, attr);
            else
            {
                strValue = attr.getValue("value");
                vargs = new Object[2];
                vargs[0] = main;
                vargs[1] = strValue;
            }
            
            if (c != null) {
                try {
                    comp = (JComponentOperator)c.newInstance(vargs);
                    compTest(comp, attr);
                }
                catch (Exception e) {
                    System.out.println(e.toString());
                }
            }
        }
        
        public void warning(SAXParseException spe) {
            System.out.println("Warning at line "+spe.getLineNumber()+
                                     ", column "+spe.getColumnNumber());
        }
    }
    
}
