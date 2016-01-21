/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package accounting;

import java.awt.*;
import java.io.BufferedWriter;
import java.io.IOException;
import java.text.FieldPosition;
import java.text.SimpleDateFormat;
import java.util.*;
import javax.swing.*;
import javax.swing.table.*;

public class SpinTable2 extends JTable {

  static Font f = new Font("Serif",Font.BOLD,12);
  ArrayList  spinRows   = new ArrayList();
  OneSpinRow oneSpinRow;
  MyTableModel mtm;

  public SpinTable2() {
//    setBackground(color);
    mtm = new MyTableModel();
    setModel( mtm );
    JTableHeader jth = getTableHeader();
    jth.setFont(f);
    
    this.setDefaultRenderer(DaySpinner.class,  new DaySpinRenderer());
    this.setDefaultEditor  (DaySpinner.class,  new DaySpinEditor());
    this.setDefaultRenderer(RateSpinner.class, new DaySpinRenderer());
    this.setDefaultEditor  (RateSpinner.class, new DaySpinEditor());
  }

  public int checkOrder(JPanel p) {
    Date d1,d2;
    String msgStr;
    Date day1,day2;
    Calendar cal = new GregorianCalendar();
    int dow1, dow2, cmpTime;
    String[] options = {"Save","Cancel"};
    int mri = spinRows.size(); // number of rows: 1,2,3...
    int result = 0;
    
    for (int i = 1; i< mri; i++) {
        day1 = (Date)((OneSpinRow)spinRows.get(i-1)).day.getValue();
        day2 = (Date)((OneSpinRow)spinRows.get(i)).day.getValue();
        cal.setTime(day1);
        dow1 = cal.get(Calendar.DAY_OF_WEEK);
        cal.setTime(day2);
        dow2 =cal.get(Calendar.DAY_OF_WEEK);
//        System.out.println("==>>"+day1+"==="+day2+"<<==");
//        System.out.println("==>>"+dow1+"==="+dow2+"<<==");
        
        if (dow1 < dow2) continue;
        d1 = (Date) ((OneSpinRow)spinRows.get(i-1)).time.getValue();
        d2 = (Date) ((OneSpinRow)spinRows.get(i)).time.getValue();
//        System.out.println("==>>"+d1+"==="+d2+"<<==");
        cmpTime = d1.compareTo(d2);
        if ( ((cmpTime > 0) && (dow1==dow2)) || (dow1>dow2) ) {
            JOptionPane pane = new JOptionPane();
            msgStr = new String("Row "+(i+1)+" is out of chronological order\nDo you want to Save or Cancel and fix this");
            result = pane.showOptionDialog(p,msgStr,"Warning",
                    JOptionPane.YES_NO_OPTION, JOptionPane.QUESTION_MESSAGE,
                    null,options, options[0] );
//            System.out.println("===>>result = "+result+" <<===");
            if (result != 0)
                return(result);
        }
    }
    return (result);
  }

  public void save(BufferedWriter bufOut)  throws IOException {
    int mri = spinRows.size(); // number of rows: 1,2,3...
    bufOut.write("#Table Rows information");
    bufOut.newLine();

    SimpleDateFormat sdf;
    for (int i = 0; i<mri; i++) {
      StringBuffer buf = new StringBuffer(16);
      oneSpinRow = (OneSpinRow)spinRows.get(i);
      sdf = new SimpleDateFormat("EEE");
      sdf.format((Date)oneSpinRow.day.getValue(),buf,new FieldPosition(0));
      buf.append(" ");
      sdf = new SimpleDateFormat("HH:mm");
      sdf.format((Date)oneSpinRow.time.getValue(),buf,new FieldPosition(0));
      buf.append(" ");
      buf.append(oneSpinRow.loginRate.getValue().toString());
      buf.append(" ");
      buf.append(oneSpinRow.hrRate.getValue().toString());
//      buf.append(" ");
//      buf.append(oneSpinRow.goRate.getValue().toString());
//      buf.append(" ");
//      buf.append(oneSpinRow.gohrRate.getValue().toString());
      bufOut.write(buf.toString());
      bufOut.newLine();
    }
  }

  public void setRates(AccountInfo2 ai) {
    int n = ai.rateCount();
    RateEntry re;
    spinRows.clear();
    for (int i=0; i<n; i++) {
      re = ai.rates(i);
      oneSpinRow = new OneSpinRow();
      oneSpinRow.day  = new DaySpinner("EEE",re.day());
      oneSpinRow.time = new DaySpinner("HH:mm",re.time());
      oneSpinRow.loginRate = new RateSpinner( new Float(re.login) );
      oneSpinRow.hrRate = new RateSpinner( new Float(re.loginhr) );
//      oneSpinRow.goRate = new RateSpinner( new Float(re.go) );
//      oneSpinRow.gohrRate = new RateSpinner( new Float(re.gohr) );
      spinRows.add(oneSpinRow);
    }
    if (n==0) {
      oneSpinRow = new OneSpinRow();
      oneSpinRow.day  = new DaySpinner("EEE",new String("Mon"));
      oneSpinRow.time = new DaySpinner("HH:mm",new String("12:00"));
      oneSpinRow.loginRate = new RateSpinner( new Float(1.00) );
      oneSpinRow.hrRate = new RateSpinner( new Float(1.00) );
//      oneSpinRow.goRate = new RateSpinner( new Float(1.00) );
//      oneSpinRow.gohrRate = new RateSpinner( new Float(1.00) );
      spinRows.add(oneSpinRow);
    }
    // mtm.fireTableDataChanged();
    mtm.fireTableStructureChanged();
  }

  public void deleteRow() {
    int sri = getSelectedRow();
    int sci = getSelectedColumn();
    int mri = mtm.getRowCount()-1;
    if ( (sri == -1) || (mri < 1))
        return;
    getCellEditor(sri,sci).cancelCellEditing();
    spinRows.remove(sri);
    // mtm.fireTableRowsDeleted(sri,sri);
    mtm.fireTableStructureChanged();
  }

  public void insertRow() {
    OneSpinRow tmpRow;
    int sri = getSelectedRow();
    int sci = getSelectedColumn();
    int mri = spinRows.size();
    if (sri == -1) {
      sri = mri-1;
      sci = 0;
    }
    tmpRow = new OneSpinRow();
    tmpRow.day  = new DaySpinner("EEE","Mon");
    tmpRow.day.setValue(((DaySpinner)mtm.getValueAt(sri,0)).getValue());
    Date s = (Date)((Date)((DaySpinner)mtm.getValueAt(sri,0)).getValue());
    tmpRow.time = new DaySpinner("HH:mm","00:00");
    tmpRow.time.setValue(((DaySpinner)mtm.getValueAt(sri,1)).getValue());
    tmpRow.loginRate = new RateSpinner(new Float(1.00));
    tmpRow.hrRate = new RateSpinner(new Float(1.00));
    tmpRow.goRate = new RateSpinner(new Float(1.00));
    tmpRow.gohrRate = new RateSpinner(new Float(1.00));
    spinRows.add(sri+1,tmpRow);

    mtm.fireTableRowsInserted(sri+1,sri+1);
  }

  class MyTableModel extends AbstractTableModel {
    String[] columnNames;

    public MyTableModel() {
        oneSpinRow = new OneSpinRow();
        oneSpinRow.day  = new DaySpinner("EEE","Mon");
        oneSpinRow.time = new DaySpinner("HH:mm","00:00");
        oneSpinRow.loginRate = new RateSpinner( new Float(1.00));
        oneSpinRow.hrRate = new RateSpinner( new Float(1.00));
        oneSpinRow.goRate = new RateSpinner( new Float(1.00));
        oneSpinRow.gohrRate = new RateSpinner( new Float(1.00));
        spinRows.add(oneSpinRow);
    };

    public int getColumnCount() {
      AProps aProps = AProps.getInstance();
      columnNames = aProps.getTableHeaders();
      return columnNames.length; 
    }
    public int getRowCount() {
      return spinRows.size();
    }
    public String getColumnName(int col) {
      AProps aProps = AProps.getInstance();
      columnNames = aProps.getTableHeaders();
      return columnNames[col];
    }
    public Class getColumnClass(int col) {
      Class tmpClass = null;
      switch(col) {
        case 0:
        case 1:  tmpClass = DaySpinner.class;   break;
        case 2:  tmpClass = RateSpinner.class;  break;
        case 3:  tmpClass = RateSpinner.class;  break;
        case 4:  tmpClass = RateSpinner.class;  break;
        case 5:  tmpClass = RateSpinner.class;  break;
      }
      return (tmpClass);
    }

    public Object getValueAt(int row, int col) {
      JSpinner tmpSpinner = null;
      if (row >= spinRows.size())
          return null;
      if (col >= columnNames.length)
          return null;

//      System.out.println("getValueAt: row="+row+" col="+col);
      OneSpinRow oneSpinRow = (OneSpinRow)spinRows.get(row);
      switch (col) {
      case 0:  tmpSpinner = oneSpinRow.day;        break;
      case 1:  tmpSpinner = oneSpinRow.time;       break;
      case 2:  tmpSpinner = oneSpinRow.loginRate;  break;
      case 3:  tmpSpinner = oneSpinRow.hrRate;     break;
      case 4:  tmpSpinner = oneSpinRow.goRate;     break;
      case 5:  tmpSpinner = oneSpinRow.gohrRate;   break;
      }
      return tmpSpinner;
    }
    public boolean isCellEditable(int row, int col) {
      return true;
    }
    public void setValueAt(Object value, int row, int col) {
      if (row >= spinRows.size())
          return;
      if (col >= columnNames.length)
          return;
      OneSpinRow oneSpinRow = (OneSpinRow)spinRows.get(row);
      switch (col) {
      case 0:  oneSpinRow.day = (DaySpinner)value;        break;
      case 1:  oneSpinRow.time = (DaySpinner)value;       break;
      case 2:  oneSpinRow.loginRate = (RateSpinner)value; break; 
      case 3:  oneSpinRow.hrRate = (RateSpinner)value;    break;
      case 4:  oneSpinRow.goRate = (RateSpinner)value;    break;
      case 5: oneSpinRow.gohrRate = (RateSpinner)value;   break;
      }
      mtm.fireTableCellUpdated(row,col);
    }
  }
 
  class OneSpinRow {
    DaySpinner day;
    DaySpinner time;
    RateSpinner loginRate;
    RateSpinner hrRate;
    RateSpinner goRate;
    RateSpinner gohrRate;
  }
}
