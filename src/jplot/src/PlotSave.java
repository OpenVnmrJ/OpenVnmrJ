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
import java.util.*; 
import java.awt.event.*; 
// import PlotConfig.*; 

public class PlotSave extends JFrame
   implements ItemListener, ActionListener 
{
   private DefaultListModel dirListModel;
   private DefaultListModel fileListModel;
   private String userDir;
   private String outDir;
   private String outFile;
   private String outDpi = "72";
   
   public PlotSave(PlotConfig.PlPanel parent, String usrdir)
   {
	p_object = parent;

	ft = new Font("Monospaced", Font.BOLD, 16);
	JPanel tpan = new JPanel();
	tpan.setFont(ft);
	tpan.setLayout(new GridLayout(1, 2));
	tpan.add(new JLabel("Directory:"));
	tpan.add(new JLabel("File:"));
        outDir = usrdir;
        userDir = usrdir;
        outFile = "";
        readOutputFormat();


	pan1 = new JPanel();
	setTitle("Plot Save");
	JPanel scr1 = new JPanel();
	JPanel scr2 = new JPanel();

	scr1.setLayout(new GridLayout(1, 1));
	scr2.setLayout(new GridLayout(1, 1));
	dirListModel = new DefaultListModel();
	dirList = new JList(dirListModel);
	JScrollPane js1 = new JScrollPane(dirList,
                            JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                            JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
     
	fileListModel = new DefaultListModel();
	fileList = new JList(fileListModel);
	JScrollPane js2 = new JScrollPane(fileList,
                            JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                            JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
     
	dirList.setFont(ft);
	fileList.setFont(ft);
	setLocation(100, 30);

/*
	ItemListener listChanged = new ItemListener()
        {
            public void itemStateChanged(ItemEvent evt)
            { updateList(); }
        };
	dirList.addItemListener(listChanged);
	ItemListener fileChanged = new ItemListener()
        {
            public void itemStateChanged(ItemEvent evt)
            { updateFile(); }
        };
	fileList.addItemListener(fileChanged);
*/
	dirList.addListSelectionListener(new ListSelectionListener() {
            public void valueChanged(ListSelectionEvent e) {
                updateDir();
            }
        });

	fileList.addListSelectionListener(new ListSelectionListener() {
            public void valueChanged(ListSelectionEvent e) {
                updateFile();
            }
        });

	scr1.add(js1);
	scr2.add(js2);

	pan1.setLayout(new GridLayout(1, 2));
	pan1.add(scr1);
	pan1.add(scr2);

	pan = new JPanel();
	pan.setLayout(new GridLayout(0, 1, 0, 10));
	pan.setFont(ft);

	JPanel formatpan = new JPanel();
	formatpan.setLayout(new FlowLayout(FlowLayout.LEFT));
	formatpan.add(new JLabel("Data Format:"));
	formatChoice = new JComboBox();
	build_format_choice();
	formatpan.add(formatChoice);
	formatpan.add(new JLabel(" Data Resolution(dpi):"));

	formatChoice.addActionListener(new ActionListener()
         {
            public void actionPerformed(ActionEvent e)
            {  updateFormat(e);  }
         });

	resText = new JTextField(outDpi, 6);
	resText.setFont(ft);
	formatpan.add(resText);
	pan.add(formatpan);

	JPanel dirpan = new JPanel();
	// dirpan.setLayout(new FlowLayout(FlowLayout.LEFT));
	// dirpan.add(new JLabel("Directory:"));
	dirpan.setLayout(new BorderLayout());
	dirpan.add(new JLabel("Directory:"), BorderLayout.WEST);
	dirText = new JTextField(outDir, 40);
	dirText.setFont(ft);
	dirpan.add(dirText, BorderLayout.CENTER);
	pan.add(dirpan);

	JPanel fpan = new JPanel();
	// fpan.setLayout(new FlowLayout(FlowLayout.TRAILING));
	// fpan.add(new JLabel("        File:"));
	fpan.setLayout(new BorderLayout());
	fpan.add(new JLabel("        File:"), BorderLayout.WEST);
	fileText = new JTextField(outFile, 40);
	fileText.setFont(ft);
	fpan.add(fileText, BorderLayout.CENTER);
	pan.add(fpan);

	dirText.addActionListener(new ActionListener()
         {  public void actionPerformed(ActionEvent evt)
            { updateText(evt); }
         });

	butpan = new JPanel();
	butpan.setFont(ft);
	addButton("Save", this);
	addButton("Close", this);

	pan.add(butpan);

	Container cont = getContentPane();
	cont.add(tpan, "North");
	cont.add(pan1, "Center");
	cont.add(pan, "South");
	
	changeDir();
	pack();	
	addWindowListener(new WindowAdapter() {
              public void windowClosing(WindowEvent e) {
                 writeOutputFormat();
                 // setVisible(false);
              }
         } );
   }

   public void changeDir()
   {
	File  f;
	int   k;
	String abspath, path;

	if (outDir == null)
	    outDir = "./";
	f = new File(outDir);
	if (!f.isAbsolute())
	{
	    outDir = f.getAbsolutePath();
	    f = new File(outDir);
	}
	String[] dirs = f.list();
	dirListModel.removeAllElements();
	fileListModel.removeAllElements();
	path = f.getPath();
	parentDir = f.getParent();
	abspath = f.getAbsolutePath();
	outDir = f.getPath();
	if (!outDir.equals("/"))
	    dirListModel.addElement("../");
	if (dirs == null)
	{
	    return;
	}
	for (k = 0; k < dirs.length; k++)
	{
	    File d = new File(outDir + File.separator + dirs[k]);
	    if (d.isDirectory())
	        dirListModel.addElement(dirs[k]+"/");
	    else
	        fileListModel.addElement(dirs[k]);
	}
	dirText.setText(path);
   }

   public void addButton(String label, Object obj)
   {
	JButton but = new JButton(label);
	but.addActionListener((ActionListener) obj);
	butpan.add(but);
   }

   public void itemStateChanged(ItemEvent  evt)
   {
        String newTemp = (String) fileList.getSelectedValue();
   }

   public void updateList() {}

   public void updateDir()
   {
	String newTemp = (String) dirList.getSelectedValue();
	if (newTemp != null)
	{
	    if (newTemp.endsWith("/"))
		newTemp = newTemp.substring(0, newTemp.length() - 1);
	    if (newTemp.equals(".."))
	        outDir = parentDir;
	    else
	    {
	        if (outDir.endsWith("/"))
	            outDir = outDir + newTemp;
		else
	            outDir = outDir + File.separator + newTemp;
	    }
	    changeDir();
	}
   }

   public void updateFile()
   {
	String newFile = (String) fileList.getSelectedValue();
	if (newFile != null)
	    fileText.setText(newFile);
   }

   public void updateText(ActionEvent ev)
   {
	String  nstr;
	Object target = ev.getSource();
        if (target == dirText)
	{
	    nstr = dirText.getText().trim();
	    if (nstr.length() > 0)
	    {
	        outDir = nstr;
	        changeDir();
	    }
	}
   }


   private void getResolution() {
        String res = resText.getText().trim();
        if (res.length() <= 1) {
            if (curFormat.equals("PCL"))
               res = "300";
            else
               res = "72";
        }
        if (curFormat.equals("PCL"))
            pclRes = res;
        else
            otherRes = res;
        outDpi = res;
   }


   public void updateFormat(ActionEvent e)
   {
	JComboBox cb = (JComboBox) e.getSource();
        if (cb == null)
            return;
        String str = (String) cb.getSelectedItem();
        if (str == null)
            return;
	if (curFormat.equals("PCL"))
	    pclRes = resText.getText();
	else
	    otherRes = resText.getText();
	curFormat = str;
	if (curFormat.equals("PCL"))
	    resText.setText(pclRes);
	else
	    resText.setText(otherRes);
   }


   public void actionPerformed(ActionEvent  evt)
   {
	String cmd = evt.getActionCommand();
	if (cmd.equals("Save"))
	{
	    save_proc();
	}
	else if (cmd.equals("Close"))
	{
            writeOutputFormat();
	    setVisible(false);
	}
	else if (cmd.equals("Delete"))
	{
	}
   }

   private void build_format_choice()
   {
	formatChoice.addItem("AVS");
	formatChoice.addItem("BMP");
	formatChoice.addItem("EPS");
	formatChoice.addItem("FAX");
/*
	formatChoice.addItem("FDF");
*/
	formatChoice.addItem("FITS");
	formatChoice.addItem("GIF");
	formatChoice.addItem("GIF87");
/*
	formatChoice.addItem("GRAY");
	formatChoice.addItem("IRIS");
*/
	formatChoice.addItem("JPEG");
	formatChoice.addItem("MIFF");
	formatChoice.addItem("MTV");
	formatChoice.addItem("PCD");
	formatChoice.addItem("PCL");
	formatChoice.addItem("PCX");
	formatChoice.addItem("PDF");
	formatChoice.addItem("PICT");
	formatChoice.addItem("PGM");
	formatChoice.addItem("PNG");
	formatChoice.addItem("PS");
	formatChoice.addItem("PS2");
/*
	formatChoice.addItem("RGB");
*/
	formatChoice.addItem("SGI");
	formatChoice.addItem("SUN");
	formatChoice.addItem("TGA");
	formatChoice.addItem("TIFF");
	formatChoice.addItem("VIFF");
	formatChoice.addItem("XBM");
	formatChoice.addItem("XPM");
	formatChoice.addItem("XWD");
/*
	formatChoice.addItem("YUV");
*/

        if (curFormat == null)
	    curFormat = "PS";
	formatChoice.setSelectedItem(curFormat);
   }

   private void save_proc()
   {
	String path;
	String dir = dirText.getText();
	String file = fileText.getText().trim();
	if ((file == null) || (file.length() < 1))
	{
	    JOptionPane.showMessageDialog(this,
                 "File entry is empty.\nPlease enter file name.");
	    return;
	}
	if ((dir == null) || (dir.length() < 1)) {
	    JOptionPane.showMessageDialog(this,
                 "Directory entry is empty.\nPlease enter directory name.");
	    return;
	}
	if (dir.endsWith("/"))
	    path = dir+file;
	else
	    path = dir + File.separator + file;
        getResolution();
        outDir = dir;
        outFile = file;
	p_object.savePlotData(path);
   }

   public String dataFormat()
   {
/*
	String format = (String) formatChoice.getSelectedItem();
	String res = resText.getText().trim();
	if (res.length() <= 1)
	    res = "300";
	String out = "'"+format+"', "+"'"+res+"' ";
*/
	String out = "'"+curFormat+"', "+"'"+outDpi+"' ";
System.out.println(" out format: "+out);
	return (out);
   }

   private void writeOutputFormat() {
        if (userDir == null)
           return;
        if (curFormat == null)
            return;
        getResolution();
        String fpath = userDir+"/templates/plot/.outinfo";
        try {
          PrintWriter outStream = new PrintWriter(new FileWriter(fpath));
          if (outStream == null)
             return;
          outStream.println("outformat  "+curFormat);
          outStream.println("outdpi  "+outDpi);
          outStream.println("defaultdpi  "+otherRes);
          outStream.println("pcldpi  "+pclRes);
          outStream.println("outdir  "+outDir);
          outStream.println("outfile  "+outFile);
          outStream.close();
        }
        catch(IOException er) { }
   }

   private void readOutputFormat() {
        String  fpath, inData;
        String  name, data;
        StringTokenizer tok;
        BufferedReader inStream;

        if (userDir == null)
           return;
        inStream = null;
        fpath = userDir+"/templates/plot/.outinfo";
        try  {
              inStream = new BufferedReader(new FileReader(fpath));
        }
        catch(IOException e) { return; }
        if (inStream == null)
           return;
        try  {
           while ((inData = inStream.readLine()) != null)
           {
               if (inData.length() > 3) {
                   tok = new StringTokenizer(inData, " ,\n\t");
                   name = null;
                   data = null;
                   if (tok.hasMoreTokens())
                      name = tok.nextToken();
                   if (tok.hasMoreTokens())
                      data = tok.nextToken();
                   if (data != null) {
                      if (name.equals("outformat")) {
                         curFormat = data;
                         continue;
                      }
                      if (name.equals("outdpi")) {
                         outDpi = data;
                         continue;
                      }
                      if (name.equals("defaultdpi")) {
                         otherRes = data;
                         continue;
                      }
                      if (name.equals("pcldpi")) {
                            pclRes = data;
                         continue;
                      }
                      if (name.equals("outdir"))
                            outDir = data;
                      else if (name.equals("outfile"))
                            outFile = data;
                   }
               }
           }
           inStream.close();
        }
        catch(IOException e)
        { }
   }


   private JList dirList, fileList;
   private JPanel pan;
   private PlotConfig.PlPanel p_object;
   private JPanel pan1;
   private JPanel lpan;
   private JPanel defpan;
   private JPanel fpan;
   private JPanel butpan;
   private String parentDir = null;
   private String curFormat = null;
   private String pclRes = "300";
   private String otherRes = "90";
   private JTextField dirText, fileText, resText;
   private JComboBox formatChoice;
   private Font  ft;
   private fileDialog fd = null;
}

class fileDialog extends Dialog
{
   public fileDialog(JFrame parent)
   {  super(parent, "Warning!", true);

      setFont(new Font("Monospaced", Font.BOLD, 22));
      JPanel p1 = new JPanel();
      p1.setLayout(new GridLayout(2, 1));
      p1.add(new JLabel(" File entry is empty."));
      p1.add(new JLabel(" Please enter file name."));
      add(p1, "Center");

      JPanel p2 = new JPanel();
      JButton ok = new JButton("Ok");
      p2.add(ok);
      add(p2, "South");

      ok.addActionListener(new ActionListener() { public void
         actionPerformed(ActionEvent evt) { setVisible(false); } } );

      addWindowListener(new WindowAdapter() { public void
            windowClosing(WindowEvent e) { setVisible(false); } } );
      pack();
      setLocation(200, 60);
   }
}


