/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * 
 *
 */
/*
 * UserChooser.java
 *
 * Created on March 14, 2006, 5:54 PM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package accounting;

import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.util.*;
import javax.swing.*;
import javax.swing.event.*;

/**
 *
 * @author frits
 */
public class UserChooser extends JList implements MouseListener {
    
    ArrayList<String> oAndOs,checkedOAndOs;
    DefaultListModel ulm;
    JList thisList;
    Font f = new Font("Serif",Font.PLAIN,12);
    static UserChooser thisInstance;
    
    /**
     * Creates a new instance of UserChooser
     */
    private UserChooser() {
        thisList = this;
        checkedOAndOs = new ArrayList();
        ulm = new DefaultListModel();
        getAllOwnersAndOperators();
        for (int i=0; i<oAndOs.size(); i++) {
            ulm.addElement( new CellInfo((String)(oAndOs.get(i)),false) );
        }
        
        setModel(ulm);
        
        addMouseListener(this);
        setCellRenderer( new MyCellRenderer() );
    }
    
    public static UserChooser getInstance() {
        if (thisInstance == null) {
            thisInstance = new UserChooser();
        }
        return (thisInstance);
    }
    
    public void mouseClicked(MouseEvent e){
        Point p = e.getPoint();
        int index = thisList.locationToIndex(p);
        CellInfo ci = (CellInfo)ulm.getElementAt(index);
        if (ci.e){
            ci.e = false;
            checkedOAndOs.remove(ci.s);
        }else{
            ci.e = true;
            checkedOAndOs.add(ci.s);
        }
        thisList.repaint();
    }
    public void mouseEntered(MouseEvent e){}
    public void mouseExited(MouseEvent e){}
    public void mousePressed(MouseEvent e){}
    public void mouseReleased(MouseEvent e){}
    
    private void getAllOwnersAndOperators() {
        int iTmp;
        String[] accounts;
        String   oneLine, owner, operator;
        StringTokenizer st;
        BufferedReader br = null;
        oAndOs = new ArrayList();
//        System.out.println(AProps.getInstance().getRootDir());
        File userDir = new File(AProps.getInstance().getRootDir()+"/adm/users/profiles/user");
        if ( ! userDir.exists() ) {
            System.out.println("OandO: user directory "+userDir+" does not exist");
        }
        accounts = userDir.list();
        AProps aProps = AProps.getInstance();
        String billMode = aProps.getBillingMode();
        for (int i=0; i<accounts.length; i++) {
            String strtmp;
            // System.out.println("OandO Opening: '"+accounts[i]+"'");
            iTmp = accounts[i].length();
            if ( (iTmp > 4) && accounts[i].substring(iTmp-4,iTmp).equals(".txt") ) {
                owner = accounts[i].substring(0,iTmp-4);
            }
            else
                owner = accounts[i];
            oAndOs.add(owner);
            // System.out.println("OandO Opening: '"+accounts[i]+"'");
            if ( billMode.equalsIgnoreCase("goes") || billMode.equalsIgnoreCase("login")) {
                try {
                    br = new BufferedReader(new FileReader(AProps.getInstance().getRootDir()+"/adm/users/profiles/user/"+accounts[i]));
                    oneLine = new String("");  // seed
                    while ( (oneLine != null) && ! oneLine.startsWith("operators") ) 
                        oneLine = br.readLine();
                    if ( (oneLine != null) && oneLine.startsWith("operators") ) {
                        st = new StringTokenizer(oneLine,"\t \r\n\"");
                        st.nextToken();   // skip 'operators'
                        int n = st.countTokens();
                        for (int j=0; j<n; j++) {
                            strtmp = st.nextToken();
                            oAndOs.add(strtmp);
                        }
                    }
                    br.close();
                }
                catch (Exception e) 
                {
                    System.out.println("getAllOwnersAndOperators: IO Exception");
                    e.printStackTrace();
                }                
            }
        }
        //sort the list alphabetically
        Collections.sort(oAndOs);
        int n = oAndOs.size();
        if (n<2) return; // no need to check on one
        int j=0;
        String str1,str2;
        for (int i=0; i<(n-1); i++) {
            str1 = oAndOs.get(j);
            str2 = oAndOs.get(j+1);
            if (str1.compareTo(str2) == 0) {
                oAndOs.remove(j+1);
            }
            else
                j++;            
        }
    }
    
    final static String UserComment = new String("# Owners/operators list charging to this account");
    public void listCheckedOAndOs(BufferedWriter bufOut) throws IOException {
        bufOut.write(UserComment);  bufOut.newLine();
        bufOut.write("operators ");
       if (checkedOAndOs.size() > 0) {
        for (int i=0; i<checkedOAndOs.size();i++) {
            bufOut.write(checkedOAndOs.get(i)+" ");
        }
      }
        bufOut.newLine();
    }
    
    public void setCheckedOAndOs(AccountInfo2 ai) {
        CellInfo ci;
        String oOrO;
        // clear existing ones
        for (int j=0; j<ulm.getSize(); j++) {
            ci = (CellInfo)ulm.getElementAt(j);
            ci.e = false;
        }
        checkedOAndOs.clear();
        StringTokenizer st = new StringTokenizer(ai.oAndOs());
        st.nextToken(); // skip 'operators '
        int n = st.countTokens(); // remaining tokens!
        for (int i=0; i<n; i++) {
            oOrO = st.nextToken();
            for (int j=0; j<ulm.getSize(); j++) {
                ci = (CellInfo)ulm.getElementAt(j);
                if ( oOrO.compareTo(ci.s) == 0 ) {
                    ci.e = true;
                    checkedOAndOs.add(ci.s);
                }
            }
        }
        repaint();
    }
    
    public ArrayList getCheckedOAndOs() {
        return checkedOAndOs;
    }
    
    class MyCellRenderer extends JCheckBox implements ListCellRenderer {
        // This is the only method defined by ListCellRenderer.
        // We just reconfigure the JLabel each time we're called.

        public Component getListCellRendererComponent(
               JList list,
               Object value,            // value to display
               int index,               // cell index
               boolean isSelected,      // is the cell selected
               boolean cellHasFocus)    // the list and the cell have the focus
        {
          CellInfo ci;
         // if (list.getValueIsAdjusting()) {
         //     System.out.println("Cell Render: adjusting");
         //     return(this);
          //}
          ci = (CellInfo)value;
          setText(ci.s);
          setSelected(ci.e);
       //   System.out.println("Cell Render "+jcb.getText()+" is "+jcb.isSelected());
          setEnabled(list.isEnabled());
          //setFont(list.getFont());
          setFont(f);
          setOpaque(true);
          return this;
        }
    }
    
    public class CellInfo {
        String s;
        boolean e;
        public CellInfo(String str, boolean b) {
            s = str;
            e = b;
        }
    }
}
