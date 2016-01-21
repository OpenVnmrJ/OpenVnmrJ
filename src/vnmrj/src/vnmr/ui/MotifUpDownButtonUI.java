/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.awt.event.*;
import java.awt.font.FontRenderContext;
import java.awt.font.TextLayout;
import java.awt.geom.Rectangle2D;

import javax.swing.*;
import javax.swing.plaf.*;
import javax.swing.border.*;

import vnmr.util.*;
import vnmr.bo.*;

public class MotifUpDownButtonUI extends UpDownButtonUI
    implements MouseListener, MouseMotionListener {

    protected Timer repeatTimer;
    protected TimerActionListener timerActionListener;

    private boolean isPointy = false;
    private int maxMargin = 20;
    private int maxSlope = 2;
    private int lastValue = 0;
    private boolean controlKey = false;
    private UpDownButton button = null;
    private UpDownIcon icon = null;
    private JTextField typeIn = new JTextField("");
    private ButtonActionListener actionListener = null;
    private Rectangle valueRect = new Rectangle(); // Location of value string
    private Rectangle incrRect = new Rectangle(); // Location of incr string

    public MotifUpDownButtonUI() {
    }

    public static ComponentUI createUI(JComponent c) {
	return new MotifUpDownButtonUI();
    }

    public void installUI(JComponent c) {
	//System.out.println("installUI()");/*CMP*/
	UpDownButton b = (UpDownButton)c;
	button = b;
	b.addMouseListener(this);
	b.addMouseMotionListener(this);
	actionListener = new ButtonActionListener();
	b.addActionListener(actionListener);
	b.setLayout(null);
	b.add(typeIn);
	typeIn.setBackground(Color.white);
	typeIn.setFont(b.getFont());
	typeIn.setVisible(false);
	typeIn.addActionListener(new TextActionListener());

	timerActionListener = new TimerActionListener();
	//System.out.println("timerListener=" + timerActionListener);/*CMP*/
	repeatTimer = new Timer(50, timerActionListener);
	repeatTimer.setInitialDelay(300);

	icon = new UpDownIcon(b);
    }

    public void uninstallUI(JComponent c) {
	UpDownButton b = (UpDownButton)c;
	repeatTimer.stop();
	repeatTimer = null;
	b.removeMouseListener(this);
	b.removeMouseMotionListener(this);
	b.removeActionListener(actionListener);
    }

    public boolean contains(JComponent jc, int x, int y) {
	if (x < 0 || y < 0) {
	    return false;
	}
	UpDownButton b = (UpDownButton)jc;
	Insets insets = b.getInsets();
	int w = b.getWidth() - insets.left - insets.right;
	int h = b.getHeight() - insets.top - insets.bottom;
	if (x >= w || y >= h) {
	    return false;
	}
	this.isPointy = b.isPointy();
	int[] margin = getMargins(h);
	int dx = margin[0];
	int dy = margin[1];
	Polygon poly = new Polygon();
	poly.addPoint(0, h-1-dy);
	poly.addPoint(0, dy);
	poly.addPoint(dx, 0);
	poly.addPoint(w-1-dx, 0);
	poly.addPoint(w-1, dy);
	poly.addPoint(w-1, h-1-dy);
	poly.addPoint(w-1-dx, h-1);
	poly.addPoint(dx, h-1);
	return poly.contains(x, y);
    }

    public void paint(Graphics g, JComponent c) {
	//System.out.println("UpDownButton.paint(): bounds="
	//		   + c.getBounds());/*CMP*/
	UpDownButton b = (UpDownButton)c;
	IntegerRangeModel dataModel = b.getDataModel();
	paintBezel(g, b);
	icon.paintIcon(b, g, 0, 0);
    }

    /**
     * paint the Motif style bezel
     */
    protected void paintBezel(Graphics g1, UpDownButton b) {
	Graphics2D g = (Graphics2D)g1;
	//g.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
	//		   RenderingHints.VALUE_ANTIALIAS_ON);
	Insets insets = b.getInsets();
	int w = b.getWidth() - insets.left - insets.right;
	int h = b.getHeight() - insets.top - insets.bottom;
	//g.setColor(Color.black);
	//g.drawRect(0, 0, w-1, h-1);
	Color highlightColor = b.getBackground().brighter().brighter();
	Color shadowColor = b.getBackground().darker().darker();
	Color disabledColor = b.getBackground().darker();
	Color depressedColor = Util.changeBrightness(b.getBackground(), -10);
	this.isPointy = b.isPointy();
	int[] margin = getMargins(h);
	int dx = margin[0];
	int dy = margin[1];
	boolean isPressed1 = false;
	boolean isPressed2 = false;
	boolean isPressed3 = false;
	boolean hasFocus = false;
	if (! (b.getStateModel() instanceof ThreeButtonModel) ) {
	    return;
	}
	ThreeButtonModel model = b.getStateModel();
	isPressed1 = model.isArmed(1) && model.isPressed(1);
	isPressed2 = model.isArmed(2) && model.isPressed(2);
	isPressed3 = model.isArmed(3) && model.isPressed(3);
	boolean isPressed = isPressed1 || isPressed2 || isPressed3;
	/*hasFocus = ((model.isArmed() && isPressed)
	  || (b.isFocusPainted() && b.hasFocus()));*/

	g.translate(insets.left, insets.top);
	Color oldColor = g.getColor();
	boolean isRocker = b.isRocker();

	/*
	 * Draw the background
	 */
	if (model.isEnabled() && isPressed) {
	    if (isRocker && isPressed1) {
		g.setPaint(new GradientPaint(new Point(0, 0),
					     depressedColor,
					     new Point(w-1, 0),
					     b.getBackground()
					     ));
	    } else if (isRocker && isPressed3) {
		g.setPaint(new GradientPaint(new Point(w-1, 0),
					     depressedColor,
					     new Point(0, 0),
					     b.getBackground()
					     ));
	    } else {
		g.setColor(depressedColor);
	    }
	    Polygon poly = new Polygon();
	    poly.addPoint(0, h-1-dy);
	    poly.addPoint(0, dy);
	    poly.addPoint(dx, 0);
	    poly.addPoint(w-1-dx, 0);
	    poly.addPoint(w-1, h-1-dy);
	    poly.addPoint(w-1, dy);
	    poly.addPoint(w-1-dx, h-1);
	    poly.addPoint(dx, h-1);
	    g.fillPolygon(poly);
	}

	/*
	 * Draw an arrow, if requested
	 */
	if ((isPressed1 || isPressed3) && model.isEnabled() && b.isArrow()) {
	    // Draw arrow
	    int awidth = (w/10) * 2 + 1;
	    drawArrow(g, b, w/2-awidth/2, 1, awidth, h-2,
		      isPressed1 ? "down" : "up");
	}

	/*
	 * Draw the boundary
	 */
	if (!model.isEnabled()) {
	    g.setColor(disabledColor);
	} else  if (!isPressed) {
	    g.setColor(highlightColor);
	} else if (isRocker && isPressed1) {
	    g.setPaint(new GradientPaint(new Point(0, 0), shadowColor,
					 new Point(w-1, 0), highlightColor));
	} else if (isRocker && isPressed3) {
	    g.setPaint(new GradientPaint(new Point(0, 0), highlightColor,
					 new Point(w-1, 0), shadowColor));
	} else {
	    g.setColor(shadowColor);
	}
	g.drawLine(0, dy, 0, h-1-dy); // Left side
	g.drawLine(0, dy, dx, 0); // Top-left slant
	g.drawLine(dx+1, 0, w-dx-2, 0); // Top
	g.drawLine(w-dx-1, 0, w-1, dy); // Top-right slant

	if (!model.isEnabled()) {
	    // Color is already OK
	} else if (!isPressed) {
	    g.setColor(shadowColor);
	} else if (isRocker && isPressed3) {
	    g.setPaint(new GradientPaint(new Point(0, 0), shadowColor,
					 new Point(w-1, 0), highlightColor));
	} else if (isRocker && isPressed1) {
	    g.setPaint(new GradientPaint(new Point(0, 0), highlightColor,
					 new Point(w-1, 0), shadowColor));
	} else {
	    g.setColor(highlightColor);
	}
	g.drawLine(w-1, dy, w-1, h-1-dy); // Right side
	g.drawLine(w-dx-1, h-1, w-1, h-1-dy); // Bottom-right slant
	g.drawLine(dx+1, h-1, w-dx-2, h-1);	// Bottom
	g.drawLine(0, h-1-dy, dx, h-1); // Bottom-left slant

	g.setColor(oldColor);
	g.translate(-insets.left, -insets.top);
    }


    public void mousePressed(MouseEvent e) {
	if (!e.isShiftDown()) {
	    UpDownButton b = (UpDownButton)e.getComponent();
	    ThreeButtonModel m = b.getStateModel();
	    int n = getButtonNumber(e);
	    if ( !m.isPressed() && !repeatTimer.isRunning()) {
		m.setPressed(true, n);
		m.setArmed(true, n);
	    }
	    if ((n == 1 || n == 3) && !repeatTimer.isRunning()) {
		timerActionListener.setDirection(n);
		repeatTimer.start();
	    }
	}
    }

    public void mouseReleased(MouseEvent e) {
	UpDownButton b = (UpDownButton)e.getComponent();
	ThreeButtonModel m = b.getStateModel();
	int n = getButtonNumber(e);
	repeatTimer.stop();
	if (m.isEnabled() && e.isShiftDown()) {
	    //System.out.println("Popup: button #" + n);/*CMP*/
	    if (n == 1 || n == 3) {
		typeIn.setText(button.getValueStr());
		typeIn.setActionCommand("value");
		typeIn.setBounds(valueRect);
	    } else if (n == 2) {
		typeIn.setText(button.getIncrementStr());
		typeIn.setActionCommand("incr");
		typeIn.setBounds(incrRect);
	    }
	    typeIn.setVisible(true);
	    typeIn.grabFocus();
	    typeIn.selectAll();
	    //typeIn.setSelectionColor(Color.yellow);
	    m.setEnabled(false);
	} else {
	    if (n == 2) {
	       if ( e.isControlDown() ) {
                  controlKey = true;
               } else {
                  controlKey = false;
               }
            }
	    m.setPressed(false, n);
	    m.setArmed(false, n); // Do this last
	}
    }

    public void mouseEntered(MouseEvent e) { }
    public void mouseExited(MouseEvent e) { }
    public void mouseClicked(MouseEvent e) { }
    public void mouseMoved(MouseEvent e) { }

    public void mouseDragged(MouseEvent e) {
	UpDownButton b = (UpDownButton) e.getSource();

	// HACK! We're forced to do this since mouseEnter and mouseExit aren't
	// reported while the mouse is down.
	ButtonModel model = b.getStateModel();

	if(model.isPressed()) {
            Rectangle tmpRect = new Rectangle();
	    tmpRect.width = b.getWidth();
            tmpRect.height = b.getHeight();
            if(tmpRect.contains(e.getPoint())) {
                model.setArmed(true);
            } else {
                model.setArmed(false);
            }
        }
    };

    private int[] getMargins(int height) {
	int dx;
	int dy;
	int[] margin = new int[2];
	if (this.isPointy) {
	    dy = height /2 ;
	    dx = dy > maxMargin ? maxMargin : dy;
	    margin[0] = dx;
	    margin[1] = dy;
	} else {
	    margin[0] = 2;
	    margin[1] = 2;
	}
	return margin;
    }

    private Insets getInsets(int height) {
	int[] margin = getMargins(height);
	Insets insets = new Insets(2, margin[0], 2, 5);
	return insets;
    }

    private int getButtonNumber(MouseEvent e) {
	int n = 0;
	int bits = e.getModifiers();
	//System.out.println("Button mask = " + bits);/*CMP*/
	if ((bits & InputEvent.BUTTON3_MASK) != 0) { n = 3; }
	if ((bits & InputEvent.BUTTON2_MASK) != 0) { n = 2; }
	if ((bits & InputEvent.BUTTON1_MASK) != 0) { n = 1; }
	return n;
    }

    private void drawArrow(Graphics g, UpDownButton b,
			   int xpos, int ypos, int width, int height,
			   String direction) {
	int x1, x2, x3, y1, y2, y3, w, h;

	g.setColor(b.getArrowColor());
	// Draw head
	x1 = xpos;
	x2 = xpos + width;
	x3 = xpos + width/2;
	if (direction.equals("down")){
	    y1 = y2 = ypos + height - height/3;
	    y3 = ypos + height;
	} else {
	    y1 = y2 = ypos + height/3 + 1;
	    y3 = ypos;
	}
	Polygon poly = new Polygon();
	poly.addPoint(x1, y1);
	poly.addPoint(x2, y2);
	poly.addPoint(x3, y3);
	g.fillPolygon(poly);
	
	// Draw shaft
	h = height - height/3;
	w = (width/4) * 2 + 1;
	x1 = x3 - w/2;
	y1 = direction.equals("down") ? ypos : y1;
	g.fillRect(x1, y1, w, h);
    }

    /**
     * Icon (labels) for UpDownButton
     */
    class UpDownIcon implements Icon {

	private UpDownButton myButton; // Parent component

	/**
	 * constructor
	 * @param button parent component
	 */
	public UpDownIcon(UpDownButton button) {
	    myButton = button;
	}

	/**
	 * paint icon
	 */
	public void paintIcon(Component c, Graphics g, int x, int y) {
	    UpDownButton b = (UpDownButton)c;
	    String valueStr = b.getValueStr();
	    //String incrStr = "-" + myButton.getIncrementStr() + "+";
	    String incrStr = (char)0xb1 + b.getIncrementStr();
	    String label = b.getLabel();
	    if (label == null || label.length() == 0) {
		label = b.getName();
	    }
	    Insets insets = getInsets(c.getHeight());

	    Graphics2D g2 = (Graphics2D)g;
	    FontRenderContext frc = g2.getFontRenderContext();

	    Font font = g.getFont();
	    //g.setFont(font);

	    String minVal = Integer.toString(b.getMinimum());
	    String maxVal = Integer.toString(b.getMaximum());

	    boolean rtnflag = false;
	    if (label == null) {
		System.out.println("label is null");
		rtnflag = true;
	    }
	    if (valueStr == null) {
		System.out.println("valueStr is null");
		rtnflag = true;
	    }
	    if (incrStr == null) {
		System.out.println("incrStr is null");
		rtnflag = true;
	    }
	    if (minVal == null) {
		System.out.println("minVal is null");
		rtnflag = true;
	    }
	    if (maxVal == null) {
		System.out.println("maxVal is null");
		rtnflag = true;
	    }
	    if (rtnflag) {
		return;
	    }

	    TextLayout t1 = new TextLayout(label, font, frc);
	    TextLayout t2 = new TextLayout(valueStr, font, frc);
	    TextLayout t3 = new TextLayout(incrStr, font, frc);

	    Rectangle2D b1 = t1.getBounds();
	    Rectangle2D b2 = t2.getBounds();
	    Rectangle2D b3 = t3.getBounds();
	    Rectangle2D b2min = new TextLayout(minVal, font, frc).getBounds();
	    Rectangle2D b2max = new TextLayout(maxVal, font, frc).getBounds();

	    y = insets.top - (int)b1.getY();
	    x = insets.left;
	    b1.setRect(b1.getX() + x, b1.getY() + y,
		       b1.getWidth(), b1.getHeight());
	    //g.setColor(Color.yellow);
	    //g2.fill(b1);
	    if (myButton.isEnabled()) {
		g.setColor(c.getForeground());
	    } else {
		g.setColor(myButton.getBackground().darker());
	    }
	    t1.draw(g2, x, y);

	    y = c.getHeight() - insets.bottom
		- (int)(b2.getHeight() + b2.getY());
	    b2.setRect(b2.getX() + x, b2.getY() + y,
		       b2.getWidth(), b2.getHeight());
	    Insets txtInsets = typeIn.getInsets();
	    int txtYPad = txtInsets.top + txtInsets.bottom + 2;
	    int txtXPad = txtInsets.left + txtInsets.right;
	    int vh = (int)b2.getHeight() + txtYPad;
	    // Remember where this is for direct type-in of value
	    double w = Math.max(b2max.getWidth(), b2min.getWidth());
	    valueRect.setRect(b2.getX(), c.getHeight() - vh,
			      w + txtXPad, vh);
	    //g.setColor(Color.yellow);
	    //g2.fill(b2);
	    t2.draw(g2, x, y);

	    y = (int)(c.getHeight() + b3.getHeight()) / 2;
	    x = (int)(c.getWidth() - b3.getWidth()) - insets.right;
	    if (b.isPointy()) {
		x -= b3.getHeight() / 2;
	    }
	    b3.setRect(b3.getX() + x, b3.getY() + y,
		       b3.getWidth(), b3.getHeight());
	    int ih = (int)b3.getHeight() + txtYPad;
	    int iw = (int)w + txtXPad;
	    // Remember where this is for direct type-in of increment
	    incrRect.setRect(b.getWidth() - iw, b.getHeight()/2 - ih/2,
			     iw, ih);
	    //g.setColor(Color.yellow);
	    //g2.fill(b3);
	    t3.draw(g2, x, y);
	}

	/**
	 * get width
	 * @return width
	 */
	public int getIconWidth() {
	    String valueStr = myButton.getValueStr();
	    String incrStr = myButton.getIncrementStr();
	    String label = myButton.getLabel();
	    FontMetrics metrics = myButton.getFontMetrics(myButton.getFont());
	    return (Math.max(metrics.stringWidth(label),
			     metrics.stringWidth(valueStr))
		    + metrics.stringWidth(incrStr));
	}

	/**
	 * get height
	 * @return height
	 */
	public int getIconHeight() {
	    String valueStr = myButton.getValueStr();
	    String incrStr = myButton.getIncrementStr();
	    String label = myButton.getLabel();
	    FontMetrics metrics = myButton.getFontMetrics(myButton.getFont());
	    return (int)(2 * metrics.getHeight());
	}

    }

    class TextActionListener implements ActionListener {
	public void actionPerformed(ActionEvent e) {
	    try {
		if (e.getActionCommand().equals("value")) {
		    button.setValue(Integer.parseInt(typeIn.getText()));
		} else if (e.getActionCommand().equals("incr")) {
		    int inc = Integer.parseInt(typeIn.getText());
		    button.setIncrement(Math.abs(inc));
		}
		typeIn.setVisible(false);
		ThreeButtonModel m = button.getStateModel();
		m.setEnabled(true);
		m.setPressed(false, 0);
		m.setArmed(false, 0); // Do this last
		button.repaint();
	    } catch (NumberFormatException exception) {}
	}
    }

    class ButtonActionListener implements ActionListener {
	public void actionPerformed(ActionEvent e) {
	    String cmd = e.getActionCommand();
	    //System.out.println("Button action: command=" + cmd);/*CMP*/
	    if (cmd.equals("1")) {
		button.setValue(button.getValue() - button.getIncrement());
	    } else if (cmd.equals("2")) {
		button.getDataModel().toggleIncrement();
                if (controlKey) {
                   try {
                     int incr = button.getDataModel().getIncrementIndex();
                     VShimSet shimset = (VShimSet) button.getParent();
                     shimset.setAttribute(VObjDef.SETINCREMENTS, Integer.toString(incr));
                   } catch (Exception exception) { }
                }
	    } else if (cmd.equals("3")) {
		button.setValue(button.getValue() + button.getIncrement());
	    }
	    button.repaint();
	}
    }

    protected class TimerActionListener implements ActionListener {
	int direction = 1;

	public TimerActionListener() {
	}

	public void setDirection(int direction) {
	    this.direction = direction;
	}

	public void actionPerformed(ActionEvent e) {
	    ThreeButtonModel m = button.getStateModel();
	    if (m.isArmed()) {
		boolean pressed = ! m.isPressed(direction);
		m.setPressed(pressed, direction);
	    }
	}
    }
}
