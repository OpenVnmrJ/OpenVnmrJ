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

public class PlotItemPref extends JFrame
   implements  ActionListener, AdjustmentListener, ItemListener
{
   public PlotItemPref(PlotConfig.PlPanel parent, String usrdir)
   {
	p_object = parent;
	userDir = usrdir;
	ft = new Font("Monospaced", Font.BOLD, 16);
	setTitle("Item Preferences");
	setLocation(300, 30);

	pan = new JPanel();
	pan.setLayout(new GridLayout(3, 1));
	pan.setFont(ft);

	JPanel fgpan = new JPanel();
	fgpan.setLayout(new FlowLayout(FlowLayout.LEFT));
	fgpan.add(new JLabel("Color(RGB):"));
	redLabel = new JTextField("0", 5);
	greenLabel = new JTextField("0", 5);
	blueLabel = new JTextField("0", 5);
	colorChoice = new JComboBox();
	build_color_choice(colorChoice);
	colorChoice.setActionCommand("setcolor");
	colorChoice.addActionListener(new ActionListener() {
               public void actionPerformed(ActionEvent evt) {
                   changeColor(evt);
               }
        });
       
	fgpan.add(redLabel);
	fgpan.add(greenLabel);
	fgpan.add(blueLabel);
	fgpan.add(colorChoice);

	ActionListener colorChanged = new ActionListener()
        {
                public void actionPerformed(ActionEvent ev)
                {  updateColor(ev);  }
        };
	redLabel.addActionListener (colorChanged);
	greenLabel.addActionListener (colorChanged);
	blueLabel.addActionListener (colorChanged);

	JPanel ftpan = new JPanel();
	ftpan.setLayout(new FlowLayout(FlowLayout.LEFT));
	ftpan.add(new JLabel("Font:      "));
	fnameChoice = new JComboBox();
	fnameChoice.addItem("Monospaced");
	fnameChoice.addItem("SansSerif");
	fnameChoice.addItem("Serif");
        ftpan.add(fnameChoice);
	ftypeChoice = new JComboBox();
	ftypeChoice.addItem("Bold");
	ftypeChoice.addItem("Italic");
	ftypeChoice.addItem("Plain");
        ftpan.add(ftypeChoice);
	fsize = new JTextField("12", 6);
        ftpan.add(fnameChoice);
        ftpan.add(ftypeChoice);
        ftpan.add(fsize);

	JPanel linepan = new JPanel();
	linepan.setLayout(new FlowLayout(FlowLayout.LEFT));
	linepan.add(new JLabel("Line Width:"));
	lineText = new JTextField("1", 6);
	linepan.add(lineText);
	ActionListener al = new  ActionListener() {
                public void actionPerformed(ActionEvent ev) {
                    apply_proc();
		}
            };

	lineText.addActionListener(al);
	fsize.addActionListener(al);

	pan.add(fgpan);
	pan.add(linepan);
	pan.add(ftpan);

	canvas = new JPanel();
        canvas.setBackground(color);
        canvas.setSize(300, 60);

	cpan = new JPanel();
	cpan.setLayout(new GridLayout(4, 1));
	cpan.setFont(ft);

	JPanel rpan = new JPanel();
	rpan.setLayout(new BorderLayout());
        rpan.add(new JLabel("Red:  "), "West");
        redBar = new JScrollBar(JScrollBar.HORIZONTAL, 0, 0, 0, 255);
        rpan.add(redBar, "Center");
        redBar.setBlockIncrement(16);
        redBar.addAdjustmentListener(this);

	JPanel gpan = new JPanel();
	gpan.setLayout(new BorderLayout());
        gpan.add( new JLabel("Green:"), "West");
        greenBar = new JScrollBar(JScrollBar.HORIZONTAL, 0, 0, 0, 255);
        gpan.add(greenBar, "Center");
        greenBar.setBlockIncrement(16);
        greenBar.addAdjustmentListener(this);

	JPanel bpan = new JPanel();
	bpan.setLayout(new BorderLayout());
        bpan.add(new JLabel("Blue: "), "West");
        blueBar = new JScrollBar(JScrollBar.HORIZONTAL, 0, 0, 0, 255);
        bpan.add(blueBar, "Center");
        blueBar.setBlockIncrement(16);
        blueBar.addAdjustmentListener(this);

	butpan = new JPanel();
	butpan.setFont(ft);
	addButton("Apply", this);
	addButton("Close", this);

	cpan.add(rpan);
	cpan.add(gpan);
	cpan.add(bpan);
	cpan.add(butpan);

	Container cont = getContentPane();
	cont.add(pan, BorderLayout.NORTH);
	cont.add(canvas, BorderLayout.CENTER);
	cont.add(cpan, BorderLayout.SOUTH);
	
	showItemColor(color);
	pack();	
	addWindowListener(new WindowAdapter() {
	    public void windowDeiconified(WindowEvent e) { openPref();  }
	    public void windowOpened(WindowEvent e) { openPref();  }
	    public void windowClosing(WindowEvent e) { setVisible(false); } } );
   }

   public void changeColor(ActionEvent  evt)
   {
	String cmd = evt.getActionCommand();
	if (cmd.equals("setcolor")) {
	   JComboBox combo = (JComboBox)evt.getSource();
           String s = (String)combo.getSelectedItem();
	   if (s.equals("custom")) {
                isCustom = true;
                return;
           }
           isCustom = false;
           color = PlotRgb.getColorByName(s);
	   bChanging = true;
           showItemColor(color);
	   bChanging = false;
        }
   }

   private void openPref()
   {
	p_object.showItemPreferences();
   }

   public void itemStateChanged(ItemEvent evt)
   {
	Color  c;
	if (evt.getStateChange() == ItemEvent.SELECTED)
	{
            String s = (String) evt.getItem();
	    if (s.equals("custom"))
	    {
		isCustom = true;
		return;
	    }
	    isCustom = false;
	    c = PlotRgb.getColorByName(s);
	    showItemColor(c);
	}
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

   private void apply_proc()
   {
	p_object.setItemPreference();
   }

   public Color getItemColor()
   {
	return color;
   }

   public int getItemLineWidth()
   {
	String data = lineText.getText().trim();
	if ((data == null) || (data.length() < 1))
	     lineWidth = 1;
	else
	     lineWidth = Format.atoi(data);
	return (lineWidth);
   }

   public void showItemColor(Color c)
   {
	int	k, r, g, b;
	String  d;

	color = c;
	r = color.getRed();
	g = color.getGreen();
	b = color.getBlue();
	canvas.setBackground(color);
        canvas.repaint();
	redLabel.setText(Integer.toString(r));
	greenLabel.setText(Integer.toString(g));
	blueLabel.setText(Integer.toString(b));
	redBar.setValue(r);
	greenBar.setValue(g);
	blueBar.setValue(b);
   }

   public void setItemColor(Color c)
   {
	isCustom = true;
	showItemColor(c);
	colorChoice.setSelectedItem("custom");
   }

   public void setItemLineWidth(int w)
   {
	lineText.setText(Integer.toString(w));
   }

   public void setItemLineWidth(String len)
   {
	lineText.setText(len);
   }

   public void setFontChoice(String name, int style, int size)
   {
	if (name != null)
	   fnameChoice.setSelectedItem(name);
	setFontStyle (style);
	setFontSize (size);
   }

   public void setFontName(String name)
   {
	if (name != null)
	   fnameChoice.setSelectedItem(name);
   }

   public void setFontStyle(String name)
   {
	if (name != null)
	   ftypeChoice.setSelectedItem(name);
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
        c.addItem("custom");
	color = PlotRgb.getColorByName("black");
	colorChoice.setSelectedItem("black");
   }

   public void setFontStyle(int v)
   {
	if (v == Font.ITALIC)
	   ftypeChoice.setSelectedItem("Italic");
	else if (v == Font.BOLD)
	   ftypeChoice.setSelectedItem("Bold");
	else
	   ftypeChoice.setSelectedItem("Plain");
   }

   public void setFontSize(String name)
   {
	if (name != null)
	   fsize.setText(name);
   }

   public void setFontSize(int v)
   {
	fsize.setText(Integer.toString(v));
   }

   public String getFontName()
   {
	return((String)fnameChoice.getSelectedItem());
   }

   public int getFontStyle()
   {
	String s = (String)ftypeChoice.getSelectedItem();
	if (s.equals("Italic"))
	   return (Font.ITALIC);
	if (s.equals("Bold"))
	   return (Font.BOLD);
	else
	   return (Font.PLAIN);
   }

   public int getFontSize()
   {
	int	k;
	String d = fsize.getText().trim();
	if ((d == null) || (d.length() < 1))
	    k = 12;
	else
	{
	    k = Format.atoi(d);
	    if (k < 0)
		k = 12;
	}
	return k;
   }

   private int get_color_value(JTextField tx)
   {
	int	k;
	String d = tx.getText().trim();
	if ((d == null) || (d.length() < 1))
	    k = 0;
	else
	{
	    k = Format.atoi(d);
	    if (k < 0)
		k = 0;
	    else if (k > 255)
		k = 255;
	}
	return k;
   }

   public void updateColor(ActionEvent ev)
   {
	int	r, g, b;

	r = get_color_value(redLabel);
	g = get_color_value(greenLabel);
	b = get_color_value(blueLabel);
	if (r > 255)
	   r = 255;
	if (g > 255)
	   g = 255;
	if (b > 255)
	   b = 255;
	color = new Color(r, g, b);
        bChanging = true;
	redBar.setValue(r);
	greenBar.setValue(g);
	blueBar.setValue(b);
        bChanging = false;
	canvas.setBackground(color);
        canvas.repaint();
	colorChoice.setSelectedItem("custom");
   }

   public void adjustmentValueChanged(AdjustmentEvent evt)
   {  
	int	r, g, b;

        
        if (bChanging)
	    return;
	r = redBar.getValue();
	g = greenBar.getValue();
	b = blueBar.getValue();
	redLabel.setText("" + r);
        greenLabel.setText("" + g);
	blueLabel.setText("" + b);
	color = new Color(r, g, b);
	canvas.setBackground(color);
        canvas.repaint();

	if (!isCustom)
	{
	    if ((r != redNum) || (g != greenNum) || (b != blueNum))
	    {
		colorChoice.setSelectedItem("custom");
		isCustom = true;
	    }
	}
   }

   private JPanel pan;
   private JPanel cpan;
   private PlotConfig.PlPanel p_object;
   private JPanel butpan;
   private JTextField  lineText;
   private Font  ft;
   private String  userDir;
   private int lineWidth = 1;
   private int redNum, greenNum, blueNum;
   private boolean isCustom = false;
   private boolean bChanging = false;
   private JScrollBar redBar;
   private JScrollBar greenBar;
   private JScrollBar blueBar;
   private Color     color;
   private JPanel canvas;
   private JComboBox fnameChoice, ftypeChoice;
   private JComboBox colorChoice;
   private JTextField fsize;
   private JTextField redLabel;
   private JTextField greenLabel;
   private JTextField blueLabel;

}

