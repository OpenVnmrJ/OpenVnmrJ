/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import vnmr.bo.EasyJTable;
import java.util.ArrayList;
import javax.swing.*;
import javax.swing.table.*;
import java.awt.*;
import java.awt.event.*;

/**
 * The purpose of this class is to display a dialog of "required parameters"
 * that have been determined by an administrator.  This dialog is generally
 * to be called up from the "reqpar" section of vjCcmd2() in ExpPanel.  That
 * command in turn is generally to be called from a macro.
 * 
 * This class has no public constructor and an object (Singleton) is to be
 * obtained via getReqparWarnDialog().  Then parameters are added using
 * addParameter.  After all the parameters are added, the dialog is displayed
 * with showDialog.  When the user clicks "OK" those parameter values are
 * sent to vnmr.  Then the object is disposed -- if another instance is needed,
 * getReqparWarnDialog() creates it as necessary.
 * 
 * The parameter is set within sendToVnmr(), a method of the ButtonIF
 * interface.
 * 
 * @author Jon Wurl
 *
 */
public class ReqparWarnDialog extends ModalDialog
                              implements ActionListener, WindowListener {
    private static ReqparWarnDialog reqparWarnDialog;
    private EasyJTable stringParameterTable;
    private EasyJTable realParameterTable;
    private DefaultTableModel stringParameterTableModel;
    private DefaultTableModel realParameterTableModel;
    private JPanel tablePanel;
    private ButtonIF buttonIF;		// need for sendToVnmr(String cmd)
    private String callbackCommandString;

    private final static String OK_COMMAND = "Ok";
    private final static String CANCEL_COMMAND = "Cancel";

    private ReqparWarnDialog() {
        super("Some required parameters missing ...");
        JScrollPane stringScrollPane;
        JScrollPane realScrollPane;
        GridBagConstraints gbc;

        String[] columnHeadings = {"parameter", "description", "enter value:"};

        tablePanel = new JPanel();

        realParameterTableModel = new DefaultTableModel(columnHeadings, 0);
        realParameterTable = new EasyJTable(realParameterTableModel);
        realParameterTable.getColumnModel().getColumn(0).setPreferredWidth(100);
        realParameterTable.getColumnModel().getColumn(1).setPreferredWidth(300);
        realParameterTable.getColumnModel().getColumn(2).setPreferredWidth(100);

        realScrollPane = new JScrollPane(realParameterTable);
        realParameterTable.setPreferredScrollableViewportSize(new Dimension(500, 100));   

        stringParameterTableModel = new DefaultTableModel(columnHeadings, 0);
        stringParameterTable = new EasyJTable(stringParameterTableModel);
        stringParameterTable.getColumnModel().getColumn(0).setPreferredWidth(100);
        stringParameterTable.getColumnModel().getColumn(1).setPreferredWidth(300);
        stringParameterTable.getColumnModel().getColumn(2).setPreferredWidth(100);


        stringScrollPane = new JScrollPane(stringParameterTable);
        stringParameterTable.setPreferredScrollableViewportSize(new Dimension(500, 100));   

        tablePanel.setLayout(new GridBagLayout());
        gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        tablePanel.add(new JLabel("Real parameters:"), gbc);
        gbc.gridy = 1;
        tablePanel.add(realScrollPane, gbc);
        gbc.gridy = 2;
        tablePanel.add(new JLabel("String parameters:"), gbc);
        gbc.gridy = 3;
        tablePanel.add(stringScrollPane, gbc);

        add(tablePanel);

        // these two are public in the superclass -- this is the
        // way the action commands need to be synchronized with the
        // choice in actionCommand
        okButton.setActionCommand(OK_COMMAND);
        cancelButton.setActionCommand(CANCEL_COMMAND);

        okButton.addActionListener(this);
        cancelButton.addActionListener(this);
        addWindowListener(this);

    }

    /**
     * Returns a Singleton instance of this class.  The instance stays "alive"
     * until dismissed via the ok button.
     * 
     * @param bif ButtonIF type, which has sendToVnmr().
     * @return Singleton instance of ReqparWarnDialog
     */
    public static ReqparWarnDialog getReqparWarnDialog(ButtonIF bif) {
        if (reqparWarnDialog == null) {
            reqparWarnDialog = new ReqparWarnDialog();
            reqparWarnDialog.buttonIF = bif;
        }
        return reqparWarnDialog;
    }

    /**
     * Add parameters one at a time to the dialog via this method.  Do this
     * prior to calling showDialog.
     * 
     * @param parameter name of parameter
     * @param isStringParameter true if the parameter is a string type (vnmr's
     * definition of "string")
     */
    public void addParameter(String parameter, boolean isStringParameter) {
        String paramDescription = "";

        // ParamResourceBundle returns parameter name if there is
        // no key entry in the properties file -- in our case, we
        // therefore want to make the string empty

	paramDescription = Util.getParamDescription(parameter);
        if (paramDescription.equals(parameter)) {
            paramDescription = "";
        }
        Object[] tempObject = new Object[3];
        tempObject[0] = new String(parameter);
        tempObject[1] = new String(paramDescription);
        tempObject[2] = new String("");
        if (isStringParameter) {
            stringParameterTableModel.addRow(tempObject);
        } else {
            realParameterTableModel.addRow(tempObject);
        }

    }

    /**
     * 
     * After all the paramteres have been added, call this method to show the
     * dialog.
     *
     */
    public void showDialog() {
        pack();
        setResizable(false);
        setVisible(true);
    }

    /**
     * After all the parameters have been added, call this method to show the
     * dialog.  This method also sets the command string to call back to
     * vnmr.  This is needed to synchronize between VnmrJ and vnmr -- basically
     * the calling macro needs to be re-entered when the Ok button is
     * hit, and this callback string is the re-entry command.
     */
    public void showDialog(String s) {
        callbackCommandString = new String(s);
        showDialog();
    }

    // implementation of ActionListener
    public void actionPerformed(ActionEvent e) {
        String cmd = e.getActionCommand();

        if (cmd.equals(OK_COMMAND)) {
            if (validAnswers() ) {
                if (callbackCommandString != null) {
                    buttonIF.sendToVnmr(callbackCommandString);
                }
                this.dispose();
                reqparWarnDialog = null;
            } else {
                this.setTitle("Invalid values -- please re-enter ...");
            }
        } else if (cmd.equals(CANCEL_COMMAND)) {
            this.dispose();
            reqparWarnDialog = null;    		
        }
    }

    // implementation of WindowListener
    public void windowActivated(WindowEvent e){}
    public void windowClosed(WindowEvent e) {}
    public void windowClosing(WindowEvent e) {
        System.out.println("windowClosing called");
        reqparWarnDialog = null;
    }
    public void windowDeactivated(WindowEvent e) {}
    public void windowDeiconified(WindowEvent e) {}
    public void windowIconified(WindowEvent e) {}
    public void windowOpened(WindowEvent e) {}

    private boolean validAnswers() {
        TableColumn column;
        boolean returnValue = true;
        int L;
        double x;
        String s;
        String vnmrCommand;

        L= realParameterTable.getRowCount();
        // count backwards, because we may remove rows
        for (int i=L-1; i>=0; i--) {
            try {
                x = (new Double( (String) realParameterTable.getValueAt(i,2))).doubleValue();
                vnmrCommand = (String) realParameterTable.getValueAt(i,0);      // parameter name
                vnmrCommand = vnmrCommand + "=" + x;
                buttonIF.sendToVnmr(vnmrCommand);
                realParameterTableModel.removeRow(i);
            } catch (NumberFormatException nfe) {
                // TODO: this does not work quite right, but I will leave it in
                // as a reminder
                //tcr = realParameterTable.getCellRenderer(i, 2);
                //if (tcr instanceof DefaultTableCellRenderer) {
                //	renderer = (DefaultTableCellRenderer) tcr;
                //	renderer.setBackground(Color.yellow.brighter());
                //}
                returnValue = false;
            }
        }

        L= stringParameterTable.getRowCount();
        // count backwards, because we may remove rows
        for (int i=L-1; i>=0; i--) {
            s = stringParameterTable.getValueAt(i,2).toString();
            if (s.equals("")) {
                returnValue = false;
            } else {
                vnmrCommand = (String) stringParameterTable.getValueAt(i,0);    // parameter name
                vnmrCommand = vnmrCommand + "='" + s + "'";
                buttonIF.sendToVnmr(vnmrCommand);
                stringParameterTableModel.removeRow(i);
            }
        }

        return returnValue;
    }

}
