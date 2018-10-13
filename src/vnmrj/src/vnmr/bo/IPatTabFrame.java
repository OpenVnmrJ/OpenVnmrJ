/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.util.Vector;

 /* OK here is what I've learn so far, before I forget it over Xmas break.

    1. Mouse events are not blocked via JButton.setEnable(false)
    2. Action_Events are  blocked via JButton.setEnable(false)
    3. Play with mouse press and release to be able to due
       a button repeat via the
       repeatButton = new Timer(100,new java.awt.event.ActionListener() {
          public void actionPerformed(ActionEvent e) {
              UpDownInOutRepeatButton_actionPerformed(e);
          }
          });
       Thus in mouse pressed used repeatButton.start() and mouse released
            if (repeatButton.isRunning()) repeatButton.stop();

       However when used with actual patient table a repeating button was
      not what one would want.

    4. The biggest problem was that even for action_events the
       JButton.setEnable(false) had no effect until the action_event returned.
       Thus if the actual robot action was down within the action_event
       then any addition button presses while the table moved were queued.
       And then acted on after the tabled completed the 1st button press.
       What was needed was to prevent addition button presses being queued.

    5. One way was to look at the timestamp of the button event and based on
       when the last moused released timestamp where the mouse events happened
       during the table movement. Unfortunately the timestamps where not when
       the mouse was pressed but when they were serviced. Thus the mouse press
       timestamp could never be prior to the mouse release. Thus I could not use
       the timestamp in this way. However the other way to use the timestamp was
       to look at the difference in time, thus in the mouse presses appearred to
       happen just 100s of milliseconds apart I could ignore them. This worked
       but since the table can take significate time to move > 10 sec, this
       method might not work in all cases. So I looked for another way.

    6. The next method was to endow the Com object with seperate threads
       Thus the action_event could just queue the command to the thread
       and return quickly thus disabling the button.
       Another watching for the Cmd prompt then calls the update method, I
       used the Observer and Observable classes. the update method looked at the
       cmd responcible to this update and perform the proper operation of re-enabling
       the button and updating the X or Y position.

    7. Design using the EventObject model. The above threads were used, the
       Observer and Observable classes were replaced with EventObjects. Thus
       following the AWT event model. Thus the IPatTabFrame implements a
       PatientTableListener interface. The patientTableComm is the source of
       PatientTableEvents. The IPatTabFrame adds itself to the patientTableComm
       Listener list (addPatientTableListener(this)). The PatientTableEvent
       is created by the patientTableComm and then this event is fired off by
       calling the appropriate PatientTableListener method of the registered
       listener (i.e. IPatTabFrame).  I tried extending the patientTableEvent
       class by other more specific events. Thus hoping that a single Listener
       method with parameter overloaded would call the appropriate method.
       Unfortunately the events need to be handle via the super class
       (PatientTableEvent) and thus the overloading would not work.



 */

public class IPatTabFrame extends JFrame implements PatientTableListener {
  JPanel contentPane;
  BorderLayout borderLayout1 = new BorderLayout();
  JPanel jPanel1 = new JPanel();
  GridBagLayout gridBagLayout1 = new GridBagLayout();
  JLabel LocLabel = new JLabel();
  JTextField AbsHorzValue = new JTextField();
  JTextField AbsVertValue = new JTextField();
  JLabel ISOLabel = new JLabel();
  JTextField ISOHorzValue = new JTextField();
  JTextField ISOVertValue = new JTextField();
  JButton UpButton = new JButton();
  JButton OutButton = new JButton();
  JButton InButton = new JButton();
  JButton DownButton = new JButton();
  JComboBox INComboBox = new JComboBox();

  JComboBox UpComboBox = new JComboBox();
  DefaultComboBoxModel dcbmInOut,dcbmUpDown;

  // javax.swing.Timer repeatButton;
  Integer Xincr, Yincr;

  private PatientTableComm ptc;
  private String ptalignPath, termDev;

  /**Construct the frame*/
  public IPatTabFrame(String ptalignpath,String termdev) {
    ptalignPath = ptalignpath;
    termDev = termdev;
    enableEvents(AWTEvent.WINDOW_EVENT_MASK);
    try {
      jbInit();
       // ptc = new PatientTableComm("/vnmr/bin","/dev/term/a");
      ptc = new PatientTableComm(ptalignPath,termDev);
      ptc.addPatientTableListener(this);
      ptc.getXLoc();
      ptc.getYLoc();
    }
    catch(Exception e) {
      e.printStackTrace();
    }
  }
  /**Component initialization*/
  private void jbInit() throws Exception  {

    DownButton.setMaximumSize(new Dimension(100, 37));
    DownButton.setMinimumSize(new Dimension(100, 37));
    DownButton.setPreferredSize(new Dimension(100, 37));
    DownButton.setToolTipText("Move Patient Table Down by increment shown");
    // DownButton.setText("DOWN \u00B11");
    DownButton.setText("DOWN -1");
    Yincr = new Integer(1);

    DownButton.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        DownButton_actionPerformed(e);
      }
    });

    InButton.setMaximumSize(new Dimension(81, 37));
    InButton.setMinimumSize(new Dimension(81, 37));
    InButton.setPreferredSize(new Dimension(81, 37));
    InButton.setToolTipText("Move Patient Table into Magnet by increment shown");
    // InButton.setText("IN \u00b11");
    InButton.setText("IN +1");
    Xincr = new Integer(1);

    InButton.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        InButton_actionPerformed(e);
      }
    });

    OutButton.setMaximumSize(new Dimension(81, 37));
    OutButton.setMinimumSize(new Dimension(81, 37));
    OutButton.setPreferredSize(new Dimension(81, 37));
    OutButton.setToolTipText("Move Patient Table Out of magnet by increment shown");
    //OutButton.setText("OUT \u00b11");
    OutButton.setText("OUT -1");

    OutButton.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        OutButton_actionPerformed(e);
      }
    });

    UpButton.setMaximumSize(new Dimension(81, 37));
    UpButton.setMinimumSize(new Dimension(81, 37));
    UpButton.setPreferredSize(new Dimension(81, 37));
    UpButton.setToolTipText("Move Patient Table Up by increment shown");
    //UpButton.setText("UP \u00B11")
    UpButton.setText("UP +1");

    UpButton.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        UpButton_actionPerformed(e);
      }
    });

    ISOVertValue.setPreferredSize(new Dimension(51, 21));
    ISOVertValue.setEditable(false);
    ISOVertValue.setHorizontalAlignment(SwingConstants.RIGHT);
    ISOVertValue.setText("0");
    ISOHorzValue.setPreferredSize(new Dimension(51, 21));
    ISOHorzValue.setEditable(false);
    ISOHorzValue.setHorizontalAlignment(SwingConstants.RIGHT);
    ISOHorzValue.setText("0");
    ISOLabel.setText("Dist. From Isocenter:");
    AbsVertValue.setPreferredSize(new Dimension(51, 21));
    AbsVertValue.setEditable(false);
    AbsVertValue.setHorizontalAlignment(SwingConstants.RIGHT);
    AbsVertValue.setText("0");
    AbsHorzValue.setPreferredSize(new Dimension(51, 21));
    AbsHorzValue.setEditable(false);
    AbsHorzValue.setHorizontalAlignment(SwingConstants.RIGHT);
    AbsHorzValue.setText("0");
    LocLabel.setText("Location  Hor,Vert:");
    jPanel1.setLayout(gridBagLayout1);
    //setIconImage(Toolkit.getDefaultToolkit().createImage(IPatTabFrame.class.getResource("[Your Icon]")));
    contentPane = (JPanel) this.getContentPane();
    contentPane.setLayout(borderLayout1);
    this.setSize(new Dimension(400, 300));
    this.setTitle("Imaging Patient Table");

    Vector ilist = new Vector();
    INComboBox.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        INComboBox_actionPerformed(e);
      }
    });
    UpComboBox.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        UpComboBox_actionPerformed(e);
      }
    });
    UpComboBox.setToolTipText("UP/DOWN Increment Selection");
    INComboBox.setToolTipText("IN/OUT Increment Selection");
    ilist.add(new Integer(1)); ilist.add(new Integer(5));
    ilist.add(new Integer(10)); ilist.add(new Integer(25));
    ilist.add(new Integer(50)); ilist.add(new Integer(100));
    dcbmInOut = new DefaultComboBoxModel(ilist);
    dcbmUpDown = new DefaultComboBoxModel(ilist);
    UpComboBox.setMinimumSize(new Dimension(60, 23));
    UpComboBox.setPreferredSize(new Dimension(64, 23));
    UpComboBox.setModel(dcbmUpDown);
    INComboBox.setPreferredSize(new Dimension(64, 23));
    INComboBox.setModel(dcbmInOut);

    contentPane.add(jPanel1, BorderLayout.CENTER);
    jPanel1.add(LocLabel, new GridBagConstraints(0, 0, 1, 1, 0.0, 0.0
            ,GridBagConstraints.CENTER, GridBagConstraints.NONE, new Insets(0, 0, 0, 0), 0, 0));
    jPanel1.add(AbsHorzValue, new GridBagConstraints(1, 0, 1, 1, 0.0, 0.0
            ,GridBagConstraints.CENTER, GridBagConstraints.NONE, new Insets(0, 0, 0, 0), 0, 0));
    jPanel1.add(AbsVertValue, new GridBagConstraints(2, 0, 1, 1, 0.0, 0.0
            ,GridBagConstraints.CENTER, GridBagConstraints.NONE, new Insets(0, 0, 0, 0), 0, 0));
    jPanel1.add(ISOLabel, new GridBagConstraints(0, 1, 1, 1, 0.0, 0.0
            ,GridBagConstraints.CENTER, GridBagConstraints.NONE, new Insets(0, 0, 0, 0), 0, 0));
    jPanel1.add(ISOHorzValue, new GridBagConstraints(1, 1, 1, 1, 0.0, 0.0
            ,GridBagConstraints.CENTER, GridBagConstraints.NONE, new Insets(0, 0, 0, 0), 0, 0));
    jPanel1.add(ISOVertValue, new GridBagConstraints(2, 1, 1, 1, 0.0, 0.0
            ,GridBagConstraints.CENTER, GridBagConstraints.NONE, new Insets(0, 0, 0, 0), 0, 0));
    jPanel1.add(UpButton, new GridBagConstraints(1, 2, 1, 1, 0.0, 0.0
            ,GridBagConstraints.CENTER, GridBagConstraints.NONE, new Insets(0, 0, 0, 0), 0, 0));
    jPanel1.add(OutButton, new GridBagConstraints(0, 3, 1, 1, 0.0, 0.0
            ,GridBagConstraints.EAST, GridBagConstraints.NONE, new Insets(0, 0, 0, 0), 0, 0));
    jPanel1.add(InButton, new GridBagConstraints(2, 3, 1, 1, 0.0, 0.0
            ,GridBagConstraints.WEST, GridBagConstraints.NONE, new Insets(0, 0, 0, 0), 0, 0));
    jPanel1.add(DownButton, new GridBagConstraints(1, 4, 1, 1, 0.0, 0.0
            ,GridBagConstraints.CENTER, GridBagConstraints.NONE, new Insets(0, 0, 0, 0), 0, 0));
    jPanel1.add(INComboBox, new GridBagConstraints(3, 3, 1, 1, 0.0, 0.0
            ,GridBagConstraints.CENTER, GridBagConstraints.NONE, new Insets(0, 0, 0, 0), 0, 0));
    jPanel1.add(UpComboBox, new GridBagConstraints(3, 2, 1, 1, 0.0, 0.0
            ,GridBagConstraints.CENTER, GridBagConstraints.NONE, new Insets(0, 0, 0, 0), 0, 0));
  }
  /**Overridden so we can exit when window is closed*/
  protected void processWindowEvent(WindowEvent e) {
    super.processWindowEvent(e);
    if (e.getID() == WindowEvent.WINDOW_CLOSING) {
      System.exit(0);
    }
  }

  /* Patient Table Event Interface methods */
  public void tableMotionUpCmplt(PatientTableEvent e)
  {
    //UpButton.setEnabled(true);
    ptc.getYLoc();
  }
  public void tableMotionDownCmplt(PatientTableEvent e)
  {
    //DownButton.setEnabled(true);
    ptc.getYLoc();
  }
  public void tableMotionInCmplt(PatientTableEvent e)
  {
    //InButton.setEnabled(true);
    ptc.getXLoc();
  }

  public void tableMotionOutCmplt(PatientTableEvent e)
  {
    //OutButton.setEnabled(true);
    ptc.getXLoc();
  }

  private String AxisLocToString(String locationValue)
  {
    Double axisPos = Double.valueOf(locationValue);
    long axisPosInt = Math.round(axisPos.doubleValue());
    String locvalue = new Long(axisPosInt).toString();
    System.out.println("Axis Location: "+axisPos+"  Y: "+locvalue);
    return(locvalue);
  }

  public void tableMotionQweryCmplt(PatientTableEvent e)
  {
       String y,x;
       //String answer = (String) e.getData();
       System.out.println("tableMotionQweryCmplt");
       java.util.List answers = e.getDataList();
       int size = e.getDataSize();
       for (int i=0; i < size; i++)
          System.out.println("answer["+i+"] = '"+answers.get(i)+"'");
       if (e.isVerticalLocation())
       {
         if (size > 0)
         {
            y = AxisLocToString((String) answers.get(0));
            AbsVertValue.setText(y);
         }
         if (size > 1)
         {
            y = AxisLocToString((String) answers.get(1));
            ISOVertValue.setText(y);
         }
         UpButton.setEnabled(true);
         DownButton.setEnabled(true);
      }
      else if(e.isHorizontalLocation())
      {
         if (size > 0)
         {
            x = AxisLocToString((String) answers.get(0));
            AbsHorzValue.setText(x);
         }
         if (size > 1)
         {
            x = AxisLocToString((String) answers.get(1));
            ISOHorzValue.setText(x);
         }
         OutButton.setEnabled(true);
         InButton.setEnabled(true);
      }
  }

  void INComboBox_actionPerformed(ActionEvent e) {
      Xincr = (Integer) dcbmInOut.getSelectedItem();
      System.out.println("IN/OUT Selected: "+Xincr);
      //InButton.setText("IN \u00b1"+item);
      //OutButton.setText("OUT \u00b1"+item);
      InButton.setText("IN +"+Xincr);
      OutButton.setText("OUT -"+Xincr);
  }

  void UpComboBox_actionPerformed(ActionEvent e) {
      Yincr = (Integer) dcbmUpDown.getSelectedItem();

      System.out.println("UP/DOWN Selected: "+Yincr);
      //UpButton.setText("UP \u00b1"+item);
      //DownButton.setText("DOWN \u00b1"+item);
      UpButton.setText("UP +"+Yincr);
      DownButton.setText("DOWN -"+Yincr);
  }

  void DownButton_actionPerformed(ActionEvent e)
  {
    DownButton.setEnabled(false);
    ptc.moveY(-(Yincr.intValue()));
  }

  void UpButton_actionPerformed(ActionEvent e)
  {
    System.out.println("UpButton_actionPerformed");
    System.out.println("UpButton: "+UpButton);
    System.out.println("ptc: "+ptc+" Yincr: "+Yincr);
    UpButton.setEnabled(false);
    ptc.moveY(Yincr.intValue());
  }

  void InButton_actionPerformed(ActionEvent e)
  {
    InButton.setEnabled(false);
    ptc.moveX((Xincr.intValue()));
  }

  void OutButton_actionPerformed(ActionEvent e)
  {
    OutButton.setEnabled(false);
    ptc.moveX(-(Xincr.intValue()));
  }



/*
  public void update(java.util.Observable obs, Object arg)
  {
    String cmd;
    System.out.println("update called: "+obs+" Arg: "+arg);
    PatientTableComm lptc = (PatientTableComm) obs;
    if (!lptc.cmdll.isEmpty())
    {
      cmd = (String) lptc.cmdll.get(0);
      lptc.cmdll.remove(0);
    }
    else
      cmd = "NA";
    System.out.println("update called: resulted from cmd: '"+cmd+"'");
    if (cmd.length() > 3)
    {
      String scmd = cmd.substring(0,3);
      System.out.println("subcmd: '"+scmd+"'");
      if (scmd.compareToIgnoreCase("MY-") == 0)
      {
        UpdateYLoc();
        DownButton.setEnabled(true);
      }
      else if (scmd.compareToIgnoreCase("MY+") == 0)
      {
        UpdateYLoc();
        UpButton.setEnabled(true);
      }
      else if (scmd.compareToIgnoreCase("MX+") == 0)
      {
        UpdateXLoc();
        InButton.setEnabled(true);
      }
      else if (scmd.compareToIgnoreCase("MX-") == 0)
      {
        UpdateXLoc();
        OutButton.setEnabled(true);
      }
    }
    // ptc.waitForCmdPrompt();
    //UpdateYLoc();
    // DownButton.setEnabled(true);
  }
  */
}
