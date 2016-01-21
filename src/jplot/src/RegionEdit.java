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

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
// import PlotEditButton.*;
// import PlotConfig.*;

public class RegionEdit extends JFrame
   implements ActionListener
{
   public RegionEdit(PlotConfig.PlPanel parent, String  data)
   {
      setTitle("Region Editor");
      Font f = new Font("Dialog", Font.BOLD, 14);
      Font ft = new Font("Monospaced", Font.BOLD, 18);
      cont = getContentPane();
      JPanel p = new JPanel();
      p.setFont(f);
      Button restoreButton = new Button("Restore");
      Button clearButton = new Button("Delete");
      Button clearAllButton = new Button("Delete All");
      Button copyButton = new Button("Copy");
      Button pasteButton = new Button("Paste");
      Button closeButton = new Button("Close");

      p_object = parent;
      restoreButton.addActionListener((ActionListener)this);
      p.add(restoreButton);
      clearButton.addActionListener((ActionListener)this);
      p.add(clearButton);
      clearAllButton.addActionListener((ActionListener)this);
      p.add(clearAllButton);
      copyButton.addActionListener((ActionListener)this);
      p.add(copyButton);
      pasteButton.addActionListener((ActionListener)this);
      p.add(pasteButton);
      closeButton.addActionListener((ActionListener)this);
      p.add(closeButton);
      cont.add(p, "South");
      ta = new JTextArea(data, 12, 50);
      ta.setFont(ft);
      ta.setLineWrap(true);
      cont.add(ta, "Center");
      addWindowListener(new WindowAdapter() { public void
            windowClosing(WindowEvent e) { 
	p_object.saveEditData();
	setVisible(false); } } );

      pack();
   }

   public void actionPerformed(ActionEvent evt)
   {
      int    pstart, pend;
      String data;
      String arg = evt.getActionCommand();
      if(arg.equals("Delete"))
      {
	pstart = ta.getSelectionStart();
	pend = ta.getSelectionEnd();
	if (pend > pstart)
	    ta.replaceRange("", pstart, pend);
	return;
      }
      if(arg.equals("Delete All"))
      {
	 ta.replaceRange("", 0, ta.getText().length());
	 return;
      }
      if(arg.equals("Copy"))
      {
	 copyStr = ta.getSelectedText();
	 return;
      }
      if(arg.equals("Paste"))
      {
	 if (copyStr == null || copyStr.length() <= 0)
	    return;
	 pstart = ta.getCaretPosition();
	 ta.insert(copyStr, pstart);
      }
      else if(arg.equals("Close"))
      {
	p_object.saveEditData();
        setVisible(false);
      }
      else if(arg.equals("Restore"))
      {
	p_object.restoreEditData();
      }
   }

   public String getData()
   {
	return (ta.getText());
   }

   public void setData(String  data)
   {
	if (data == null)
	   ta.replaceRange("", 0, ta.getText().length());
	else
	   ta.replaceRange(data, 0, ta.getText().length());
   }

   private JTextArea ta;
   private PlotConfig.PlPanel p_object;
   private String  copyStr = null;
   private Container cont;
}


