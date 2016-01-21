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

public class TextInput extends JFrame
   implements ItemListener, ActionListener 
{
   public TextInput(PlotEditTool caller)
   {
	cont = getContentPane();
	p_object = caller;
	ft = new Font("Monospaced", Font.BOLD, 14);
	setTitle("Text Input");
	setLocation(500, 32);
	cont.setLayout(new textLayout());

	inputField = new JTextField(32);
	cont.add(inputField);

	botpan = new JPanel();
	botpan.setLayout(new GridLayout(5,1));

	fmpan = new JPanel();
	fmpan.setLayout(new FlowLayout(FlowLayout.LEFT));
	fmpan.setFont(ft);
	fmpan.add(new JLabel("Font family: "));
	ButtonGroup fmGrp = new ButtonGroup();
	fmCheck1 = new JCheckBox("SansSerif", true);
	fmCheck2 = new JCheckBox("Serif", false);
	fmCheck3 = new JCheckBox("Monospaced", false);
	ActionListener act = new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                boolean flag = ((JCheckBox)e.getSource()).isSelected();
		if (flag)
                   updateFamily(e); 
	    }
	};
	fmCheck1.addActionListener(act);
	fmCheck2.addActionListener(act);
	fmCheck3.addActionListener(act);
	fmGrp.add((AbstractButton) fmCheck1);
	fmGrp.add((AbstractButton) fmCheck2);
	fmGrp.add((AbstractButton) fmCheck3);
	fmpan.add(fmCheck1);
	fmpan.add(fmCheck2);
	fmpan.add(fmCheck3);
	botpan.add(fmpan);

	stylepan = new JPanel();
	stylepan.setLayout(new FlowLayout(FlowLayout.LEFT));
	stylepan.setFont(ft);
	stylepan.add(new JLabel("Font style:  "));
	ButtonGroup styleGrp = new ButtonGroup();
	styleCheck1 = new JCheckBox("Bold", true);
	styleGrp.add((AbstractButton) styleCheck1);
	styleCheck2 = new JCheckBox("Italic", false);
	styleGrp.add((AbstractButton) styleCheck2);
	styleCheck3 = new JCheckBox("Plain", false);
	styleGrp.add((AbstractButton) styleCheck3);
	stylepan.add(styleCheck1);
	stylepan.add(styleCheck2);
	stylepan.add(styleCheck3);
	ActionListener al = new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                boolean flag = ((JCheckBox)e.getSource()).isSelected();
		if (flag)
		    updateStyle(e); 
	    }
	};
	styleCheck1.addActionListener(al);
	styleCheck2.addActionListener(al);
	styleCheck3.addActionListener(al);
	botpan.add(stylepan);

	sizepan = new JPanel();
	sizepan.setLayout(new FlowLayout(FlowLayout.LEFT));
	sizepan.setFont(ft);
	sizepan.add(new JLabel("Font size:   "));
	sizeField = new JTextField("16", 4);
	sizeField.setFont(ft);
	sizeField.addActionListener(new ActionListener()
	{
		public void actionPerformed(ActionEvent ev)
		{  updateSize(ev);  }
	});
	sizepan.add(sizeField);
	botpan.add(sizepan);
	
	butpan = new JPanel();
	butpan.setFont(ft);
	addButton("Paste", this);
	addButton("Cancel", this);
	addButton("Clear", this);
	addButton("Close", this);
	botpan.add(butpan);

	cont.add(botpan);
	fmStr = "SansSerif";
	styleStr = "Bold";
	sizeStr = "16";
	font_style = Font.BOLD;
	font_size = 16;
	wft = new Font(fmStr, font_style, font_size);
	inputField.setFont(wft);
	pack();	
	addWindowListener(new WindowAdapter() { public void
            windowClosing(WindowEvent e) { setVisible(false); } } );
   }

   public void addButton(String label, Object obj)
   {
	JButton but = new JButton(label);
	but.addActionListener((ActionListener) obj);
	butpan.add(but);
   }


   public void updateFont()
   {
	String newsize = sizeField.getText().trim();
        if (newsize.length() > 0)
	    sizeStr = newsize;
	else
	    sizeStr = "16";
	try {
            int a = Integer.parseInt(sizeStr);
	    font_size = a;
        }
        catch (NumberFormatException er) { return; }
                
	if (font_size > 100)
	{
	    font_size = 100;
	    sizeField.setText("100");
	}
	if (font_size < 6)
	    font_size = 6;
	wft = new Font(fmStr, font_style, font_size);
	inputField.setFont(wft);
	inputField.validate();
	validate();
   }

   public void updateFamily(ActionEvent  ev)
   {
	fmStr = (String) ((JCheckBox)ev.getSource()).getText();
	updateFont();
	inputField.requestFocus();
   }

   public void updateStyle(ActionEvent ev)
   {
	styleStr = (String) ((JCheckBox)ev.getSource()).getText();
	if (styleStr.equals("Bold"))
	    font_style = Font.BOLD;
	else if (styleStr.equals("Italic"))
	    font_style = Font.ITALIC;
	else
	    font_style = Font.PLAIN;
	updateFont();
	inputField.requestFocus();
   }

   public void updateSize(ActionEvent ev)
   {
	updateFont();
   }

   public void actionPerformed(ActionEvent  evt)
   {
	String cmd = evt.getActionCommand();
	if (cmd.equals("Close"))
	{
	    this.setVisible(false);
	    p_object.putText(false, null);
	    return;
	}
	if (cmd.equals("Paste"))
	{
	    String input = inputField.getText().trim();
            if (input.length() <= 0)
		 input = null;
	    p_object.putText(true, input);
	    return;
	}
	if (cmd.equals("Clear"))
	{
	    inputField.setText("");
	    return;
	}
	if (cmd.equals("Cancel"))
	{
	    p_object.putText(false, null);
	    return;
	}
   }

   public String toString()
   {
	String input = inputField.getText().trim();

        if (input.length() > 0)
	    return input;
	else
	    return null;
   }

   public Font  getFont()
   {
	return wft;
   }

   public FontMetrics  getFontMetrics()
   {
	FontMetrics  fm;
	fm = inputField.getFontMetrics(wft);
	return fm;
   }

   public void itemStateChanged(ItemEvent  evt)
   {
   }

   public String  getFontFamily()
   {
	return fmStr;
   }

   public void  setFontFamily(String s)
   {
	if (s == null || s.length() < 1)
	    return;
	fmStr = s;;
	if (s.equals("SansSerif"))
	    fmCheck1.setSelected(true);
	else if (s.equals("Serif"))
	    fmCheck2.setSelected(true);
	else
	    fmCheck3.setSelected(true);
   }

   public int  getFontStyle()
   {
        return font_style;
   }


   public void  setFontStyle(int n)
   {
	font_style = n;
	if (font_style == Font.BOLD)
	    styleCheck1.setSelected(true);
	else if (font_style == Font.ITALIC)
	    styleCheck2.setSelected(true);
	else
	    styleCheck3.setSelected(true);
   }

   public int  getFontSize()
   {
        return font_size;
   }

   public void  setFontSize(int n)
   {
	font_size = n;
	String s = ""+n;
	sizeField.setText(s);
   }

   public void  setData(String s)
   {
	inputField.setText(s);
   }

/*
   public void setForeground(Color c)
   {
        fg = c;
	inputField.setForeground(fg);
   }
*/

   public void setTextColor(Color c)
   {
	inputField.setForeground(c);
   }

   class textLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

        public Dimension preferredLayoutSize(Container target) {
            Dimension dim;
            int   w = 0;
            int   h = 4;
            int   n = target.getComponentCount();
            for (int k = 0; k < n; k++) {
                Component m = target.getComponent(k);
                dim = m.getPreferredSize();
                if (dim.width > w)
                    w = dim.width;
                h += dim.height;
            }
            dim = null;
            return new Dimension(w, h); // unused
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        }

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension dim0 = target.getSize();
                Dimension dim1 = inputField.getPreferredSize();
                int     y = 4;
                inputField.setBounds(2, y, dim0.width-2, dim1.height);
		y += dim1.height;
                dim1 = botpan.getPreferredSize();
                botpan.setBounds(0, y, dim0.width, dim1.height);
                inputField.repaint();
            }
        }
   }



   private JPanel botpan;
   private JPanel butpan;
   private JPanel fmpan, stylepan, sizepan;
   private JCheckBox fmCheck1, fmCheck2, fmCheck3;
   private JCheckBox styleCheck1, styleCheck2, styleCheck3;
   private JTextField sizeField;
   private JTextField inputField;
   private String fmStr;
   private String styleStr;
   private String sizeStr;
   private Font  ft;
   private Font  wft;
   private Color fg;
   private int  font_style, font_size;
   private PlotEditTool  p_object;
   private Container cont;
}

