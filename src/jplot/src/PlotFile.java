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

import javax.swing.*;
import javax.swing.event.*;
import java.awt.*; 
import java.io.*; 
import java.awt.event.*; 

public class PlotFile extends JFrame
   implements TextListener, ItemListener, ActionListener 
{
   private DefaultListModel listModel;
   private boolean bChangeDef = false;

   public PlotFile(PlotConfig.PlPanel parent)
   {
	p_object = parent;
	ft = new Font("Monospaced", Font.BOLD, 16);
	wft = new Font("SansSerif", Font.BOLD, 18);
	setTitle("Plot Templates");
	// fList = new JList(8, false);
	listModel = new DefaultListModel();
	fList = new JList(listModel);
	fList.setFont(ft);
	setLocation(100, 30);

        js = new JScrollPane(fList,
                            JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                            JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
        js.setAlignmentX(LEFT_ALIGNMENT);

	addFiles(getUserDir());
	addFiles(getSysDir());
	defName = getDefTemplate();
/*
	ItemListener listChanged = new ItemListener()
        {  
	    public void itemStateChanged(ItemEvent evt)
            { listChange(); }
        };
	fList.addItemListener(listChanged);
*/
	pan = new JPanel();
	pan.setLayout(new GridLayout(4, 1));
	pan.setFont(ft);
	fpan = new JPanel();
	fpan.setLayout(new FlowLayout(FlowLayout.LEFT));
	fpan.add(new JLabel("Template: "));
	toFile = new JTextField("", 32);
	toFile.setFont(ft);
	fpan.add(toFile);
	pan.add(fpan);

	defpan = new JPanel();
	defpan.setFont(ft);
	defpan.setLayout(new FlowLayout(FlowLayout.LEFT));
	defCheck = new JCheckBox("Use this template as default");
	ItemListener defChanged = new ItemListener()
        {  
	    public void itemStateChanged(ItemEvent evt)
            { checkDef(); }
        };
	defCheck.addItemListener(defChanged);
	defpan.add(defCheck);
	pan.add(defpan, "LEFT");

	defMess = new JLabel("   ", JLabel.LEFT);
	pan.add(defMess);

	if (defName != null)
	{
	    toFile.setText(defName);
	    // defCheck.setState(true);
	    defCheck.setSelected(true);
	    defMess.setText("  default template is "+defName);
	}
	else
	    defMess.setText("  default template is null");
	butpan = new JPanel();
	butpan.setFont(ft);
	addButton("Open", this);
	addButton("Save", this);
	addButton("Remove", this);
	addButton("Close", this);

	pan.add(butpan);

	Container cont = getContentPane();
	cont.add(js, "Center");
	cont.add(pan, "South");
	
	pack();	
	addWindowListener(new WindowAdapter() { public void
            windowClosing(WindowEvent e) { setVisible(false); } } );
	// toFile.addTextListener(this);
	toFile.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent ev) {
                textChange();
            }
        });

	fList.addListSelectionListener(new ListSelectionListener() {
            public void valueChanged(ListSelectionEvent e) {
                listChange();
            }
        });
   }

   public void addButton(String label, Object obj)
   {
	JButton but = new JButton(label);
	but.addActionListener((ActionListener) obj);
	butpan.add(but);
   }

   public void setTopObj(PlotConfig.PlPanel parent)
   {
	p_object = parent;
   }

   private void updateDefMark(String name)
   {
	if ((defName == null) || (defName.length() < 1))
	{
/*
		defName = name;
		defCheck.setState(true);
*/
		defCheck.setSelected(false);
	        defMess.setText("  default template is null");
	}
	else
	{
		if (!defName.equals(name))
		   defCheck.setSelected(false);
		else
		   defCheck.setSelected(true);
	}
   }


   public void textChange()
   {
	boolean set = false;

/*
	if ((defName == null) || (defName.length() < 1))
	    return; 
*/
	bChangeDef = true;
	String f = toFile.getText().trim();
	if (f.length() > 0)
	{
	   if (f.equals(defName))
		set = true;
	}
	defCheck.setSelected(set);
	bChangeDef = false;
   }

   public void textValueChanged(TextEvent e)
   {
   }

   private String getUserDir() {
	 return System.getProperty("userdir")+"/templates/plot/";
   }

   private String getSysDir() {
	 return System.getProperty("sysdir")+"/user_templates/plot/";
   }

   public String getTemplateName() {
	String d = toFile.getText().trim();
	if (d.length() > 0)
	    return d;
	else
	    return null;
   }

   public void listChange()
   {
	String newTemp = (String)fList.getSelectedValue();
	bChangeDef = true;
	if (newTemp != null)
	{
	    toFile.setText(newTemp);
	    updateDefMark(newTemp);
	}
	bChangeDef = false;
   }

   public String getDefaultTemplate()
   {
	return defName;
   }

   public void checkDef()
   {
	if (bChangeDef)
	    return;
	if (defCheck.isSelected())
	{
	    String ndef = toFile.getText().trim();
	    if (ndef.length() > 0)
	        defName = ndef;
	    else
	        defName = "";
	}
	else
	    defName = "";
	if (defName.length() < 1)
	    defMess.setText("  default template is null");
	else
	    defMess.setText("  default template is "+defName);
	if (p_object != null) {
	    p_object.setDefaultName(defName);
	}
   }

   public void itemStateChanged(ItemEvent  evt)
   {
	String newTemp = (String) fList.getSelectedValue();
	if (newTemp != null)
	    toFile.setText(newTemp);
   }

   public void setItem(String name)
   {
	int	n, k;
	String  ftext;
	if ((name == null) || (name.length() <= 0))
	   return;
	n = listModel.getSize();
	k = 0;
	while (k < n)
	{
	   ftext = (String)listModel.elementAt(k);
	   if (ftext.equals(name))
	   {
		// fList.makeVisible(k);
	        fList.ensureIndexIsVisible(k);
		break;
	   }
	   k++;
	}
	toFile.setText(name);
	if (defName != null)
	{
	   bChangeDef = true;
	   if (!name.equals(defName))
		defCheck.setSelected(false);
	   else
		defCheck.setSelected(true);
	   bChangeDef = false;
	}
	updateDefMark(name);
   }

   private void openNewTemplate(String name) {
	String file = getUserDir()+name;
	File fd = new File(file);
	String mess;
      	if (!fd.exists() || !fd.canRead()) {
	   file = getSysDir()+name;
	   fd = new File(file);
	}
      	if (!fd.exists() || !fd.canRead()) {
	   if (!fd.exists())
		mess = " Template "+"'"+name+"'"+" does not exist. ";
	   else
		mess = " Template "+"'"+name+"'"+" permission denied! ";
	   JOptionPane.showMessageDialog(this, mess);
	   return;
	}
	p_object.openTemplate(file);
   }


   public void actionPerformed(ActionEvent  evt)
   {
	String ftext = null;
	String mess;
	File fd;
	String cmd = evt.getActionCommand();
	if (cmd.equals("Open"))
	{
	    int	   n;
	    ftext = toFile.getText().trim();
	    if ((ftext == null) || (ftext.length() <= 0))
		return;
	    openNewTemplate(ftext);
	    return;
	}
	if (cmd.equals("Save"))
	{
	    int	 n, k, exist;
	    String ltext;
	    ftext = toFile.getText().trim();
	    if ((ftext == null) || (ftext.length() <= 0))
		return;
	    // n = fList.getItemCount();
	    n = listModel.getSize();
	    k = 0;
	    exist = 0;
	    while (k < n)
	    {
		// ltext = fList.getItem(k);
	        ltext = (String)listModel.elementAt(k);
		if (ftext.equals(ltext))
		{
		    exist = 1;
	            // fList.makeVisible(k);
	            fList.ensureIndexIsVisible(k);
		    break;
		}
		k++;
	    }
	    if (exist == 0)
	    {
/*
	        fList.add(ftext);
	        fList.makeVisible(n);
	        fList.select(n);
*/
		listModel.addElement(ftext);
		fList.setSelectedIndex(n);
	        fList.ensureIndexIsVisible(n);
	    }
	    mess = getUserDir()+ftext;
	    fd = new File(mess);
      	    if (fd.exists())
	    {
		mess = "Template "+"'"+ftext+"'"+" will be overwritten!";
                butAnswer = JOptionPane.showConfirmDialog(this,
                    mess, "Save Template",
                    JOptionPane.OK_CANCEL_OPTION);
		if (butAnswer != 0)
		   return;
	    }
	    p_object.saveTemplate(ftext, false);
	    return;
	}
	if (cmd.equals("Close"))
	{
	    this.setVisible(false);
	    return;
	}
	if (cmd.equals("Remove"))
	{
	    int	 n, k;
	    String ltext;

	    ftext = toFile.getText().trim();
	    if ((ftext == null) || (ftext.length() <= 0))
		return;
	    // n = fList.getItemCount();
	    n = listModel.getSize();
	    k = 0;
	    while (k < n)
	    {
		// ltext = fList.getItem(k);
	        ltext = (String)listModel.elementAt(k);
		if (ftext.equals(ltext))
		    break;
		k++;
	    }
	    mess = getUserDir()+ftext;
	    fd = new File(mess);
      	    if (!fd.exists()) {
	        mess = getSysDir()+ftext;
	        fd = new File(mess);
	    }
      	    if (!fd.exists() || !fd.canWrite()) {
		if (!fd.exists())
		   mess = " Template "+"'"+ftext+"'"+" does not exist! ";
		else
		   mess = " Template "+"'"+ftext+"'"+" permission denied! ";
		JOptionPane.showMessageDialog(this, mess);
                return;
	    }
	    else
	    {
		mess = "Template "+"'"+ftext+"'"+" will be removed!";
                butAnswer = JOptionPane.showConfirmDialog(this,
                    mess, "Remove Template",
                    JOptionPane.OK_CANCEL_OPTION);
		if (butAnswer != 0)
		   return;
		fd.delete();
	    	n = listModel.getSize();
		if (k < n)
	    	    listModel.removeElementAt(k);
	    	toFile.setText("");
		if (ftext.equals(defName)) {
		    defName = "";
		    defCheck.setSelected(false);
	    	    // updateDefMark(" ");
		}
	    }
	    return;
	}
   }

   private class oneButtonDialog extends Dialog
	implements  ActionListener
   {
        public oneButtonDialog (Frame parent)
	{
	    super(parent, "Warning!", true);
      	    p1 = new JPanel();
	    p1.setFont(wft);
	    l1 = new JLabel("Label 1");
	    p1.add(l1);
	    add(p1, "Center");

      	    p2 = new JPanel();
	    p2.setFont(ft);
	    b1 = new JButton("Ok");
	    b1.addActionListener(this);
	    p2.add(b1);
            add(p2, "South");
	    pack();
	    setLocation(200, 80);
	    addWindowListener(new WindowAdapter() { public void
               windowClosing(WindowEvent e) { setVisible(false); } } );
	}
        public void message (String text)
	{
	    l1.setText(text);
	    pack();
	}

   	public void actionPerformed(ActionEvent evt)
   	{  String arg = evt.getActionCommand();
      	   if(arg.equals("Ok"))
         	setVisible(false);
        }

      	JPanel p1;
      	JPanel p2;
	private JLabel  l1;
	private JButton b1;
   }

   private class twoButtonDialog extends Dialog
	implements  ActionListener
   {
        public twoButtonDialog (Frame parent)
	{
	    super(parent, "Warning!", true);
      	    p1 = new JPanel();
	    p1.setFont(wft);
	    l1 = new JLabel("Label 1");
	    p1.add(l1);
	    add(p1, "Center");

      	    p2 = new JPanel();
	    p2.setFont(ft);
	    b1 = new JButton("Continue");
	    b1.addActionListener(this);
	    b2 = new JButton("Cancel");
	    b2.addActionListener(this);
	    p2.add(b1);
	    p2.add(b2);
            add(p2, "South");
	    pack();
	    setLocation(200, 80);
	    addWindowListener(new WindowAdapter() { public void
               windowClosing(WindowEvent e) { setVisible(false); } } );
	}
        public void message (String text)
	{
	    l1.setText(text);
	    pack();
	}

   	public void actionPerformed(ActionEvent evt)
   	{  String arg = evt.getActionCommand();
      	   if(arg.equals("Continue"))
		butAnswer = 1;
	   else
		butAnswer = 0;
           setVisible(false);
        }

      	JPanel p1;
      	JPanel p2;
	private JLabel  l1;
	private JButton b1;
	private JButton b2;
   }

   private void addToFileList(String name) {
	// int n = fList.getItemCount();
	int n = listModel.getSize();
	for (int k = 0; k < n; k++)
	{
	   String ftext = (String)listModel.elementAt(k);
	   if (ftext.equals(name))
		return;
	}
	listModel.addElement(name);
	// fList.add(name);
   }

   private void addFiles(String path) {
	File dir = new File(path);
	if (!dir.exists())
	    return;
	if (!dir.isDirectory())
	    return;
	File  files[] = dir.listFiles();
	for (int i = 0; i < files.length; i++) {
	    if (files[i].isFile()) {
		String name = files[i].getName();
		if (!name.startsWith(".")) {
		    if (!name.equals("menu")) {
			addToFileList(name);
		    }
		}
	    }
	}
   }



   public String getDefTemplate() {
	String file = getUserDir()+".default";
	File fd = new File(file);
        if (!fd.exists() || !fd.canRead()) {
          file = getSysDir()+".default";
          fd = new File(file);
        }
        if (!fd.exists() || !fd.canRead()) {
	    return null;
	}
	BufferedReader in = null;
	try  {
           in = new BufferedReader(new FileReader(file));
           }
        catch(IOException e) { return null; }
	if (in == null)
	    return null;
	String data = null;
	try  {
           while ((data = in.readLine()) != null)
           {
                data = data.trim();
		if (data.length() > 0)
		   break;
	   }
	   in.close();
	}
	catch(IOException e) { 
	   return null;
	}
	return data;
   }

   private JList fList;
   private JScrollPane js;
   private JPanel pan;
   private JPanel lpan;
   private JPanel defpan;
   private JPanel fpan;
   private JPanel butpan;
   private JCheckBox defCheck;
   private String fname;
   private String defName = null;
   private JTextField toFile;
   private JLabel defMess;
   private Font  ft;
   private Font  wft;
   private int   butAnswer;
   private Point px;
   private PlotConfig.PlPanel p_object;
   private oneButtonDialog d_one = null;
   private twoButtonDialog d_two = null;
}


