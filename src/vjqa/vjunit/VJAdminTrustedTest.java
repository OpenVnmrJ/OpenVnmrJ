/*
 * VJAdminTrustedTest.java
 * JUnit based test
 *
 * Created on October 13, 2004, 1:47 PM
 */

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;
import java.awt.event.*;
import junit.framework.*;
import org.netbeans.jemmy.*;
import org.netbeans.jemmy.operators.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.ui.*;
import vnmr.admin.util.*;

/**
 *
 * @author mrani
 */
public class VJAdminTrustedTest extends TestCase {
    
    protected Thread threadVJ;
    protected JFrameOperator main;
    protected String strUser = "user914";
    
    public VJAdminTrustedTest(java.lang.String testName) {
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
        TestSuite suite = new TestSuite(VJAdminTrustedTest.class);
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
                    createUser();
                    setItype();
                    createOperators();
                    setOperators();
                    setPreferences();
                    resetPassword();
                    deleteOperators();
                    setDataDir();
                    setUserTemplates();
                    convertUser();
                    deleteUser();
                    restoreUser();
                }
                catch (Exception e)
                {
                    e.printStackTrace();
                }
            }
        });
        
        threadVJ.start();
        
    }
    
    public void createUser() throws InterruptedException
    {
        log("Create User");
        Calendar calendar = Calendar.getInstance();
        strUser = "tmp"+calendar.get(Calendar.MONTH)+calendar.get(Calendar.DATE);
        int i = 1;
        strUser = strUser + i;
        while (WUserUtil.isVjUser(strUser))
        {
            strUser = strUser.substring(0, strUser.length()-1)+i;
            i = i+1;
        }
        
        JButtonOperator btn = new JButtonOperator(main, "New User");
        btn.doClick();
        JTextFieldOperator txt = new JTextFieldOperator(main, "Enter login name and press 'RETURN'");
        txt.clearText();
        txt.typeText(strUser);
        JRadioButtonOperator radibtn = new JRadioButtonOperator(main, "Experimental");
        radibtn.clickMouse();
        setUserField(null, true, "name", strUser);

        btn = new JButtonOperator(main, "Save User");
        btn.clickMouse();
        
        threadVJ.sleep(60000);
        
        assertTrue(WUserUtil.isVjUser(strUser));
        
        isEntryLogged("New user '"+strUser+"' created.");
    }
    
    public void setItype()
    {
        log("setItype");
        JButtonOperator btn = new JButtonOperator(main, strUser);
        btn.doClick();
        JRadioButtonOperator rdbtn = new JRadioButtonOperator(main, "Walkup");
        rdbtn.doClick();
        btn = new JButtonOperator(main, "Save User");
        btn.doClick();
        
        isEntryLogged("Modified itype");
    }
    
    public void createOperators()
    {
        log("createOperators");
        String strOperator = strUser.replaceAll("user", "oper");
        JMenuBarOperator menubar = new JMenuBarOperator(main);
        menubar.pushMenuNoBlock("Configure|Operators|Edit Operators", "|");
        JDialogOperator frame = new JDialogOperator("Modify Operators");
        JTableOperator table = new JTableOperator(frame);
        table.clickMouse(1, InputEvent.BUTTON3_MASK);
        int row = table.getRowCount()-1;
        table.clickForEdit(row, 0);
        table.changeCellObject(row, 0, strOperator+"a");
        table.clickForEdit(row, 1);
        table.changeCellObject(row, 1, strOperator+"a@abc.com");
        table.changeCellObject(row, 3, strOperator+"a");
        table.clickMouse(1, InputEvent.BUTTON3_MASK);
        row = table.getRowCount()-1;
        table.clickForEdit(row, 0);
        table.changeCellObject(row, 0, strOperator+"b");
        table.clickForEdit(row, 1);
        table.changeCellObject(row, 1, strOperator+"b@abc.com");
        table.changeCellObject(row, 3, strOperator+"b");
        table.clickMouse(1, InputEvent.BUTTON3_MASK);
        row = table.getRowCount()-1;
        table.clickForEdit(row, 0);
        table.changeCellObject(row, 0, strOperator+"b");
        table.clickForEdit(row, 1);
        table.changeCellObject(row, 1, strOperator+"b@abc.com");
        JButtonOperator btn = new JButtonOperator(frame, "OK");
        btn.clickMouse();
        
        isEntryLogged("Created operator");
    }
    
    public void setOperators()
    {
        log("setOperators");
        String strOperator = strUser.replaceAll("user", "oper");
        JButtonOperator btn = new JButtonOperator(main, strUser);
        btn.doClick();
        setUserField(null, true, "operators", "oper1, oper2, " + 
                        strOperator+"a, " + strOperator+"b");
        btn = new JButtonOperator(main, "Save User");
        btn.doClick();
        
        isEntryLogged("Modified operators");
    }
    
    public void setPreferences()
    {
        log("setPreferences");
        JMenuBarOperator menubar = new JMenuBarOperator(main);
        menubar.pushMenuNoBlock("Configure|Operators|Preferences", "|");
        JDialogOperator frame = new JDialogOperator("Preferences");
        JTextFieldOperator txt = new JTextFieldOperator(frame);
        txt.setText("abc123");
        JButtonOperator btn = new JButtonOperator(frame, "OK");
        btn.doClick();
        
        isEntryLogged("Set");
    }
    
    public void resetPassword()
    {
        log("resetPassword");
        String strOperator = strUser.replaceAll("user", "oper");
        JMenuBarOperator menubar = new JMenuBarOperator(main);
        menubar.pushMenuNoBlock("Configure|Operators|Reset Password", "|");
        JDialogOperator frame = new JDialogOperator("Reset Operators' Password");
        JTextFieldOperator txt = new JTextFieldOperator(frame);
        txt.setText(strOperator+"a, "+strOperator+"b");
        JButtonOperator btn = new JButtonOperator(frame, "OK");
        btn.doClick();
        
        isEntryLogged("Reset Password");
    }
    
    public void deleteOperators()
    {
        log("deleteOperators");
        String strOperator = strUser.replaceAll("user", "oper");
        JMenuBarOperator menubar = new JMenuBarOperator(main);
        menubar.pushMenuNoBlock("Configure|Operators|Delete Operators","|");
        JDialogOperator frame = new JDialogOperator("Modify Operators");
        JCheckBoxOperator chk = new JCheckBoxOperator(frame, strOperator+"a");
        chk.clickMouse();
        chk = new JCheckBoxOperator(frame, strOperator+"b");
        chk.clickMouse();
        System.out.println("menu operator " + chk.getText());
        JButtonOperator btn = new JButtonOperator(frame, "OK");
        btn.clickMouse();
        
        isEntryLogged("Deleted ");
    }
    
    public void setDataDir()
    {
        log("setDataDir");
        JButtonOperator btn = new JButtonOperator(main, strUser);
        btn.doClick();
        JTabbedPaneOperator tabPane = new JTabbedPaneOperator(main);
        btn = new JButtonOperator(tabPane, "New Label");
        btn.doClick();
        JTextFieldOperator txt = new JTextFieldOperator(tabPane, 0);
        String strValue = txt.getText();
        int i = 1;
        while (strValue != null && !strValue.equals(""))
        {
            txt = new JTextFieldOperator(tabPane, i);
            strValue = txt.getText();
            i = i+1;
        }
        txt.setText("data2");
        txt = new JTextFieldOperator(tabPane, "Enter");
        txt.setText("/vnmr");
        
        btn = new JButtonOperator(main, "Save User");
        btn.doClick();
        
        isEntryLogged("Added Data Dir");
    }
    
    public void setUserTemplates()
    {
        log("setUserTemplate");
        JButtonOperator btn = new JButtonOperator(main, strUser);
        btn.doClick();
        JTabbedPaneOperator tabPane = new JTabbedPaneOperator(main);
        tabPane.selectPage("User Templates");
        btn = new JButtonOperator(tabPane, "New Label");
        btn.doClick();
        JTextFieldOperator txt = new JTextFieldOperator(tabPane, 0);
        String strValue = txt.getText();
        int i = 1;
        while (strValue != null && !strValue.equals(""))
        {
            txt = new JTextFieldOperator(tabPane, i);
            strValue = txt.getText();
            i = i+1;
        }
        txt.setText("template2");
        txt = new JTextFieldOperator(tabPane, i);
        txt.setText("%template%");
        
        btn = new JButtonOperator(main, "Save User");
        btn.doClick();
        
        isEntryLogged("Added");
    }
    
    public void convertUser() throws InterruptedException
    {
        log("convertUser");
        int i = Integer.parseInt(strUser.substring(strUser.length()-1))+1;
        String strUser2 = strUser.substring(0, strUser.length()-1)+i;
        WUserUtil.createNewUnixUser(strUser2, "/export/home");
        threadVJ.sleep(60000);
        JMenuBarOperator menu = new JMenuBarOperator(main);
        menu.pushMenu("Configure|Users|Convert Users", "|");
        JDialogOperator frame = new JDialogOperator("Convert");
        JListOperator list = new JListOperator(frame, 0);
        list.selectItem(strUser2);
        for (i = 0; i < 10; i++)
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
        threadVJ.sleep(60000);        
    }
    
    public void deleteUser()
    {
        log("Delete User");
        JButtonOperator btn = new JButtonOperator(main, strUser);
        btn.clickForPopup(MouseEvent.BUTTON3_MASK);
        JPopupMenuOperator menu = new JPopupMenuOperator(main);
        JMenuItemOperator item = new JMenuItemOperator(menu, "Delete");
        item.doClick();
        
        isEntryLogged("Deleted '" + strUser + "'");
    }
    
    public void restoreUser()
    {
        log("Restore User");
        WUserUtil.restoreUser("newUser1", "/export/home");
        
        isEntryLogged("Restored '" + strUser + "'");
    }
    
    public void setUserField(JComponent comp, boolean bFrame, String strKey,
                             String strValue)
    {
        int nSize = 0;
        if (bFrame)
            nSize = main.getComponentCount();
        else
            nSize = comp.getComponentCount();
        for (int i = 0; i < nSize; i++)
        {
            Component comp2 = null;
            if (bFrame)
                comp2 = main.getComponent(i);
            else
                comp2 = comp.getComponent(i);    
            
            if (comp2 instanceof VDetailArea)
            {
                comp2 = ((VDetailArea)comp2).getPnlComp((JComponent)comp2, strKey, true);
                if (comp2 instanceof JTextField)
                    ((JTextField)comp2).setText(strValue);
                break;
            }
            else if (comp2 instanceof JComponent)
                setUserField((JComponent)comp2, false, strKey, strValue);
        }
    }
    
    public JTable getTable(JComponent comp, JTable table)
    {
        int nSize = comp.getComponentCount();
        for (int i = 0; i < nSize; i++)
        {
            Component comp2 = comp.getComponent(i);
            if (comp2 instanceof JTable)
            {
                table = ((JTable)comp2);
                break;
            }
            else if (comp2 instanceof JComponent)
                return getTable((JComponent)comp2, table);
        }
        return table;
    }
    
    public void isEntryLogged(String strValue) 
    {
        try
        {
            threadVJ.sleep(2000);
            if (!Util.isPart11Sys())
                return;
            JMenuBarOperator menu = new JMenuBarOperator(main);
            menu.pushMenu("Management|Auditing", "|");
            VAdminIF appIf = (VAdminIF)Util.getAppIF();
            JComponent comp = appIf.getItemArea2();
            JComboBoxOperator cmb = new JComboBoxOperator(main, 0);
            cmb.selectItem("user account audit trails");
            JTableOperator table = new JTableOperator(getTable(comp, null));
            String strValue2 = (String)table.getValueAt(table.getRowCount()-1, 
                                                        table.getColumnCount()-1);
            if (strValue2 == null || strValue2.indexOf(strValue) < 0)
                System.out.println("Error : " + strValue + " not found");
        
            menu.pushMenu("Management|Users", "|");
            threadVJ.sleep(2000);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
    
    public void log(String strTest)
    {
        System.out.println("\n****** Running Test: " + strTest + " *********\n");
    }
    
    // Add test methods here, they have to start with 'test' name.
    // for example:
    // public void testHello() {}
    
    
}
