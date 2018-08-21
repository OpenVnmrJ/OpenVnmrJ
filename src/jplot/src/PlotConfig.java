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
import java.io.*;
import java.util.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import sun.misc.*;

// import PlotEditTool.*;
// import PlotItemPref.*;

public class PlotConfig extends JFrame 
   implements ActionListener, PlotDefines, PlotIF
{  public PlotConfig()
   {  
	
      cont = getContentPane();
      mbar = new JMenuBar();
      mbar.setFont(new Font("Dialog", Font.BOLD, 14));
      
      setLocation(100, 0);
      setTitle("Plot Designer");
      cont.setLayout(new BorderLayout());
      mbar.add(makeMenu("File",
         new Object[] 
         {  
	    "Templates", 
            null, 
            "Save Data", 
            null, 
            "Quit" 
         },
         this));

      mbar.add(makeMenu("Region",
         new Object[] 
         {  "New", 
            null, 
            "Edit", 
            null, 
            "Delete", 
            "Undelete", 
            "Delete All", 
            null, 
            "Preferences" 
         },
         this));
        
      mbar.add(makeMenu("Preview", 
         new Object[] 
         {  
            "Selected",
            "All"
         },
         this));

      mbar.add(makeMenu("Magnify",
         new Object[] 
         {
            "150%", 
            "125%", 
            "100%", 
            "75%", 
            "50%", 
            "25%" 
         },
         this));

      mbar.add(makeMenu("Preferences", 
         new Object[] 
         {  
            "Set Up"
         },
         this));
        
      mbar.add(makeMenu("Orient",
         new Object[] 
         {  "Landscape", 
            "Portrait" 
         },
         this));

/*
      mbar.add(makeMenu("Help", 
         new Object[] 
         {  
            "About",
         },
         this));
*/

      setJMenuBar(mbar);
   
      sysDir = System.getProperty("sysdir");
      if (sysDir == null || sysDir.length() < 1)
	 sysDir = "/vnmr";
      
      tmpDirUnix="/vnmr/tmp/";
      tmpDirSystem=sysDir+"/tmp/";
      userDir = System.getProperty("userdir");
      userName = System.getProperty("user");
      if (userName == null)
	userName = "x";

      make_popup_menu();
      fileList = new PlotFile(null);
      open_pre_template(false);
      dp = new PlPanel();
      fileList.setTopObj(dp);
      sp = new JScrollPane(dp, JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                            JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
      PlotUtil.setMainClass(this);
      if (popup != null)
         cont.add(popup);
   
      toolpanel = new PlotEditTool(sysDir);
      PlotUtil.setEditTool(toolpanel);
      cont.add(toolpanel, "West");
      cont.add(sp, "Center");
      pack();
      readPreference();
      dp.setBackground(bgColor);
      dp.setForeground(fgColor);
      ipPopup = new PlotItemPref(dp, userDir);
      ipPopup.setItemColor(fgColor);
      toolpanel.setParentInfo(dp);
      toolpanel.setPrefWindow(ipPopup);
      PlotUtil.setPrefWindow(ipPopup);
      rd = new RegionEdit(dp, "");
      rd.setLocation(screen_width / 2 - 200, 300);
      oldSnapSpace = snapSpace;
      oldgridFlag = gridFlag;
      borderFlag = 1;

	if (vnmrPort > 0) {
	    toVnmr = new ToVnmrSocket(vnmrHost, vnmrPort);
	}
	inSocket = new PlotSocket(this);
	inThread = new Thread(inSocket);
        inPort = inSocket.getPort();
        inThread.start();
	PlotUtil.setVnmrPort(toVnmr);
	if (toVnmr != null) {
	    String d = "jplot('-port', "+inPort+")\n";
	    toVnmr.send(d);
	}
      addWindowListener(new WindowAdapter() {
            public void windowOpened(WindowEvent e) { openFrame();  }
            public void windowClosing(WindowEvent e) { closeFrame(); } } );
      bimage = dp.createImage(pwidth, pheight);

      Signal.handle(new Signal("TERM"), new SignalHandler() {
             public void handle(Signal sig) {
		closeFrame();
             }
        });
   }


   public void openFrame()
   {
	if (screen_height < 1000)
	    set_size();
        open_pre_template(true);
   }

   public void closeFrame()
   {
	dp.saveTemplate(".temp", true);
	writePreference();
	if (toVnmr != null) {
	   String d = "jplot('-exit',"+inPort+")\n";
	   toVnmr.send(d);
	}
        System.exit(0);
   }

   public void processData(String str) {
	vnmrData = str;
	if (str.equals("open_jplot")) {
	    setState(Frame.NORMAL);
	    return;
	}
	if (str.equals("exit_jplot")) {
	    closeFrame();
	    return;
	}

	if (toWait) {
	   toWait = false;
	}
   }


   public PlotConfig(String args[])
   {  
      int k, m;
      for (k = 0; k < args.length; k++)
      {
	 if (args[k].startsWith("-h")) {
	    vnmrHost = args[k].substring(2);
	    k++;
	    if (k < args.length) {
		try {
	       	    m = Integer.parseInt(args[k]);
		    vnmrPort = m;
		}
                catch (NumberFormatException er) { }
	    }
	    k++;
	    if (k < args.length) {
		try {
	       	    m = Integer.parseInt(args[k]);
		    vnmrId = m;
		}
                catch (NumberFormatException er) { }
	    }
	 }
      }
      main_frame = new PlotConfig();
      main_frame.setVisible(true);
   }

   private static JMenu makeMenu(Object parent, 
      Object[] items, Object target)
   {
      JMenu m = null;
      String plabel;

      if (parent instanceof JMenu)
      {
         m = (JMenu)parent;
         plabel = "Popup";
      }
      else if (parent instanceof String)
      {
         m = new JMenu((String)parent);
	 plabel = (String)parent;
      }
      else
         return null;

      for (int i = 0; i < items.length; i++)
      {  if (items[i] instanceof String)
         {  JMenuItem mi = new JMenuItem((String)items[i]);
            if (target instanceof ActionListener)
               mi.addActionListener((ActionListener)target);
	    mi.setActionCommand(plabel);
            m.add(mi);
         }
         else if (items[i] instanceof JMenuItem)
         {  JMenuItem mi = (JMenuItem)items[i];
            if (target instanceof ActionListener)
               mi.addActionListener((ActionListener)target);
	    mi.setActionCommand(plabel);
            m.add(mi);
         }
         else if (items[i] == null) 
            m.addSeparator();
      }

      return m;
   }
   
   public void actionPerformed(ActionEvent evt)
   {  JMenuItem c = (JMenuItem)evt.getSource();  
      String arg = c.getText();
      String parg = c.getActionCommand();
      toolpanel.clearActions();
      dp.setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
      if(parg.equals("File"))
      {
	   dp.clearHilit();
	   fileAction(c);
      }
      else if(parg.equals("Orient"))
	   orientAction(c);
      else if(parg.equals("Region"))
      {
	   regionFunction(c);
      }
      else if(parg.equals("Magnify"))
	   magnifyAction(c);
      else if(parg.equals("Preview"))
	   previewAction(c);
      else if(parg.equals("Preferences"))
      {
	   preferenceAction(c);
      }
      else if(parg.equals("Help"))
      {
	   helpAction(c);
      }
      else if(parg.equals("menu"))
      {
	   regionAction(c);
      }
   }

   private void make_popup_menu()
   {
      int	   k;
      String 	   file, data, cmd;
      BufferedReader in;

      file = userDir+"/templates/plot";
      File fd = new File(file);
      if (!fd.exists())
             fd.mkdirs();
      file = userDir+"/templates/plot/menu";
      fd = new File(file);
      if (!fd.exists() || !fd.canRead()) {
          file = sysDir+"/user_templates/plot/menu";
          fd = new File(file);
      }
      if (!fd.exists() || !fd.canRead()) {
	  return;
      }
       
      in = null;
      try  {
           in = new BufferedReader(new FileReader(file));
           }
      catch(IOException e) { }
      if (in == null)
	  return;
      popup = new JPopupMenu();
      cmd = null;
      popup.setFont(new Font("Dialog", Font.BOLD, 14));
      try  {
           while ((data = in.readLine()) != null)
           {
		data = data.trim();
                if (data.length() > 0)
		{
		    if (cmd == null)
			cmd = data;
		    else
		    {
		        PlotMenuNode mn = new PlotMenuNode(cmd, data);
			JMenuItem mi = new JMenuItem(cmd);
                        mi.addActionListener((ActionListener)this);
            	        mi.setActionCommand("menu");
            	        popup.add(mi);
			cmd_list.append(mn);
			cmd = null;
		    }
		}
	   }
	   in.close();
      }
      catch(IOException e) { }
   }


   private void set_pre_orient(String fpath) {
	String  inData;
	String  name;
	StringTokenizer tok;
	BufferedReader inStream;

	try {
	     inStream = new BufferedReader(new FileReader(fpath));
	     if (inStream == null) {
		   return;
	     }
		
	    name = " ";
	    while ((inData = inStream.readLine()) != null)
	    {
	      if (inData.length() > 0)
	      {
		   tok = new StringTokenizer(inData, " ,\n\t");
		   if (tok.hasMoreTokens())
		       name = tok.nextToken();
		   if (name.equals(vcommand)) {
		       if (tok.hasMoreTokens())
		        name = tok.nextToken();
		       if (name.equals("'-plotter'"))
		       {
			  if (tok.hasMoreTokens())
		    	    name = tok.nextToken();
			  if (name.equals("'PS_AR'"))
			  {
      			    if (direction == 1)  /* Portrait */
			    {
			 	direction = 2;
			    }
			  }
			  else if (name.equals("'PS_A'"))
			  {
      			    if (direction == 2)  /* Landscape  */
			    {
			 	direction = 1;
			    }
			  }
			  break;
		       }
		   }
	       }
	    }
	    inStream.close();
	}
	catch(IOException e) {
      	}
   }



   private void open_pre_template(boolean to_draw)
   {
	int	   k;

	temp_name = ".temp";
	String f = userDir+"/templates/plot/"+temp_name;
	File fd = new File(f);
        if (!fd.exists() || !fd.canRead()) {
	   temp_name = fileList.getDefaultTemplate();
	   if (temp_name == null)
		return;
	   f = userDir+"/templates/plot/"+temp_name;
	   fd = new File(f);
           if (!fd.exists() || !fd.canRead()) {
	      f = sysDir+"/user_templates/plot/"+temp_name;
	      fd = new File(f);
	   }
        }
        if (!fd.exists() || !fd.canRead()) {
	    return;
	}

	if (to_draw) {
	   dp.openTemplate(f);
	   if (temp_name != null && !temp_name.equals(".temp"))
	      fileList.setItem(temp_name);
	}
	else
      	   set_pre_orient(f);
   }


   public void readPreference()
   {
	String  fpath, inData;
	String  name;
	int	v1, v2, v3;
	double  fval;
	StringTokenizer tok;
	BufferedReader inStream;

	inStream = null;
	if (userDir != null)
	{
	   fpath = userDir+"/templates/plot/.preference";
	   try  {
	      inStream = new BufferedReader(new FileReader(fpath));
	   }
	   catch(IOException e) { }
	}
	if (inStream == null)
	{
	   fpath = sysDir+"/user_templates/plot/.preference";
	   try  {
	      inStream = new BufferedReader(new FileReader(fpath));
	   }
	   catch(IOException e) { }
	}
	if (inStream == null)
	   return;
	try  {
	   while ((inData = inStream.readLine()) != null)
	   {
	   	if (inData.length() > 3)
	        {
		   tok = new StringTokenizer(inData, " ,\n\t");
		   name = tok.nextToken();
		   if (name.equals("foreground"))
		   {
      		      v1 = Format.atoi(tok.nextToken());
      		      v2 = Format.atoi(tok.nextToken());
      		      v3 = Format.atoi(tok.nextToken());
		      fgColor = new Color(v1, v2, v3);
		   }
		   else if (name.equals("background"))
		   {
		      bgName = tok.nextToken();
		      bgColor = PlotUtil.getColor(bgName);
		   }
		   else if (name.equals("hilight"))
		   {
		      hiName = tok.nextToken();
		      hiColor = PlotUtil.getColor(hiName);
		   }
		   else if (name.equals("border"))
		   {
		      borderName = tok.nextToken();
		      borderColor = PlotUtil.getColor(borderName);
		   }
		   else if (name.equals("grid"))
		   {
		      gridName = tok.nextToken();
		      gridColor = PlotUtil.getColor(gridName);
		   }
		   else if (name.equals("borderline"))
		   {
		      name = tok.nextToken();
		      if (name.equals("on"))
		          borderFlag = 1;
		      else
		          borderFlag = 0;
		   }
		   else if (name.equals("gridline"))
		   {
		      name = tok.nextToken();
		      if (name.equals("on"))
		          gridFlag = 1;
		      else
		          gridFlag = 0;
		   }
		   else if (name.equals("plotter"))
		   {
		      name = tok.nextToken();
		      if (name.equals("color"))
		          colorPlotter = true;
		      else
		          colorPlotter = false;
		      PlotUtil.setColorPlotter(colorPlotter);
		   }
		   else if (name.equals("snap"))
		   {
		      name = tok.nextToken();
		      if (name.equals("on") || name.equals("On"))
		          snapFlag = true;
		      else	
		          snapFlag = false;
		   }
		   else if (name.equals("snapspacing"))
		   {
		      snapValue = tok.nextToken();
      		      fval = Format.atof(snapValue);
      		      snapUnit = tok.nextToken();
		      if (snapUnit.equals("inch"))
			 snapSpace = (int) (fval * screen_res);
		      else if (snapUnit.equals("cm"))
			 snapSpace = (int) (fval * screen_res / 2.54);
		      else
			 snapSpace = (int) fval;
		      if (snapSpace > (pwidth / 2))
			snapSpace = pwidth / 2;
		      if (snapSpace < 1)
			snapSpace = 1;
		      snapGap = (double) snapSpace * mag;
		   }
		   else if (name.equals("linewidth"))
      		      lineWidth = Format.atoi(tok.nextToken());
		 }
	   }
	   inStream.close();
	}
	catch(IOException e)
	{
	   System.out.print("Error: " + e);
      	}
   }

   private void writePreference()
   {
        String fpath = userDir+"/templates/plot/.preference";

        try {
          PrintWriter os = new PrintWriter(new FileWriter(fpath));
          if (os == null)
             return;
          os.println("background  "+bgName);
          os.println("hilight  "+hiName);
          os.println("border  "+borderName);
          os.println("grid  "+gridName);
	  if (gridFlag > 0)
              os.println("gridline  on");
	  else
              os.println("gridline  off");
	  if (borderFlag > 0)
              os.println("borderline  on");
	  else
              os.println("borderline  off");
	  if (snapFlag)
              os.println("snap  on");
	  else
              os.println("snap  off");
          os.println("snapspacing  "+snapValue+" "+snapUnit);
          os.print("foreground ");
	  Format.print(os, "%d ", fgColor.getRed());
	  Format.print(os, "%d ", fgColor.getGreen());
	  Format.print(os, "%d", fgColor.getBlue());
          os.println(" ");
	  if (colorPlotter)
              os.println("plotter   color");
	  else
              os.println("plotter   b&w");
          os.close();
        }
        catch(IOException er) { }
   }


   public static void main(String args[])
   {  
	int  k, m;

      for (k = 0; k < args.length; k++)
      {
	 if (args[k].startsWith("-h")) {
	    vnmrHost = args[k].substring(2);
	    k++;
	    if (k < args.length) {
		try {
	       	    m = Integer.parseInt(args[k]);
		    vnmrPort = m;
		}
                catch (NumberFormatException er) { }
	    }
	    k++;
	    if (k < args.length) {
		try {
	       	    m = Integer.parseInt(args[k]);
		    vnmrId = m;
		}
                catch (NumberFormatException er) { }
	    }
	 }
      }
      main_frame = new PlotConfig();
      main_frame.setVisible(true);
   }

   public void raiseUp()
   {
      // main_frame.show();
      main_frame.setVisible(true);
      main_frame.setState(Frame.NORMAL);
   }

   private void fileAction(JMenuItem item) 
   {
      String arg = item.getText();
      if(arg.equals("Quit"))
      {
	 closeFrame();
      }
      else if(arg.equals("Templates"))
      {
	if (fileList == null)
	{
	    fileList = new PlotFile(dp);
	}
	// fileList.show();
	fileList.setVisible(true);
        fileList.setState(Frame.NORMAL);
      }
      else if(arg.equals("Save Data"))
      {
	if (savePopup == null)
	{
	    savePopup = new PlotSave(dp, userDir);
	}
	// savePopup.show();
	savePopup.setVisible(true);
        savePopup.setState(Frame.NORMAL);
      }
      else if(arg.equals("Jprint"))
      {
	 dp.print(this);
      }
      else if(arg.equals("Vprint"))
      {
	 dp.vprint();
      }
   }

   private void regionFunction(JMenuItem item) 
   {
      String arg = item.getText();
      if(arg.equals("New"))
      {
	  dp.clearHilit();
          dp.setCursor(Cursor.getPredefinedCursor(Cursor.CROSSHAIR_CURSOR));
	  win_mode = NEW;
	  return;
      }
      if(arg.equals("Edit"))
      {
	  dp.showEditWindow();
	  return;
      }

      if(arg.equals("Delete"))
      {
	  dp.delete();
	  return;
      }
      if(arg.equals("Undelete"))
      {
	  dp.undelete();
	  return;
      }
      if(arg.equals("Delete All"))
      {
	  dp.delete_all();
	  return;
      }
      if(arg.equals("Preferences"))
      {
	  itemPrefAction();
	  return;
      }
   }

   private void set_size()
   {
      DrawRegion   d;

      Point pt = sp.getLocationOnScreen();
      Point ptm = main_frame.getLocationOnScreen();
      int dx = pt.x - ptm.x;
      int dy = pt.y - ptm.y;
      if (direction == 2)  // Landscape
      {
	  pwidth = (int) (screen_res * paper_height * mag)+3;
	  pheight = (int) (screen_res * paper_width * mag)+3;
      }
      else
      {
	  pwidth = (int) (screen_res * paper_width * mag)+3;
	  pheight = (int) (screen_res * paper_height * mag)+3;
      }
      PlotUtil.setWinDimension(pwidth, pheight);
      snapGap = (double)oldSnapSpace * mag;
      dp.setPreferredSize(new Dimension(pwidth+3, pheight+3));
      a_list.reset();
      Enumeration  e = a_list.elements();
      while (e.hasMoreElements())
      {
	  d = (DrawRegion)e.nextElement();
	  d.changeParentSize(pwidth, pheight);
	  d.setRatio(mag);
      }
      Dimension dm = toolpanel.getSize();
      toolpanel.setSize(dm.width, pheight);
      toolpanel.setRatio(mag, pwidth, pheight);
      bimage = dp.createImage(pwidth, pheight);
      bufferEmpty = true;
      main_frame.pack();
   }

   private void orientAction(JMenuItem item) 
   {
      String arg = item.getText();
      if(arg.equals("Landscape"))
      {
	  if (direction == 2)
		return;
	  direction = 2;
      }
      else if(arg.equals("Portrait"))
      {
	  if (direction == 1)
		return;
	  direction = 1;
      }
      set_size();
   }

   private void previewAction(JMenuItem item) 
   {
	String arg = item.getText();
	if(arg.equals("Selected"))
	   dp.preview(false);
	else
	   dp.preview(true);
   }

   private void magnifyAction(JMenuItem item) 
   {
      double new_mag;

      String arg = item.getText();
      new_mag = mag;
      if(arg.equals("125%"))
	  new_mag = 1.25;
      if(arg.equals("100%"))
	  new_mag = 1.0;
      else if (arg.equals("75%"))
	  new_mag = 0.75;
      else if (arg.equals("50%"))
	  new_mag = 0.5;
      else if (arg.equals("25%"))
	  new_mag = 0.25;
      else if (arg.equals("125%"))
	  new_mag = 1.25;
      else if (arg.equals("150%"))
	  new_mag = 1.5;
      if (new_mag != mag)
      {
	  mag = new_mag;
	  set_size();
      }

   }

   private void regionAction(JMenuItem item) 
   {
      PlotMenuNode  node;
      String arg = item.getText();

      if (rd == null)
	  return;
      cmd_list.reset();
      Enumeration  e = cmd_list.elements();
      while (e.hasMoreElements())
      {
	 node = (PlotMenuNode)e.nextElement();
	 String cmd = node.getCommand();
	 if (cmd.equals(arg))
	 {
		rd.setData(node.getValue());
		if (hilitRegion != null)
		{
		    hilitRegion.setData(node.getValue());
		    dp.preview(false);
		}
		return;
	 }
      }
   }

   private void helpAction(JMenuItem item) 
   {
	if (helpPopup == null)
	{
	    helpPopup = new PlotHelp();
	}
	helpPopup.show();
   }

   private void preferenceAction(JMenuItem item) 
   {
	if (prefPopup == null)
	{
	    prefPopup = new PlotPref(dp, userDir);
	}
	// prefPopup.show();
	prefPopup.setVisible(true);
        prefPopup.setState(Frame.NORMAL);
   }

   private void showPreference()
   {
	if ((ipPopup == null) || (!ipPopup.isVisible()))
	    return;
	if (hilitRegion != null)
	{
	    hilitRegion.showPreference();
	    return;
	}
   }

   private void showPreference(PlotObj p)
   {
	if ((ipPopup == null) || (!ipPopup.isVisible()))
	    return;
	p.showPreference();
   }


   private void itemPrefAction()
   {
	if (ipPopup == null)
	{
	    ipPopup = new PlotItemPref(dp, userDir);
	}
	ipPopup.setVisible(true);
        ipPopup.setState(Frame.NORMAL);
   }

   private int snapAdjust(int v, boolean vertical)
   {
	int	nv;

	if (!snapFlag || (snapGap < 2.0))
	   return v;
	nv = (int) ((double) v / snapGap + 0.5);
	nv = (int) (nv * snapGap);
	if (vertical)
	{
	   if (nv >= pheight)
		nv = nv - (int) snapGap;
	}
	else
	{
	   if (nv >= pwidth)
		nv = nv - (int)snapGap;
	}
	if (nv < 1)
	   nv = 1;
	return nv;
   }

   public void backUp() {
	dp.backUpArea();
   }

   public void restoreArea(Rectangle rect) {
	dp.recoverArea(rect);
   }

   public Color getWinBackground() {
	return dp.getWinBackground();
   }

   public Color getWinForeground() {
	return dp.getWinForeground();
   }

   public Color getHighlightColor() {
	return dp.getHighlightColor();
   }

   public Graphics getGC() {
	return dp.getWinGraphics();
   }

   public Image createBuffer(int w, int h) {
        return dp.createImage(w, h);
   }

   class PlPanel extends JPanel
        implements MouseMotionListener, KeyListener
   {
	public PlPanel()
	{
	   Toolkit tk = Toolkit.getDefaultToolkit();
	   Dimension d = tk.getScreenSize();

	   screen_height = d.height;
	   screen_width = d.width;
	   screen_res = tk.getScreenResolution();
	   paper_width = 7.5;
	   paper_height = 10.0;
	   if (direction == 1) {
	      pwidth = (int) (screen_res * paper_width);
	      pheight = (int) (screen_res * paper_height);
	   }
	   else {
	      pwidth = (int) (screen_res * paper_height);
	      pheight = (int) (screen_res * paper_width);
	   }
      	   PlotUtil.setWinDimension(pwidth, pheight);
	   setPreferredSize(new Dimension(pwidth+3, pheight+3));
      	   addMouseListener(new MouseInputAdapter() 
           {  
	      public void mouseClicked(MouseEvent evt) 
              {  
		int clicks = evt.getClickCount();
   		DrawRegion newHilitRegion;

		modifier = evt.getModifiers();
		if ((modifier & (1 << 4)) != 0)
		{
		    if (clicks >= 2)
		    {
		        removeKeyListener(dp);
			toolpanel.selectItem(evt);
			if (win_mode == MODIFY)
			{
	   		     addKeyListener(dp);
			     requestFocus();
			     return;
			}
		    	selectRegion(evt);
		    }
		}
		if ((modifier & InputEvent.ALT_MASK) != 0) // button 2 with ctl
		{
		    selectRegion(evt);
		}
              } 

	      public void mousePressed(MouseEvent evt)
	      {
		modifier = evt.getModifiers();

		if ((win_mode & MODIFY) != 0)
		{
		    toolpanel.mEvent(evt);
		    return;
		}
	        gc = getGraphics();
		if ((win_mode & NEW) != 0)
		{
		    int  x = evt.getX();
		    int  y = evt.getY();
		    org_x = evt.getX();
		    org_y = evt.getY();
		    new_w = 0;
		    new_h = 0;
		}
		else
		{
		    modifier = evt.getModifiers();

		    if ((modifier & (1 << 1)) != 0) // Control key pressed
		    {
			win_mode = 0;
			if ((modifier & (1 << 3)) != 0) // button 2
			{
			}
			else if ((modifier & (1 << 2)) != 0) // button 3
			{
			}
		  	else  // button 1
			{
		    	    if (hilitRegion == null)
				return;
			    startMoveRegion(evt);
			}
		    }
		    else
		    {
			if ((modifier & (1 << 2)) != 0) // button 3
			{
			   if ((popup != null) && (hilitRegion != null))
			       popup.show(evt.getComponent(), evt.getX(), evt.getY());
		    	}
		    	else if (win_mode == 0)
			{
			    if ((modifier == 0) && (hilitRegion != null))
		    	    {
			        startMoveRegion(evt);
			    }
			}
		    }
		}
	      }

	      public void mouseEntered(MouseEvent evt)
	      {
		if ((win_mode & MODIFY) != 0)
		{
		    toolpanel.mEvent(evt);
		    return;
		}
	      }

	      public void mouseReleased(MouseEvent evt)
	      {
		if ((win_mode & MODIFY) != 0)
		{
		    removeKeyListener(dp);
		    toolpanel.mEvent(evt);
		    return;
		}
		if ((win_mode & NEW) != 0)
		{
      		     setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
		     win_mode = 0;
		     if (new_w > 6 && new_h > 6)
		     {
			regions++;
			if (snapFlag && (snapSpace > 1))
			{
			    drawArea();
			    new_x = snapAdjust(new_x, false);
			    new_w = snapAdjust(new_w, false);
			    new_y = snapAdjust(new_y, true);
			    new_h = snapAdjust(new_h, true);
			    if (new_x + new_w > pwidth)
				new_w = new_w - snapSpace;
			    if (new_y + new_h > pheight)
				new_h = new_h - snapSpace;
			}
		    	newRegion = new DrawRegion(dp,regions, new_x, new_y, new_w, new_h);
			newRegion.setColors(fgColor, borderColor, hiColor);
	  		newRegion.setBorder(borderFlag);
	    		newRegion.setPrefWindow(ipPopup);
			newRegion.setRatio(mag);
			newRegion.regionDraw();
			a_list.append(newRegion);
		     }
		     return;
		}
	        else if ((win_mode & MOVE) != 0)
		{
		     moveArea(evt, 2);
      		     setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
		     return;
		}
	    	else if ((hilitRegion != null) && ((win_mode & RESIZE) != 0))
		{
		     modifier = evt.getModifiers();
		     if ((modifier & (1 << 4)) == 0)
			return;
		     if (!snapFlag || (snapSpace < 2))
		     {
			hilitRegion.mEvent(evt, RELEASE);
			return;
		     }
		     Rectangle  dm = hilitRegion.getResizeDim();
		     if (dm.x < 0 || dm.y < 0 || dm.width < 1 || dm.height < 1)
			return;
		     hilitRegion.xorOutLine();
		     dm.x = snapAdjust(dm.x, false);
		     dm.width = snapAdjust(dm.width, false);
		     if (dm.x + dm.width >= pwidth)
			dm.width = dm.width - snapSpace;
		     dm.y = snapAdjust(dm.y, true);
		     dm.height = snapAdjust(dm.height, true);
		     if (dm.y + dm.height >= pheight)
			dm.height = dm.height - snapSpace;
		     hilitRegion.resizeRegion (dm);
		}
		else if (hilitRegion != null)
		{
		     modifier = evt.getModifiers();
		     if ((modifier & (1 << 4)) != 0)
			hilitRegion.mEvent(evt, RELEASE);
		}
	      }

	      public void mouseExited(MouseEvent evt)
	      {
		if ((win_mode & MODIFY) != 0)
		{
		    toolpanel.mEvent(evt);
		    return;
		}
		if ((win_mode & NEW) != 0)
		{
/*
		     win_mode = 0;
*/
		}
		else if (hilitRegion != null)
		{
		     modifier = evt.getModifiers();
		     if (modifier == 0)
			hilitRegion.mEvent(evt, EXIT);
		}
	      }
           });  

	   addComponentListener(new ComponentAdapter()
      	   {  
	      public void componentResized(ComponentEvent evt)
	      {
		 int  w = getSize().width;
		 int  h = getSize().height;
		 if (bufferEmpty)
		     backUpArea();

/*
		 if (w > pwidth+3 || h > pheight+3)
      		    setSize(pwidth+3, pheight+3);
*/
	      }
           });  
   
	   addMouseMotionListener(this);
	   addKeyListener(this);
	}
	public void mouseMoved(MouseEvent evt)
	{
	   if ((win_mode & MODIFY) != 0)
	   {
		toolpanel.mEvent(evt);
		return;
	   }
	   if (hilitRegion != null)
	   {
		modifier = evt.getModifiers();
		if (modifier == 0)
		    hilitRegion.mEvent(evt, HMOVE);
	   }
	}

	public void keyPressed(KeyEvent e)
	{
	}

	public void keyTyped(KeyEvent e)
	{
	}

	public void keyReleased(KeyEvent e)
	{
	    int     m, x, y;
	    int     k = e.getKeyCode();

	    x = 0;
	    y = 0;
	    m = 0;
	    if (k == KeyEvent.VK_LEFT)
	    {
		x = -1;
		m = 1;
	    }
	    if (k == KeyEvent.VK_RIGHT)
	    {
		x = 1;
		m = 2;
	    }
	    if (k == KeyEvent.VK_UP)
	    {
		y = -1;
		m = 3;
	    }
	    if (k == KeyEvent.VK_DOWN)
	    {
		y = 1;
		m = 4;
	    }
	    if (k == '\n')
	    {
		m = 5;
           	removeKeyListener(this);
	    }
	    if ((win_mode & MODIFY) != 0)
	    {
		toolpanel.keyMove(m);
		return;
	    }
	    if ((win_mode == 0) && (hilitRegion != null))
 	    {
		if (bimage == null)
		     return;
		Rectangle dim = hilitRegion.getDim();
		if (backRegion != hilitRegion)
		     backUpArea();
		if (snapFlag && (snapSpace > 1))
		{
		     x = dim.x + snapSpace * x;
		     y = dim.y + snapSpace * y;
		     x = snapAdjust(x, false);
		     y = snapAdjust(y, true);
		}
		else
		{
		     x = dim.x + x;
		     y = dim.y + y;
		     if (x < 0)
			x = 0;
		     if (y < 0)
			y = 0;
		}
		Graphics bgc =  getGraphics();
                bgc.drawImage(bimage, dim.x, dim.y, dim.x+dim.width+1, dim.y + dim.height+1, dim.x, dim.y, dim.x+dim.width+1, dim.y + dim.height+1, PlPanel.this);
                bgc.dispose();
		hilitRegion.move(x, y);
		if (m == 5)
		{
		    hilitRegion.setLocation(x, y);
		    clearHilit();
		}
	    }
	}


	public int getWidth()
	{
	    return pwidth;
	}

	public int getHeight()
	{
	    return pheight;
	}

	public Point getResolution()
	{
	    Point  pt = new Point();
	    pt.x = (int) (screen_res * paper_width);
	    pt.y = (int) (screen_res * paper_height);
	    return pt;
	}

	public void drawArea()
	{
	    if (new_w < 6 || new_h < 6)
		return;
	    Graphics g;

	    g =  getGraphics();
	    g.setXORMode(getBackground());
            g.setColor(hiColor);
            g.drawRect(new_x, new_y, new_w, new_h);
	    g.dispose();
	}

	public void mouseDragged(MouseEvent evt)
	{
	    int  x;
	    int  y;

	    if ((win_mode & MODIFY) != 0)
	    {
		toolpanel.mEvent(evt);
		return;
	    }
	    if ((win_mode & RESIZE) != 0)
	    {
		if (hilitRegion != null)
		{
		    modifier = evt.getModifiers();
		    if ((modifier & (1 << 4)) != 0)
		        hilitRegion.mEvent(evt, DRAG);
	        }
		return;
	    }
	    if ((win_mode & MOVE) != 0) {
		moveArea(evt, 1);
		return;
	    }
	    if ((win_mode & NEW) == 0)
		return;
	    x = evt.getX();
	    y = evt.getY();

	    if (new_w > 0 && new_h > 0)
		drawArea();
	    if (x > org_x)
	    {
                new_w = x - org_x;
		new_x = org_x;
	    }
            else
            {
            	new_w = org_x - x;
            	new_x = x;
            }
	    if (y > org_y)
	    {
                new_h = y - org_y;
		new_y = org_y;
	    }
            else
            {
            	new_h = org_y - y;
            	new_y = y;
            }
	    drawArea();
	}

	public void backUpArea()
        {
            if (bimage == null)
                return;

	    Graphics bgc = bimage.getGraphics();
	    Graphics2D bg2 = (Graphics2D) bgc;
	    bg2.setBackground(bgColor);
            bg2.clearRect(0, 0, pwidth, pheight);
            drawGrid(bg2);
            a_list.reset();
            Enumeration  e = a_list.elements();
            while (e.hasMoreElements())
            {
                DrawRegion  d = (DrawRegion)e.nextElement();
                if (d != hilitRegion)
                    d.regionDraw(bg2);
            }
            toolpanel.backUp(bg2);
            bgc.setColor(borderColor);
            bgc.drawRect(0, 0, pwidth+2, pheight+2);
            bgc.dispose();
	    backRegion = hilitRegion;
            bufferEmpty = false;
        }


	public void moveArea(MouseEvent mev, int mode)
	{
	    boolean todo = true;
	    boolean tomove = false;
	    int	     ev_id, x, y, dx, dy;
	    long     old_time = 0;
	    long   new_time, time_diff;

	    if (hilitRegion == null)
		return;
	    new_time = mev.getWhen();
	    time_diff = new_time - event_time;
	    event_time = new_time;
	    if (mode == 0) { // starting
	        moveRec = hilitRegion.getDim();
	        dif_x = mev.getX() - moveRec.x;
	        dif_y = mev.getY() - moveRec.y;
	        new_x = moveRec.x;
	        new_y = moveRec.y;
	        new_w = moveRec.width;
	        new_h = moveRec.height;
	        hilitRegion.setMoveMode(true);
		return;
	    }
	    x = mev.getX() - dif_x;
	    y = mev.getY() - dif_y;

	    tomove = false;
	    if (mode == 2) { // stop
		win_mode = 0;
	     	hilitRegion.setMoveMode(false);
      		setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
		if (snapFlag && (snapSpace > 1))
		{
		    x = snapAdjust(x, false);
		    y = snapAdjust(y, true);
		    if (x + new_w > pwidth)
			x = x - snapSpace;
		    if (y + new_h > pheight)
			y = y - snapSpace;
		    tomove = true;
		}
		if (!tomove) {
	     	    hilitRegion.regionDraw();
		    return;
		}
	    }

	    if (mode == 1) { // move
		if (x < 1)
		    x = 1;
	        else if (x + moveRec.width >= pwidth)
		    x = pwidth - moveRec.width - 1;
		if (y < 1)
		    y = 1;
		else if (y + moveRec.height >= pheight)
		    y = pheight - moveRec.height - 1;
		dx = x - new_x;
		dy = y - new_y;
		if (dx > 5 || dx < -5)
		    tomove = true;
		else if (dy > 5 || dy < -5)
		    tomove = true;
		else if (dx != 0 || dy != 0) {
		    new_time = mev.getWhen();
	            if (new_time - old_time > 200)
	            {
		   	tomove = true;
		   	old_time = new_time;
		    }
	        }
	   }
	   if (tomove) {
		recoverArea(new Rectangle(new_x, new_y, new_w, new_h));
		hilitRegion.move(x, y);
		new_x = x;
		new_y = y;
		hilitRegion.setLocation(new_x, new_y);
	    }
	}

	public DrawRegion findArea(int x, int y)
	{
	    Rectangle  dim;
	    DrawRegion cur_area;
	    DrawRegion dest_area;
	    int	       dx, dy, ndy, ndx, dx1, dx2, dy1, dy2;

      	    a_list.reset();
	    Enumeration  e = a_list.elements();
	    dx = 9999;
	    dy = 9999;
	    dest_area = null;
	    while (e.hasMoreElements())
	    {
		cur_area = (DrawRegion)e.nextElement();
		dim = cur_area.getRegion();
		if ((dim.x <= x) && (dim.y <= y))
		{
		    if ((x <= dim.width) && (y <= dim.height))
		    {
			dx1 = x - dim.x;
			dx2 = dim.width - x;
			if (dx1 > dx2)
			    ndx = dx2;
			else
			    ndx = dx1;
			dy1 = y - dim.y;
			dy2 = dim.height - y;
			if (dy1 > dy2)
			    ndy = dy2;
			else
			    ndy = dy1;
			if (ndy < ndx)
			    ndx = ndy;
			if (dx > ndx) {
			    dest_area = cur_area;
			    dx = ndx;
			}
			    
		    }
		}
	    }
	    return dest_area;
	}

	public void drawGrid(Graphics g)
	{
	    int    x, gap;
	    double x0;

	    if (gridFlag <= 0)
	  	return;
	    
	    if (snapGap < 4.0)
		gap = 4;
	    else
	        gap = (int)snapGap;
	    g.setPaintMode();
	    g.setColor(gridColor);
	    x0 = gap;
	    x = gap;
	    while (x < pwidth)
	    {
		g.drawLine(x, 1, x, pheight - 2);
		x0 += gap;
		x = (int) x0;
	    }
	    x0 = gap;
	    x = gap;
	    while (x < pheight)
	    {
		g.drawLine(1, x, pwidth - 2, x);
		x0 += gap;
		x = (int) x0;
	    }
	}

	public void drawGrid()
	{
	    Graphics gc =  getGraphics();
	    drawGrid(gc);
	    gc.dispose();
	}


	public void paint(Graphics g)
	{
	    super.paint(g);
	    paintAllItem(g);
 	}

	public Graphics getWinGraphics() {
	    return getGraphics();
	}

	public void paintAllItem(Graphics g)
	{
	    drawGrid(g);
      	    a_list.reset();
	    Enumeration  e = a_list.elements();
	    while (e.hasMoreElements())
	    {
		DrawRegion  d = (DrawRegion)e.nextElement();
	        if (d != hilitRegion)
		    d.regionPaint(g);
	    }
	    if (hilitRegion != null)
		hilitRegion.regionPaint(g);
	    toolpanel.show_items(g);
	    g.setColor(borderColor);
            g.drawRect(0, 0, pwidth+1, pheight+1);
	}

	public void paint()
	{
	    Graphics gc = getGraphics();
	    paintAllItem(gc);
	    gc.dispose();
        }

	private void bufferRecoverRegion(DrawRegion region, boolean redraw)
	{
	    if (region == null)
		return;
	    Rectangle dim = region.getDim();
	    int	   x2 = dim.x + dim.width + 1;
	    int	   y2 = dim.y + dim.height + 1;
	    Graphics fgc = getGraphics();
	    if (redraw) {
	        Graphics bgc = bimage.getGraphics();
		region.regionDraw(bgc);
		bgc.dispose();
	    }
            fgc.drawImage(bimage, dim.x, dim.y, x2, y2, dim.x, dim.y, x2, y2, 
		 	PlPanel.this);
	    fgc.dispose();
	}

	public void recoverRegion(DrawRegion region, boolean redraw)
	{
	    Rectangle  dim, dim2;
	    int	       x11, x12, x21, x22;
	    int	       y11, y12, y21, y22;
	    int	       rect_only;

	    if(bimage != null)
	    {
		bufferRecoverRegion(region, redraw);
		return;
	    }
	    region.clear();
	    dim = region.getDim();
	    drawGrid();
      	    a_list.reset();
	    Enumeration  e = a_list.elements();
	    x11 = new_x;
	    y11 = new_y;
	    x12 = new_x + new_w;
	    y12 = new_y + new_h;
	    while (e.hasMoreElements())
	    {
		DrawRegion  d = (DrawRegion)e.nextElement();
		if (d == region)
		    continue;
		dim2 = d.getDim();
		x21 = dim2.x;
		y21 = dim2.y;
		x22 = dim2.x + dim2.width+1;
		y22 = dim2.y + dim2.height+1;
		rect_only = 1;
		if ((x21 >= x11 && x21 <= x12) || (x22 >= x11 && x22 <= x12) ||
			(x21 <= x11 && x22 >= x22))
		{
		    if ((y21 >= y11 && y21 <= y12) || (y22 >= y11 && y22 <= y12)

			|| (y21 <= y11 && y22 >= y12))
		    {
			rect_only = 0;
			d.regionDraw();
		    }
	    	    if ((rect_only > 0) && (gridFlag > 0))
			d.outLine();
		}
	    }
	    toolpanel.recoverItems(dim);
	    if (redraw)
		region.regionDraw();
	}

	public void bufferRecoverArea(Rectangle dim)
	{
	    int	   x1 = dim.x - 2;
	    int	   y1 = dim.y - 2;
	    int	   x2 = dim.x + dim.width + 6;
	    int	   y2 = dim.y + dim.height + 6;

	    if (x1 < 0)
		x1 = 0;
	    if (y1 < 0)
		y1 = 0;
	    Graphics fgc = getGraphics();
            fgc.drawImage(bimage, x1, y1, x2, y2, x1, y1, x2, y2, null);
	    fgc.dispose();
	}

	public void recoverArea(Rectangle dim)
	{
	    Rectangle  dim2;
	    int	       x11, x12, x21, x22;
	    int	       y11, y12, y21, y22;
	    int	       rect_only;

	    if(bimage != null)
	    {
		bufferRecoverArea(dim);
		return;
	    }
	    gc.clearRect(dim.x, dim.y, dim.width, dim.height);
	    drawGrid();
      	    a_list.reset();
	    Enumeration  e = a_list.elements();
	    x11 = dim.x - 2;
	    y11 = dim.y - 2;
	    x12 = dim.x + dim.width + 6;
	    y12 = dim.y + dim.height + 6;
	    while (e.hasMoreElements())
	    {
		DrawRegion  d = (DrawRegion)e.nextElement();
		dim2 = d.getDim();
		x21 = dim2.x;
		y21 = dim2.y;
		x22 = dim2.x + dim2.width+1;
		y22 = dim2.y + dim2.height+1;
		rect_only = 1;
		if ((x21 >= x11 && x21 <= x12) || (x22 >= x11 && x22 <= x12) ||
			(x21 <= x11 && x22 >= x22))
		{
		    if ((y21 >= y11 && y21 <= y12) || (y22 >= y11 && y22 <= y12)

			|| (y21 <= y11 && y22 >= y12))
		    {
			rect_only = 0;
			d.regionDraw();
		    }
		}
	    	if ((rect_only > 0) && (gridFlag > 0))
		    d.outLine();
	    }
	}

	public void print(PlotConfig p)
	{
	    PrintJob pjob = getToolkit().getPrintJob(p, "Printing Test", null);
	    if (pjob != null)
            {  Graphics pg = pjob.getGraphics();
      	       a_list.reset();
	       Enumeration  e = a_list.elements();
           	if (pg != null)
           	{  
		   while (e.hasMoreElements())
		   {
                	DrawRegion  d = (DrawRegion)e.nextElement();
                	d.print(pg);
		   }
              	   pg.dispose(); // flush page
           	}
           	pjob.end();
	     }
         }
   
	public void clearHilit()
	{
	    if (hilitRegion != null)
	    {
		hilitRegion.select(false);
		recoverRegion(hilitRegion, true);
	    }
	    hilitRegion = null;
	    removeKeyListener(dp);
	}

	private void preview_image(DrawRegion region)
	{
		
	    String     pname, vret;
	    PrintWriter os;

	    pname = userName+"jplotp"+vid;
	    try 
	    {
		os = new PrintWriter(new FileWriter(tmpDirSystem+pname));
		if (os == null)
	  	   return;
		os.println(""+vcommand+" '-preview', 1)");	
      		if (direction == 2)  // Landscape
		    os.print(""+vcommand+" '-plotter', 'PS_AR', 10.0, 7.5");	
		else
		    os.print(""+vcommand+" '-plotter', 'PS_A', 7.5, 10.0");	
		if (colorPlotter)
		    os.println(", 1 )");
		else
		    os.println(", 0 )");
		os.println(""+vcommand+" '-preview', 1)");	
		os.print(""+vcommand+" '-screen', ");	
		Format.print(os, "%.2f", (double)screen_res);
		os.println(")");	
		region.outputTemplate(os, true);
		os.println(""+vcommand+" '-page', 0)");	
		os.close();
	    }
            catch(IOException er) { }

	    region.viewImage(PlotUtil.jplotImage(tmpDirUnix+pname, 1));
	    vid++;
	    if (vid > 60)
		vid = 1;
	}

	public void preview(boolean all)
	{
      	    setCursor(Cursor.getPredefinedCursor(Cursor.WAIT_CURSOR));
	    saveEditData();
	    if (all)
	    {
      	        a_list.reset();
	        Enumeration  e = a_list.elements();
		while (e.hasMoreElements())
		{
		    DrawRegion  d = (DrawRegion)e.nextElement();
		    preview_image(d);
		}
	    }
	    else if (hilitRegion != null) {
		 preview_image(hilitRegion);
	    }
	    paint();
      	    setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
	}

	public void vprint()
	{
	    int	   ret;
	    String fname = tmpDirUnix+userName+"jplot"+vid;
	    String fname2 = tmpDirSystem+userName+"jplot"+vid;
	    vid++;
	    if (vid > 60)
		vid = 1;
	    ret = saveTemplate (fname2, false);
	    if (ret > 0)
	        PlotUtil.jplotMacro("load", fname, fname, direction, 0);
	}

	public void selectRegion(MouseEvent evt)
	{
	    DrawRegion newHilitRegion = findArea(evt.getX(), evt.getY());
	    toolpanel.clearHighlight();
	    backRegion = null;
	    if (hilitRegion == newHilitRegion)
		return;
	    if (hilitRegion != null)
	    {
		hilitRegion.select(false);
                if (!bufferEmpty)
		   recoverRegion(hilitRegion, true);
	    }
	    hilitRegion = newHilitRegion;
	    if (hilitRegion != null)
	    {
		backUpArea();
		newHilitRegion.select(true);
		newHilitRegion.regionDraw();
		showPreference();
		requestFocus();
	        addKeyListener(dp);
	    }
	}

	public void delete()
	{
	    if (hilitRegion == null)
		return;
	    hilitRegion.select(false);
	    recoverRegion(hilitRegion, false);
	    if (deletedRegion != null)
		deletedRegion.delete();
	    deletedRegion = hilitRegion;
	    hilitRegion = null;
	    a_list.reset();
	    while (a_list.hasMoreElements())
	    {
		DrawRegion  d = (DrawRegion)a_list.currentElement();
		if (d == deletedRegion)
		{
		    a_list.remove();
		    d = null;
		    return;
		}
		a_list.nextElement();
	    }
	}

	public void undelete()
	{
	    if (deletedRegion == null)
	    {
		return;
	    }
	    deletedRegion.regionDraw();
	    a_list.append(deletedRegion);
	    deletedRegion = null;
	}

	public void delete_all()
	{
	    hilitRegion = null;
	    a_list.reset();
	    while (a_list.hasMoreElements())
	    {
		DrawRegion  d = (DrawRegion)a_list.currentElement();
		a_list.remove();
		d.delete();
		d = null;
	        a_list.reset();
	    }
	    toolpanel.delete_all();
	    deletedRegion = null;
	    repaint();
	    backUpArea();
	}

	public void clear()
	{
	    if (hilitRegion == null)
		return;
	    hilitRegion.clear();
	    hilitRegion.regionDraw();
	}

	public void changeResizeCursor(int  which)
	{
	    win_mode = RESIZE;
	    switch (which) {
	       case  0:
          		setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
			win_mode = 0;
			break;
	       case  1:
          		setCursor(Cursor.getPredefinedCursor(Cursor.W_RESIZE_CURSOR));
			break;
	       case  2:
          		setCursor(Cursor.getPredefinedCursor(Cursor.NW_RESIZE_CURSOR));
			break;
	       case  3:
          		setCursor(Cursor.getPredefinedCursor(Cursor.SW_RESIZE_CURSOR));
			break;
	       case  4:
          		setCursor(Cursor.getPredefinedCursor(Cursor.E_RESIZE_CURSOR));
			break;
	       case  5:
          		setCursor(Cursor.getPredefinedCursor(Cursor.NE_RESIZE_CURSOR));
			break;
	       case  6:
          		setCursor(Cursor.getPredefinedCursor(Cursor.SE_RESIZE_CURSOR));
			break;
	       case  7:
          		setCursor(Cursor.getPredefinedCursor(Cursor.N_RESIZE_CURSOR));
			break;
	       case  8:
          		setCursor(Cursor.getPredefinedCursor(Cursor.S_RESIZE_CURSOR));
			break;
	       case  9:
          		setCursor(Cursor.getPredefinedCursor(Cursor.WAIT_CURSOR));
			break;
	    }
	}

        public void showEditWindow()
	{
	    String  data;
	    Enumeration  e;
	    DrawRegion  d;

	    if (hilitRegion == null)
	    {
      	        a_list.reset();
	    	e = a_list.elements();
	    	if (e.hasMoreElements())
		{
		    hilitRegion = (DrawRegion)e.nextElement();
		    hilitRegion.select(true);
		    hilitRegion.regionDraw();
		}
		else
		    return;
	    }
	    data = hilitRegion.getData(1);
	    if (rd == null)
	    {
		rd = new RegionEdit(dp, data);
		rd.setLocation(screen_width / 2 - 200, 300);
	    }
	    else
		rd.setData(data);
	    rd.setVisible(true);
/*
	    rd.toFront();
*/
            rd.setState(Frame.NORMAL);
       }

	public void restoreEditData()
	{
	    if (hilitRegion == null)
		return;
	    String data = hilitRegion.getData(2);
	    rd.setData(data);
	}

	public void saveEditData()
	{
	    if (hilitRegion == null)
		return;
	    if ((rd == null) || (!rd.isVisible()))
		return;
	    String  data = rd.getData();
	    hilitRegion.setData(data);
	}

	public void saveEditData(DrawRegion region)
	{
	    if ((rd == null) || (!rd.isVisible()))
		return;
	    String  newdata = rd.getData();
	    region.setData(newdata);
	}


	public void setEditData(DrawRegion region)
	{
	    if ((rd == null) || (!rd.isVisible()))
		return;
	    String ndata = region.getData(1);
	    rd.setData(ndata);
	}


	public void openTemplate(String  fpath)
	{
	     String  inData;
	     String  name;
	     StringTokenizer tok;
	     BufferedReader inStream;
	     int     x, y, w, h;
	     double  d;
   	     DrawRegion nRegion;

	     regions = 0;
	     nRegion = null;
	     delete_all();
      	     toolpanel.setRatio(mag, pwidth, pheight);
	     
	     try
	     {
	       inStream = new BufferedReader(new FileReader(fpath));
	       if (inStream == null) {
		   repaint();
		   return;
		}
		
	while ((inData = inStream.readLine()) != null)
	{
	   if (inData.length() > 0)
	   {
		tok = new StringTokenizer(inData, " ,\n\t");
		name = tok.nextToken();
		if (name.equals(vcommand))
		{
		    name = tok.nextToken();
		    if (name.equals("'-draw'"))
		       toolpanel.addItem(inData);
		    else if (name.equals("'-region'"))
		    {
		    	name = tok.nextToken();
			if (name.equals("'start'"))
			{
		       	   d = Format.atof(tok.nextToken());
		       	   x = (int) (d * (double)pwidth);
		       	   d = Format.atof(tok.nextToken());
		       	   y = (int) (d * (double)pheight);
		       	   d = Format.atof(tok.nextToken());
		       	   w = (int) (d * (double)pwidth);
		       	   d = Format.atof(tok.nextToken());
		       	   h = (int) (d * (double)pheight);
		       	   regions++;
			   if (snapFlag && (snapSpace > 1))
			   {
			       x = snapAdjust(x, false);
			       w = snapAdjust(w, false);
			       y = snapAdjust(y, true);
			       h = snapAdjust(h, true);
			   }
			   if ((w > 5) && (h > 5))
			   {
			     nRegion = new DrawRegion(dp,regions, x, y, w, h);
			     nRegion.setColors(fgColor, borderColor, hiColor);
	  		     nRegion.setBorder(borderFlag);
			     nRegion.setPrefWindow(ipPopup);
			     nRegion.setRatio(mag);
			     a_list.append(nRegion);
			   }
/*
			   nRegion.inputTemplate(inStream);
*/
			}
			else if (name.equals("'end'"))
			   nRegion = null;
			else if (nRegion != null)
			{
			   nRegion.setAttr(inData);
		  	}
		    }
		    else if (name.equals("'-plotter'"))
		    {
		    	name = tok.nextToken();
			if (name.equals("'PS_AR'"))
			{
      			    if (direction == 1)  /* Portrait */
			    {
			 	direction = 2;
				set_size();
			    }
			}
			else if (name.equals("'PS_A'"))
			{
      			    if (direction == 2)  /* Landscape  */
			    {
			 	direction = 1;
				set_size();
			    }
			}
		    }
		    else if (name.equals("'-file'"))
		    {
		    	name = tok.nextToken();
			x = name.length();
			if (x > 2)
			   temp_name = name.substring(1, x - 1);
		    }
		}
		else if (nRegion != null)
		    nRegion.appendMacro(inData);
	   }
	}
	       inStream.close();
	     }
	     catch(IOException e)
	     {
		System.out.print("Error: " + e);
      	     }
	     repaint();
	}

	public void startMoveRegion(MouseEvent evt)
	{
	     win_mode = MOVE;
	     moveRec = hilitRegion.getDim();
	     dif_x = evt.getX() - moveRec.x;
	     dif_y = evt.getY() - moveRec.y;
	     new_x = moveRec.x;
	     new_y = moveRec.y;
	     new_w = moveRec.width;
	     new_h = moveRec.height;
             setCursor(Cursor.getPredefinedCursor(Cursor.MOVE_CURSOR));
	     moveArea(evt, 0);
	}

	public int saveTemplate(String fname, boolean saveTmpName) {
		String fpath;
		Enumeration e;
		DrawRegion d;
		PrintWriter outs;

		//if (fname.charAt(0) != '/') {
		if (!fname.startsWith(tmpDirSystem)) {
			if (userDir == null)
				return 0;
			fpath = userDir + "/templates/plot/" + fname;
		} else
			fpath = fname;
		try {
			outs = new PrintWriter(new FileWriter(fpath));
			if (outs == null)
				return 0;
			outs.println("\" Do NOT modify this file !!! \"");
			if (saveTmpName) {
				String t = fileList.getTemplateName();
				if (t != null)
					outs.println("" + vcommand + " '-file', '" + t + "' )");
				else if (temp_name != null)
					outs.println("" + vcommand + " '-file', '" + temp_name
							+ "' )");
			}
			if (direction == 2) // Landscape
				outs.print("" + vcommand
						+ " '-plotter', 'PS_AR', 10.0, 7.5");
			else
				outs.print("" + vcommand + " '-plotter', 'PS_A', 7.5, 10.0");
			if (colorPlotter)
				outs.println(", 1 )");
			else
				outs.println(", 0 )");
			outs.print("" + vcommand + " '-screen', ");
			Format.print(outs, "%.2f ", (double) screen_res);
			outs.println(")");
			a_list.reset();
			e = a_list.elements();
			while (e.hasMoreElements()) {
				d = (DrawRegion) e.nextElement();
				d.outputTemplate(outs, false);
			}
			toolpanel.output(outs);
			outs.println("" + vcommand + " '-page', 0 )");
			outs.close();
		} catch (IOException er) {
			System.out.print("Error: " + er);
			return 0;
		}
		return 1;
	}

	public void setDefaultName(String  fname)
	{
	     String  fpath;
	     PrintWriter outStream;

	     if (userDir == null)
                return;
             fpath = userDir+"/templates/plot/.default";
	     try
	     {
		outStream = new PrintWriter(new FileWriter(fpath));
		if (outStream == null)
		    return;
		outStream.println(""+fname);	
		outStream.close();
	     }
	     catch(IOException er)
	     {
		System.out.print("Error: " + er);
                return;
      	     }
	}

	public void setWindowMode (int   mode)
	{
	    win_mode = mode;
	}

	public void setModifyMode (boolean set)
	{
	     if (set)
	     {
		if (hilitRegion != null)
		    clearHilit();
		win_mode = MODIFY;
          	setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
	     }
	     else
	     {
		win_mode = 0;
      		setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
	     }

	}

	public void savePlotData (String fname)
	{
	    Enumeration  e;
	    DrawRegion  d;
	    PrintWriter os;

	    String pname = userName+"jplots"+vid;
	    try 
	    {
		os = new PrintWriter(new FileWriter(tmpDirSystem+pname));
		if (os == null)
	  	   return;
		os.println(""+vcommand+" '-save', '"+fname+"')");	
		String format = savePopup.dataFormat();
		os.println(""+vcommand+" '-format', "+format+")");	
		os.print(""+vcommand+" '-screen', ");	
		Format.print(os, "%.2f ", (double)screen_res);
		os.println(")");	

      		if (direction == 2)  // Landscape
		    os.print(""+vcommand+" '-plotter', 'PS_AR', 10.0, 7.5");	
		else
		    os.print(""+vcommand+" '-plotter', 'PS_A', 7.5, 10.0");	
		if (colorPlotter)
		    os.println(", 1 )");
		else
		    os.println(", 0 )");
		os.print(""+vcommand+" '-screen', ");	
		Format.print(os, "%.2f ", (double)screen_res);
		os.println(")");	

                a_list.reset();
                e = a_list.elements();
                while (e.hasMoreElements())
                {
                    d = (DrawRegion)e.nextElement();
                    d.outputTemplate(os, false);
                }
                toolpanel.output(os);
                os.println(""+vcommand+" '-page', 0 )");
		os.close();
	    }
            catch(IOException er) { return; }

	    PlotUtil.jplotMacro("load", tmpDirUnix+pname, tmpDirUnix+pname, direction, 0);
	    vid++;
	    if (vid > 60)
		vid = 1;
	}

	public int getScreenRes ()
	{
	     return screen_res;
	}

	public double getScaleRatio ()
	{
	     return mag;
	}

	public Color getWinForeground ()
	{
	     return fgColor;
	}

	public Color getWinBackground ()
	{
	     return bgColor;
	}

	public Color getHighlightColor ()
	{
	     return hiColor;
	}

        public void setPreference()
	{
	     DrawRegion d;
	     int	newx;
	     boolean    changed;
	     Rectangle  dim;

	     readPreference();
             this.setBackground(bgColor);
      	     this.setForeground(fgColor);
	     changed = false;
      	     a_list.reset();
      	     Enumeration  e = a_list.elements();
      	     while (e.hasMoreElements())
      	     {
	  	d = (DrawRegion)e.nextElement();
	  	d.setHilitColor(hiColor);
	  	d.setBorderColor(borderColor);
	  	d.setBorder(borderFlag);
      	     }
	     toolpanel.setHilitColor(hiColor);
	     if (snapFlag && (snapSpace > 1))
	     {
		oldSnapSpace = snapSpace;
		changed = true;
      	        a_list.reset();
      	        e = a_list.elements();
      	        while (e.hasMoreElements())
      	        {
	  	    d = (DrawRegion)e.nextElement();
		    dim = d.getDim();
		    dim.x = snapAdjust(dim.x, false);
		    dim.width = snapAdjust(dim.width, false);
		    dim.y = snapAdjust(dim.y, true);
		    dim.height = snapAdjust(dim.height, true);
		    d.changeSize(dim);
      	        }
      	     }
	     if (gridFlag != oldgridFlag)
	     {
		oldgridFlag = gridFlag;
		changed = true;
	     }
	     if (!gridName.equals(oldgridName))
	     {
		oldgridName = gridName;
		changed = true;
	     }
	     if (borderFlag != oldborderFlag)
	     {
		oldborderFlag = borderFlag;
		changed = true;
	     }
	     if (changed) {
		repaint();
		backUpArea();
	     }
	}

        public void setItemPreference()
	{
	    fgColor = ipPopup.getItemColor();
	    if (hilitRegion != null)
	    {
		hilitRegion.setPreference();
		preview(false);
	    }
	    toolpanel.setFgColor(fgColor);
	    toolpanel.setPreference();
	}

	public void showItemPreferences ()
	{
	    if (hilitRegion != null)
	    {
		hilitRegion.showPreference();
		return;
	    }
	    toolpanel.showPreference();
	}
   }


   private JPopupMenu popup = null;
   private JScrollPane sp;
   private PlPanel dp;
   private PlotEditTool toolpanel;
   private Rectangle  moveRec;
   private LinkedList  a_list = new LinkedList();
   private LinkedList  cmd_list = new LinkedList();
   private DrawRegion newRegion = null;
   private PlotMenuNode menuNode = null;
   private DrawRegion hilitRegion = null;
   private DrawRegion deletedRegion = null;
   private DrawRegion backRegion = null;
   private RegionEdit rd = null;
   private PlotFile fileList = null;
   private PlotSave savePopup = null;
   private PlotPref prefPopup = null;
   private PlotItemPref ipPopup = null;
   private PlotHelp helpPopup = null;
   private boolean waitMode = false;
   private Color borderColor = Color.gray;
   private Color gridColor = Color.gray;
   private Color fgColor = Color.black;
   private Color bgColor = Color.lightGray;
   private Color hiColor = Color.red;
   private boolean snapFlag = false;
   private boolean toWait = false;
   private int gridFlag = 0;
   private int oldgridFlag = 0;
   private int borderFlag = 1;
   private int oldborderFlag = 1;
   private boolean colorPlotter = false;
   private int snapSpace = 1;
   private int oldSnapSpace = 1;
   private double snapGap = 1.0;
   private String snapUnit = "inch";
   private String snapValue = "1";
   private int lineWidth = 1;
   private int pwidth;
   private int pheight;
   private int new_x, org_x;
   private int new_y, org_y;
   private int dif_x, dif_y;
   private int new_w;
   private int new_h;
   private int out_line;
   private int direction = 1;
   private int regions = 0;
   private int preview;
   private int region_act = 0;
   private int screen_width, screen_height, screen_res;
   private double paper_width, paper_height;
   private int border;
   private int vid = 1;
   private Graphics gc = null;
   private double mag = 1.0;
   private JMenuBar mbar;
   private int modifier;
   private String temp_name = null;
   private String hiName = "red";
   private String bgName = "lightGray";
   private String gridName = "gray";
   private String oldgridName = "gray";
   private String borderName = "gray";
   private static String userDir;
   private static String sysDir;
   private static String userName = null;
   private static String vnmrHost = null;
   private static int    vnmrId = -1;
   private static int    vnmrPort = -1;
   private static int    inPort = -1;
   private int win_mode = 0;
   private long event_time = 0;
   private static JFrame main_frame = null;
   private Image bimage = null;
   private PlotSocket inSocket = null;
   private Thread inThread;
   private ToVnmrSocket toVnmr;
   private String vnmrData;
   private Container cont;
   private boolean bufferEmpty = true;
   
   public static String tmpDirUnix;
   public static String tmpDirSystem;
}

