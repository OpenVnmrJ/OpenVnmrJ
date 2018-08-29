/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.util;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.beans.*;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.basic.BasicFileChooserUI;

import vnmr.util.*;
import vnmr.admin.ui.*;
import vnmr.ui.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p>Company: </p>
 * @author unascribed
 * @version 1.0
 */

public class WDirChooser extends JFileChooser
{

    protected WDirChooser m_dirChooser;

    /** Property Change Support object.  */
    protected static PropertyChangeSupport m_pcsTypesMgr;

    public WDirChooser()
    {
        this(null);
    }

    public WDirChooser(Container container)
    {
        super();

        m_dirChooser = this;
        setApproveButtonText(vnmr.util.Util.getLabel("_admin_Enter_Directory","Enter Directory"));
        setApproveButtonToolTipText( vnmr.util.Util.getLabel(
          "_admin_Enters_selected_directory_to_the_left_panel_in_the_empty_directory_field",
          "Enters selected directory to the left panel in the empty directory field")
        );
        m_pcsTypesMgr=new PropertyChangeSupport(this);

        setComps(this);

        // Change the listener for the new folder button
        setupNewFolderBtn();

        if (container instanceof ModelessDialog)
            setControlButtonsAreShown(false);

        addActionListener(new ActionListener()
        {
            public void actionPerformed(ActionEvent e)
            {
                File file = m_dirChooser.getSelectedFile();
                m_pcsTypesMgr.firePropertyChange(WGlobal.INFODIR, "all", file.getPath());
            }
        });

    }

    /**
     *  Sets the visibility of the components in the default filechooser.
     */
    protected void setVisibility(JComponent comp)
    {
        // don't show the help and cancel buttons.
        if (comp instanceof JButton)
        {
            JButton btn = (JButton)comp;
            String strTitle = btn.getText();
            strTitle = (strTitle != null) ? strTitle.toLowerCase(): "";
            
            if (strTitle.equals(vnmr.util.Util.getLabel("_admin_help","help")) 
                    || strTitle.equals(vnmr.util.Util.getLabel("_admin_cancel","cancel")))
            {
                btn.setVisible(false);
            }
        }
        // don't show the filename panel.
        else if (comp instanceof JPanel)
        {
            JPanel pnlComp = (JPanel)comp;
            boolean bFileName = false;
            int nCompCount = pnlComp.getComponentCount();

            for (int i = 0; i < nCompCount; i++)
            {
                Component compCh = pnlComp.getComponent(i);
                if (compCh instanceof JLabel)
                {
                    String strTitle = ((JLabel)compCh).getText();
                    strTitle = (strTitle != null) ? strTitle.toLowerCase() : "";
                    bFileName = (strTitle.indexOf(vnmr.util.Util.getLabel("_file_name","file name")) >= 0) ? true : false;
                }
                if (bFileName && compCh instanceof JTextField)
                    compCh.setVisible(false);
            }
        }
        // don't show the text area.
        else if (comp instanceof JTextArea)
        {
            comp.setVisible(false);
        }
        // don't show the filename label.
        else if (comp instanceof JLabel)
        {
            String strName = ((JLabel)comp).getText();
            strName = strName.toLowerCase();
            if (strName.indexOf(vnmr.util.Util.getLabel("_file_name","file name")) >= 0
                    || strName.indexOf(vnmr.util.Util.getLabel("_files_of_type","files of type")) >= 0)
                comp.setVisible(false);
        }
        else if (comp instanceof JComboBox)
        {
            Object objItem = ((JComboBox)comp).getSelectedItem();
            if (objItem instanceof javax.swing.filechooser.FileFilter)
            {
                comp.setVisible(false);
            }
        }
    }

    /**
     *  This method recursively looks at all components in the default
     *  file chooser, and then sets the visibility for the components
     *  that are needed for our purpose.
     */
    private Component setComps(Component comp)
    {
        if (comp.getClass() == JButton.class)
            setVisibility((JButton)comp);

        if (comp instanceof Container)
        {
            Component[] components = ((Container)comp).getComponents();
            for(int i = 0; i < components.length; i++)
            {
                Component child = components[i];
                if (child instanceof JComponent)
                {
                    setVisibility((JComponent)child);
                }
                child = setComps(components[i]);
                if (child != null)
                    return child;
            }
        }
        return null;
    }


    /** 
     *  This method adds an action listener for the 'new folder' button.
     *  We need to catch that event ourselves after the directory is
     *  created so we can make the directory to be owned by the user being 
     *  edited.
     *
     *  There is no normal java class way to do this that I know of, so
     *  we will dig our way to get the new folder object.  
     *  The panels and buttons for the JFileChooser are created in
     *  the Java file MetalFileChooserUI.java .  Based on looking at
     *  that file and the order or creation I found that:
     *  Item 0 of the JFileChooser is topPanel containing 
     *  Item 0 of topPanel is topButtonPanel 
     *  Item 4 of topButtonPanel is newFolderButton
     */
    public void setupNewFolderBtn() {

        JPanel topPanel = (JPanel) getComponent(0);
        JPanel topButtonPanel = (JPanel) topPanel.getComponent(0);
        JButton newFolderButton =(JButton)topButtonPanel.getComponent(4);

        // If this is correct, the Actioncommand for newFolderButton
        // will be 'New Folder' If this is not correct, then we probably
        // changed Java versions and the code adding buttons and
        // panels is probably different.  A programmer will need to
        // look into the Java Code in MetalFileChooserUI.java
        // to see how to fix this.
        String cmd = newFolderButton.getActionCommand();
        Icon icon = newFolderButton.getIcon();

        if(!cmd.equals("New Folder")) {
            Messages.postError("Problem with buttons in WDirChooser. "
                         + "This is probably due\n    to a change "
                         + "in the Java version.  This needs to "
                         + "be repaired in\n    the code."
                         + "  File WDirChooser.java (search \'New Folder\')");
        }

        // Now we need to get the previous action listener
        // from the newFolderButton.  Assume there is only one.
        ActionListener[] list = newFolderButton.getActionListeners();
        // Remove the normal listener
        newFolderButton.removeActionListener(list[0]);

        // Then add our new listener
        newFolderButton.addActionListener(new NewFolderAction());
        
        // For reasons I don't understand, removing the action listener
        // seems to remove the icon for this button.  That is not true
        // for other buttons.  nontheless, I need to save the icon,
        // reset the listener and then reset the icon back.
        newFolderButton.setIcon(icon);
        
    }

    protected class NewFolderAction extends AbstractAction {
        protected NewFolderAction() {
            super("New Folder");
        }

        public void actionPerformed(ActionEvent e) {
            // Get the directory where to make the new directory
            File dir = getCurrentDirectory();
            String dirpath = dir.getPath();
            
            // Get user being edited
            String user = VItemArea1.getCurAdminToolUser();
            if(user == null || user.trim().length() == 0) {
                Messages.postError("Must have a user selected to create" 
                            + " a new folder.");
                return;
            }
            
            // Get folder name to use via popup
            ModalEntryDialog med;

            med = new ModalEntryDialog(
                "New Folder Name",
                "Enter Name for New Folder");

            String folder  =  med.showDialogAndGetValue(null);
            
            // Does this folder already exist?
            File folderpath = new File(dirpath + File.separator + folder);
            if(folderpath.exists()) {
                Messages.postError("The folder " + folderpath + "" +
                        "\n     already exists, Aborting creation.");
                return;
            }
            
            try {
                // Create a directory owned by the currently edited user
                String cmd = WGlobal.SUDO + " /bin/mkdir " + folderpath;
                Runtime rt = Runtime.getRuntime();
                Process prcs = null;
                BufferedReader str=null;
                String output=null;
                try {
                    prcs = rt.exec(cmd);
                    // Wait for it to complete creating the folder
                    prcs.waitFor();

                    // Get any feedback from the command
                    str = (new BufferedReader
                                          (new InputStreamReader
                                           (prcs.getErrorStream())));

                    output = str.readLine();
                    if(output != null) {
                        Messages.postError(output + "\n    If this directory "
                                           + "is located on a network drive, "
                                           + "the error\n    is probably because root on"
                                           + " the local computer does not\n    have write"
                                           + " access on the network drive.  The new folder\n" 
                                           + "    will need to be created manually.");
                        return;
                    }
                }
                finally {
                    // It is my understanding that these streams are left
                    // open sometimes depending on the garbage collector.
                    // So, close them.
                    if(prcs != null) {
                        OutputStream os = prcs.getOutputStream();
                        if(os != null)
                            os.close();
                        InputStream is = prcs.getInputStream();
                        if(is != null)
                            is.close();
                        is = prcs.getErrorStream();
                        if(is != null)
                            is.close();
                    }
                }

                try {
                    // Now we need to change the owner to the desired user
                    cmd = WGlobal.SUDO + " /bin/chown " + user + " " 
                        + folderpath;
                    prcs = rt.exec(cmd);
                    // Wait for it to complete creating the folder
                    prcs.waitFor();

                    str = (new BufferedReader
                           (new InputStreamReader
                            (prcs.getErrorStream())));

                    output = str.readLine();
                    if(output != null) {
                        Messages.postError(output + "\n    If this directory "
                                           + "is located on a network drive, "
                                           + "the error\n    is probably because root on"
                                           + " the local computer does not\n    have write"
                                           + " access on the network drive.  The new folder\n" 
                                           + "    will need to be created manually.");
                        return;
                    }
                }
                finally {
                    // It is my understanding that these streams are left
                    // open sometimes depending on the garbage collector.
                    // So, close them.
                    if(prcs != null) {
                        OutputStream os = prcs.getOutputStream();
                        if(os != null)
                            os.close();
                        InputStream is = prcs.getInputStream();
                        if(is != null)
                            is.close();
                        is = prcs.getErrorStream();
                        if(is != null)
                            is.close();
                    }
                }
            }
            catch (Exception ex) {
                Messages.writeStackTrace(ex);
            
            }
            
            rescanCurrentDirectory();

        }
    }


    //==============================================================================
   //   PropertyChange methods follows ...
   //============================================================================

    /**
     *  Adds the specified change listener.
     *  @param l    the property change listener to be added.
     */
    public static void addChangeListener(PropertyChangeListener l)
    {
        if (m_pcsTypesMgr != null)
            m_pcsTypesMgr.addPropertyChangeListener(l);
    }

    /**
     *  Adds the specified change listener with the specified property.
     *  @param strProperty  the property to which the listener should be listening to.
     *  @param l            the property change listener to be added.
     */
    public static void addChangeListener(String strProperty, PropertyChangeListener l)
    {
        if (m_pcsTypesMgr != null)
            m_pcsTypesMgr.addPropertyChangeListener(strProperty, l);
    }

    /**
     *  Removes the specified change listener.
     */
    public static void removeChangeListener(PropertyChangeListener l)
    {
        if(m_pcsTypesMgr != null)
            m_pcsTypesMgr.removePropertyChangeListener(l);
    }


}
