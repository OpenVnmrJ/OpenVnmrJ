/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.ui;

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;
import java.text.*;

import java.awt.event.*;
import javax.swing.table.*;

import vnmr.util.*;
import vnmr.admin.util.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  not attributable
 *
 */

public class WAutomation extends ModalDialog implements ActionListener
{

    protected JTable m_table;
    protected JPanel m_pnlAutomation;
    protected JPanel m_pnlSample;

    protected static final String[] m_aStrColumn = {
        vnmr.util.Util.getLabel("_admin_Day"), 
        vnmr.util.Util.getLabel("_admin_Day-Q_Start"),
        vnmr.util.Util.getLabel("_admin_Max_Length"), 
        vnmr.util.Util.getLabel("_admin_Night-Q_Start"),
        vnmr.util.Util.getLabel("_admin_Max_Length")
   };

    public static final String SAMPLE = "SampleReuse";
    public static final String ONEQUEUE = "OneQueue";
    public static final String CHANGE_TIME = "ChangeTime";
    public static final String AUTOMATION = "SYSTEM/USRS/operators/automation.conf";

    public WAutomation(String helpfile)
    {
        super(vnmr.util.Util.getLabel("_admin_Automation_Configuration"), helpfile);
        dolayout();

        okButton.setActionCommand("ok");
        okButton.addActionListener(this);
        cancelButton.setActionCommand("cancel");
        cancelButton.addActionListener(this);
        helpButton.setActionCommand("help");
        helpButton.addActionListener(this);

        Dimension dim = m_pnlAutomation.getPreferredSize();
        setSize(dim.width+100, dim.height+70);
        setLocationRelativeTo(getParent());
    }

    public void actionPerformed(ActionEvent e)
    {
        String cmd = e.getActionCommand();
        if (cmd.equals("ok"))
        {
            m_table.getDefaultEditor(m_table.getColumnClass(1)).stopCellEditing();
            saveData();
            dispose();
        }
        else if (cmd.equals("cancel"))
            dispose();
        else if (cmd.equals("help"))
            displayHelp();

    }

    public void setVisible(boolean bShow)
    {
        if (bShow)
            buildPanel();
        super.setVisible(bShow);
    }

    protected void dolayout()
    {
        DefaultTableModel tablemodel = new DefaultTableModel();
        initializeTable(tablemodel);
        JScrollPane scrollPane = new JScrollPane(m_table);

        m_pnlAutomation = new JPanel(new BorderLayout());
        JPanel pnlTable = new JPanel();
        m_pnlSample = new JPanel(new WGridLayout(0, 2));
        Color bgColor = getParent().getBackground();
        // Stop displaying the scrollpane
//        pnlTable.add(scrollPane);
        m_pnlAutomation.add(pnlTable, BorderLayout.NORTH);
        m_pnlAutomation.add(m_pnlSample, BorderLayout.CENTER);
        scrollPane.setPreferredSize(new Dimension(400, 200));

        buildPanel();

        Container container = getContentPane();
        container.add(m_pnlAutomation);
    }

    protected void buildPanel()
    {
        String strPath = FileUtil.openPath(AUTOMATION);
        if (strPath == null)
        {
            Messages.postError("File " + AUTOMATION + " not found");
            return;
        }

        BufferedReader reader = WFileUtil.openReadFile(strPath);
        if (reader == null)
            return;
        DefaultTableModel model = (DefaultTableModel)m_table.getModel();
        model.setDataVector(null, m_aStrColumn);
        m_pnlSample.removeAll();

        boolean foundOneQueue = false;
        String strLine;
        try
        {           
            while ((strLine = reader.readLine()) != null)
            {
                strLine = strLine.trim();
                if (strLine.startsWith("\ufeff"))
                    strLine=strLine.substring(1);
                if (strLine.startsWith("#") || strLine.equals(""))
                    continue;
                // put the queue information in the table
                if ( ! strLine.startsWith(SAMPLE) && ! strLine.startsWith(CHANGE_TIME) && ! strLine.startsWith(ONEQUEUE) )
                {
                    Object[] row = WUtil.strToAList(strLine, true).toArray();
                    // convert the day string from the default format (en_US) to that of the current locale
                    row[0]=Util.getShortWeekday((String)row[0]);                   
                    model.addRow(row);
                }
                // put the automation information in the panel
                else if (strLine.startsWith(CHANGE_TIME)) {
               	    addChangeTime(CHANGE_TIME, strLine, m_pnlSample);
                	}
                else if (strLine.startsWith(SAMPLE)) {
                    addData(SAMPLE, strLine, m_pnlSample);
                        }
                else if (strLine.startsWith(ONEQUEUE)) {
                    foundOneQueue = addOneQueue(ONEQUEUE, strLine, m_pnlSample);
                	}
            }
            reader.close();
            if (! foundOneQueue)  // add OneQueue if not found
            {
                foundOneQueue = addOneQueue(ONEQUEUE, "OneQueue    no", m_pnlSample);
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    protected void initializeTable(DefaultTableModel model)
    {
        m_table = new JTable(model);
        model.setColumnIdentifiers(m_aStrColumn);
        m_table.getTableHeader().setReorderingAllowed(false);

        JSpinner spinner = new JSpinner(createModel());
        spinner.setEditor(new JSpinner.DateEditor(spinner, "HH:mm"));

        WCellEditor cellEditor = new WCellEditor(spinner);
        m_table.setDefaultEditor(m_table.getColumnClass(1), cellEditor);
    }

    private SpinnerModel createModel()
    {
        //Add the third label-spinner pair.
        Calendar calendar = Calendar.getInstance();
        Date initDate = calendar.getTime();

        SpinnerDateModel dateModel = new SpinnerDateModel(initDate, null,null,
                                                          Calendar.HOUR_OF_DAY);
        return dateModel;
    }

    protected void saveData()
    {
        String strPath = FileUtil.openPath(AUTOMATION);
        StringBuffer sbData = new StringBuffer();
        if (strPath == null)
            strPath = FileUtil.savePath(AUTOMATION);

        sbData.append(getFileData(strPath).toString());
        sbData.append(getTableData().toString());
        sbData.append("#").append("\n");
        sbData.append(getPanelData());

        BufferedWriter writer = WFileUtil.openWriteFile(strPath);
        WFileUtil.writeAndClose(writer, sbData);

    }

    protected StringBuffer getFileData(String strPath)
    {
        StringBuffer sbData = new StringBuffer();
        BufferedReader reader = WFileUtil.openReadFile(strPath);
        String strLine;
        if (reader == null)
            return sbData;

        try
        {
            // write the comments in the file
            while ((strLine = reader.readLine()) != null)
            {
                if (strLine.startsWith("#"))
                    sbData.append(strLine).append("\n");
                else
                    break;
            }
            reader.close();

        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
        return sbData;
    }

    protected StringBuffer getTableData()
    {
        StringBuffer sbData = new StringBuffer();
        // write the table contents in the file
        Vector vecData = ((DefaultTableModel)m_table.getModel()).getDataVector();
        int nLength = vecData.size();
        for (int i = 0; i < nLength; i++)
        {
            Vector vecRow = (Vector)vecData.get(i);
            int nLength2 = vecRow.size();
            for (int j = 0; j < nLength2; j++)
            {
                String strValue = (String)vecRow.get(j);
                if(j==0)  // save the day string in the default format (en_US)
                    strValue=Util.getDfltShortWeekday(strValue);
                sbData.append(strValue).append("         ");
            }
            sbData.append("\n");
        }
        return sbData;
    }

    protected String getPanelData()
    {
        StringBuffer sbData = new StringBuffer();
       
        int nSize = m_pnlSample.getComponentCount();
        for (int i = 0; i < nSize; i++)
        {
            Component comp = m_pnlSample.getComponent(i);
            if (comp instanceof JTextField) {
            	sbData.append( comp.getName() );
            	sbData.append("       ");
            	sbData.append( ((JTextField)comp).getText() );
            	sbData.append("\n");
            }
            if (comp instanceof JCheckBox)  {
            	sbData.append( comp.getName() );
                boolean bSelect = ((JCheckBox)comp).isSelected();
                String strValue = bSelect ? "yes" : "no";
                sbData.append("      ");
                sbData.append(strValue).append("\n");
            }
        }

        return sbData.toString();
    }

    protected void addData(String name, String strLine, JPanel panel)
    {
        StringTokenizer strTok = new StringTokenizer(strLine);
        String strLabel = "";
        String strValue = "";

        if (strLine == null || strLine.trim().equals(""))
            return;

        if (strTok.hasMoreTokens())
            strLabel = strTok.nextToken();
        if (strTok.hasMoreTokens())
            strValue = strTok.nextToken();
        JCheckBox checkbox = new JCheckBox();
        JLabel reuseLabel = new JLabel(vnmr.util.Util.getLabel("_admin_Sample_Reuse"));
        panel.add(checkbox);
        panel.add(reuseLabel);
        checkbox.setBackground(panel.getBackground());
        boolean bSelect = strValue.equalsIgnoreCase("yes");
        checkbox.setSelected(bSelect);
        checkbox.setName(name);
    }

    protected boolean addOneQueue(String name, String strLine, JPanel panel)
    {
        StringTokenizer strTok = new StringTokenizer(strLine);
        String strLabel = "";
        String strValue = "";

        if (strLine == null || strLine.trim().equals(""))
            return(false);

        if (strTok.hasMoreTokens())
            strLabel = strTok.nextToken();
        if (strTok.hasMoreTokens())
            strValue = strTok.nextToken();
        JCheckBox oneQcheckbox = new JCheckBox();
        JLabel oneQLabel = new JLabel(vnmr.util.Util.getLabel("_admin_One_Queue"));
        // Stop displaying these items in the panel
        oneQcheckbox.setVisible(false);
        oneQLabel.setVisible(false);
        panel.add(oneQcheckbox);
        panel.add(oneQLabel);
        oneQcheckbox.setBackground(panel.getBackground());
        boolean qSelect = strValue.equalsIgnoreCase("yes");
        oneQcheckbox.setSelected(qSelect);
        oneQcheckbox.setName(name+"   "); // pad name with blanks
        return(true);
    }

    protected void addChangeTime(String name, String strLine, JPanel panel)
    {
        StringTokenizer strTok = new StringTokenizer(strLine);
        String strLabel = "";
        String strValue = "";

        if (strLine == null || strLine.trim().equals(""))
            return;

        if (strTok.hasMoreTokens())
            strLabel = strTok.nextToken();
        if (strTok.hasMoreTokens())
            strValue = strTok.nextToken();
        JTextField changeTimeEntry  = new JTextField(strValue, 3);
        JLabel changeTimeText1 = new JLabel(vnmr.util.Util.getLabel("_admin_Sample_Change_Time"));    
        // Stop displaying these items in the panel
        changeTimeEntry.setVisible(false);
        panel.add(changeTimeEntry);
        changeTimeEntry.setBackground(Color.WHITE);
        // Stop displaying these items in the panel
        changeTimeText1.setVisible(false);
        panel.add(changeTimeText1);
        changeTimeEntry.setName(name);
    }
}

class WCellEditor extends AbstractCellEditor implements TableCellEditor
{

    protected JSpinner m_spinner;
    protected Component m_editor;
    protected int m_nColumn;

    public WCellEditor(JSpinner spinner)
    {
        m_spinner = spinner;
    }

    public Component getTableCellEditorComponent(JTable table, Object value,
                                                 boolean isSelected, int row, int col)
    {
        if (col == 0)
            m_editor = null;
        else
        {
            m_editor = m_spinner;
            setValue(value);
        }
        m_nColumn = col;
        return m_editor;
    }

    public void setValue(Object value)
    {
        if (m_editor == null || value == null)
            return;

        if (value instanceof String)
        {
            int nIndex = ((String)value).indexOf(":");
            if (nIndex < 0)
                return;
            int hour = Integer.parseInt(((String)value).substring(0, nIndex));
            int minute = Integer.parseInt(((String)value).substring(nIndex+1));
            Calendar calendar = Calendar.getInstance();
            calendar.set(calendar.get(Calendar.YEAR), calendar.get(Calendar.MONTH),
                         calendar.get(Calendar.DATE), hour, minute);
                value = calendar.getTime();
        }
        m_spinner.setValue(value);
    }

    public Object getCellEditorValue()
    {
        if (m_editor == null)
            return null;
        Object value = m_spinner.getValue();
        if (value instanceof Date)
        {
            Calendar calendar = Calendar.getInstance();
            calendar.setTime((Date)value);
            int hour = calendar.get(Calendar.HOUR_OF_DAY);
            int minute = calendar.get(Calendar.MINUTE);
            StringBuffer sbData = new StringBuffer();
            if ((m_nColumn == 1 || m_nColumn == 3) && (hour == 0))
                sbData.append("0");
            sbData.append(hour).append(":");
            if (minute >= 0 && minute < 10)
                sbData.append("0");
            sbData.append(minute);
            value = sbData.toString();
        }
        return value;
    }

    public boolean stopCellEditing()
    {
        if (m_editor != null)
        {
            // Commit edited value.
            try
            {
                m_spinner.commitEdit();
            }
            catch (Exception e) { }
        }
        return super.stopCellEditing();
    }
}
