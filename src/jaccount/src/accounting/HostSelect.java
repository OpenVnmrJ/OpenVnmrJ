/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * HostSelect.java
 *
 * Created on April 18, 2005, 6:34 AM
 */
/**
 *
 * @author frits
 */

package accounting;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.text.*;

public class HostSelect extends JPanel {
    
  String host = null;
  JButton localHost, remoteHost;
  JTextField hostField;
  JLabel hostLabel;

    
  /** Creates a new instance of HostSelect */
  public HostSelect() {
    setLayout( new HostSelectLayout());
    setSize(400,50);
    localHost = new JButton("Local Host");
    localHost.addActionListener( new LocalHostEar() );
    Font f = localHost.getFont();
    String fName = f.getName();
    int style = f.getStyle();
    int size = f.getSize();
//     System.out.println("Font='"+f+"'style="+style+" size="+size);
    if (size>16) size=14;
    else if (size>12) size=12;
    else size=10;
    f = new Font(fName, style,size);
    localHost.setFont(f);
    localHost.setEnabled(false);
    add(localHost);
    remoteHost = new JButton("Remote Host");
    remoteHost.addActionListener( new RemoteHostEar() );
    remoteHost.setFont(f);
    add(remoteHost);
    hostField = new JTextField(10);
    hostField.setEnabled(false);
    hostField.setFont(f);
    hostField.getDocument().addDocumentListener( new HostFieldEar() );
    add(hostField);
    hostLabel = new JLabel("Localhost");
    hostLabel.setFont(f);
    add(hostLabel);
  }
  
  private class HostSelectLayout implements LayoutManager {
    public void addLayoutComponent(String str, Component c) {
    }

    public void layoutContainer(Container parent) {
        int maxX=parent.getWidth();
        localHost.setBounds(10,5,100,20);
        remoteHost.setBounds(150,5,100,20);
        hostField.setBounds(260,5,maxX-270,20);
        hostLabel.setBounds(10,30,200,20);
    }
    public Dimension minimumLayoutSize( Container parent ) {
      return ( new Dimension(0,0));
    }
    public Dimension preferredLayoutSize( Container parent ) {
      return ( new Dimension(parent.getWidth(),parent.getHeight()));
    }
    public void removeLayoutComponent( Component c  ) {
    }
  }
    
  private class LocalHostEar implements ActionListener {
    public void actionPerformed(ActionEvent ae) {
      hostLabel.setText("Localhost");
      localHost.setEnabled(false);
      remoteHost.setEnabled(true);
      hostField.setEnabled(false);
    }
  }
  private class RemoteHostEar implements ActionListener {
    public void actionPerformed(ActionEvent ae) {
      hostLabel.setText("Host: "+hostField.getText());
      localHost.setEnabled(true);
      remoteHost.setEnabled(false);
      hostField.setEnabled(true);
      hostField.selectAll();
    }
  }
  private class HostFieldEar implements DocumentListener {
      public void changedUpdate(DocumentEvent de) {
          System.out.println("got change");
      }
      public void insertUpdate(DocumentEvent de) {
          System.out.println("got insert");
          hostLabel.setText("Host: "+hostField.getText());
      }
      public void removeUpdate(DocumentEvent de) {
          System.out.println("got remove");
          hostLabel.setText("Host: "+hostField.getText());
      }
  }
}
