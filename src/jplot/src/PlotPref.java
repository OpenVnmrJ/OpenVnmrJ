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
import java.awt.*; 
import java.io.*; 
import java.util.*; 
import java.awt.event.*; 
// import PlotConfig.*; 

public class PlotPref extends JFrame
   implements  ActionListener 
{
   public PlotPref(PlotConfig.PlPanel parent, String usrdir)
   {
	p_object = parent;
	userDir = usrdir;
	ft = new Font("Monospaced", Font.BOLD, 16);
	setTitle("Workspace Preferences");
	setLocation(100, 30);

	readPreference();
	pan = new JPanel();
	// pan.setLayout(new GridLayout(9, 1));
	pan.setFont(ft);

	pan.setLayout(new GridLayout( 0, 2));
	pan.add(new JLabel(" Background:   "));
	bgChoice = new JComboBox();
        build_color_choice(bgChoice);
	if (bg_color != null)
	   bgChoice.setSelectedItem(bg_color);
	else
	   bgChoice.setSelectedItem("gray");
	pan.add(bgChoice);

	pan.add(new JLabel(" Border Color: "));
	bdChoice = new JComboBox();
        build_color_choice(bdChoice);
	if (bd_color != null)
	   bdChoice.setSelectedItem(bd_color);
	else
	   bdChoice.setSelectedItem("gray");
	pan.add(bdChoice);

	pan.add(new JLabel(" Highlight Color:"));
	hiChoice = new JComboBox();
        build_color_choice(hiChoice);
	if (hi_color != null)
	   hiChoice.setSelectedItem(hi_color);
	else
	   hiChoice.setSelectedItem("red");
	pan.add(hiChoice);

	pan.add(new JLabel(" Grid Color:   "));
	gdChoice = new JComboBox();
        build_color_choice(gdChoice);
	if (gd_color != null)
	   gdChoice.setSelectedItem(gd_color);
	else
	   gdChoice.setSelectedItem("gray");
	pan.add(gdChoice);

	pan.add(new JLabel(" Plotter:   "));
	plotChoice = new JComboBox();
        build_plot_choice();
	pan.add(plotChoice);

	pan.add(new JLabel(" Border:   "));
	borderChoice = new JComboBox();
        build_border_choice();
	pan.add(borderChoice);

	pan.add(new JLabel(" Snap:  "));
	snapChoice = new JComboBox();
        build_snap_choice();
	pan.add(snapChoice);

	pan.add(new JLabel(" Grid:    "));
	gridChoice = new JComboBox();
        build_grid_choice();
	pan.add(gridChoice);

	pan.add(new JLabel(" Snap Spacing: "));

	JPanel spacepan = new JPanel();
        // spacepan.setLayout(new FlowLayout(FlowLayout.LEFT));
	spacepan.setLayout(new GridLayout( 1, 0));
	spaceText = new JTextField("0.5", 6);
	spaceChoice = new JComboBox();
        build_space_choice();
	spacepan.add(spaceText);
	spacepan.add(spaceChoice);
	if (snapSpace != null)
	    spaceText.setText(snapSpace);
	pan.add(spacepan);

	butpan = new JPanel();
	butpan.setFont(ft);
	addButton("Apply", this);
	addButton("Close", this);

	Container cont = getContentPane();
	cont.add(pan, "Center");
	cont.add(butpan, "South");
	
	pack();	
	addWindowListener(new WindowAdapter() {
           public void  windowClosing(WindowEvent e) { setVisible(false); } } );
   }


   public void addButton(String label, Object obj)
   {
	JButton but = new JButton(label);
	but.addActionListener((ActionListener) obj);
	butpan.add(but);
   }

   public void actionPerformed(ActionEvent  evt)
   {
	String cmd = evt.getActionCommand();
	if (cmd.equals("Apply"))
	{
	    apply_proc();
	}
	else if (cmd.equals("Close"))
	{
	    this.setVisible(false);
	}
   }

   private void build_color_choice(JComboBox c)
   {
	String  s;

        s = PlotRgb.getColorName(0);
        while (s != null)
        {
            c.addItem(s);
            s = PlotRgb.getColorName(1);
        }
   }

   private void build_plot_choice()
   {
	plotChoice.addItem("black & white");
	plotChoice.addItem("color");
	if (colorPlot)
	   plotChoice.setSelectedItem("color");
	else
	   plotChoice.setSelectedItem("black & white");
   }

   private void build_border_choice()
   {
	borderChoice.addItem("off");
	borderChoice.addItem("on");
	if (borderOn)
	   borderChoice.setSelectedItem("on");
	else
	   borderChoice.setSelectedItem("off");
   }

   private void build_grid_choice()
   {
	gridChoice.addItem("off");
	gridChoice.addItem("on");
	if (gridOn)
	   gridChoice.setSelectedItem("on");
	else
	   gridChoice.setSelectedItem("off");
   }

   private void build_snap_choice()
   {
	snapChoice.addItem("off");
	snapChoice.addItem("on");
	if (snap != null)
	   snapChoice.setSelectedItem(snap);
	else
	   snapChoice.setSelectedItem("off");
   }

   private void build_space_choice()
   {
	spaceChoice.addItem("inch");
	spaceChoice.addItem("cm");
	spaceChoice.addItem("point");
	if (snapUnit != null)
	   spaceChoice.setSelectedItem(snapUnit);
	else
	   spaceChoice.setSelectedItem("inch");
   }

   private void apply_proc()
   {
	String  data;
	String fpath = userDir+"/templates/plot/.preference";

	try {
	  PrintWriter outStream = new PrintWriter(new FileWriter(fpath));
          if (outStream == null)
             return;
	  outStream.println("background  "+bgChoice.getSelectedItem());
	  outStream.println("hilight  "+hiChoice.getSelectedItem());
	  outStream.println("border  "+bdChoice.getSelectedItem());
	  outStream.println("grid  "+gdChoice.getSelectedItem());
	  outStream.println("gridline  "+gridChoice.getSelectedItem());
	  outStream.println("borderline  "+borderChoice.getSelectedItem());
	  data = (String) plotChoice.getSelectedItem();
	  if (data.equals("color"))
	     outStream.println("plotter  color");
	  else
	     outStream.println("plotter  b&w");
	  outStream.println("snap  "+snapChoice.getSelectedItem());
	  outStream.print("snapspacing  ");
	  data = spaceText.getText().trim();
	  if ((data == null) || (data.length() < 1))
	     outStream.print("0 ");
	  else
	     outStream.print(" "+data);
	  outStream.println(" "+spaceChoice.getSelectedItem());
	  outStream.close();
        }
        catch(IOException er) { }
	p_object.setPreference();
   }

   private void readPreference()
   {
	String  fpath, inData;
        String  name;
        StringTokenizer tok;
        BufferedReader inStream;

        if (userDir == null)
	   return;
        inStream = null;
        fpath = userDir+"/templates/plot/.preference";
        try  {
              inStream = new BufferedReader(new FileReader(fpath));
        }
        catch(IOException e) { return; }
        if (inStream == null)
           return;
        try  {
           while ((inData = inStream.readLine()) != null)
           {
                if (inData.length() > 3)
                {
                   tok = new StringTokenizer(inData, " ,\n\t");
                   name = tok.nextToken();
                   if (name.equals("background"))
                      bg_color = tok.nextToken();
                   else if (name.equals("hilight"))
                      hi_color = tok.nextToken();
                   else if (name.equals("border"))
                      bd_color = tok.nextToken();
                   else if (name.equals("grid"))
                      gd_color = tok.nextToken();
                   else if (name.equals("borderline"))
		   {
                      name = tok.nextToken();
                      if (name.equals("on"))
			 borderOn = true;
		      else
			 borderOn = false;
		   }
                   else if (name.equals("gridline"))
		   {
                      name = tok.nextToken();
                      if (name.equals("on"))
			 gridOn = true;
		      else
			 gridOn = false;
		   }
                   else if (name.equals("plotter"))
		   {
                      name = tok.nextToken();
                      if (name.equals("color"))
			 colorPlot = true;
		      else
			 colorPlot = false;
		   }
                   else if (name.equals("snap"))
                   {
                      name = tok.nextToken();
                      if (name.equals("On") || name.equals("on"))
                          snap = "on";
                      else
                          snap = "off";
                   }
                   else if (name.equals("snapspacing"))
                   {
                      snapSpace = tok.nextToken();
                      snapUnit = tok.nextToken();
                   }
                   else if (name.equals("linewidth"))
                      lineWidth = tok.nextToken();
                 }
           }
           inStream.close();
        }
        catch(IOException e)
        {
        }
   }

   private JPanel pan;
   private PlotConfig.PlPanel p_object;
   private JPanel butpan, plotpan;
   private JTextField spaceText, lineText;
   private JComboBox fgChoice, bgChoice, snapChoice;
   private JComboBox spaceChoice, hiChoice, bdChoice;
   private JComboBox plotChoice, gdChoice, gridChoice;
   private JComboBox borderChoice;
   private boolean colorPlot = false;
   private boolean gridOn = false;
   private boolean borderOn = true;
   private Font  ft;
   private String  userDir;
   private String bg_color = null;
   private String hi_color = null;
   private String bd_color = null;
   private String gd_color = null;
   private String snap = null;
   private String snapSpace = null;
   private String snapUnit = null;
   private String lineWidth = null;
}

