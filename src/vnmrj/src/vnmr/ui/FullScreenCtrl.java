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
import java.util.*;
import javax.swing.*;
import vnmr.bo.*;
import vnmr.util.Util;
import vnmr.util.SimpleHLayout;
import vnmr.util.SimpleH2Layout;
import vnmr.util.SimpleVLayout;
import vnmr.admin.ui.*;
import vnmr.admin.util.*;



public class FullScreenCtrl extends JDialog implements ActionListener {

      private static FullScreenCtrl ctrl = null;
      private JFrame frame;
      private Window originWin;
      private JButton cancelButton;
      private JButton okButton;
      private JLabel  topMessage;
      private JLabel  message1;
      private JLabel  message2;
      private JComboBox cbUser;
      private JPasswordField password;
      private boolean bFullMode;
      private boolean bDoable;
      private GraphicsDevice gd;
      private Point frameLoc;
      private Dimension frameDim;
      public static final String okCmd = "ok";
      public static final String cancelCmd = "cancel";

      public FullScreenCtrl() {
           // super(VNMRFrame.getVNMRFrame(), "Full Screen Login", false);
           build();
           setTitle("Full Screen Login");
      }

      public static void showDialog() {
           if (ctrl == null)
                ctrl = new FullScreenCtrl();
           ctrl.openLoginBox();
      }

      private void check() {
          bDoable = false;
          frame = Util.getMainFrame();
          if (frame == null)
               return;
          gd = GraphicsEnvironment.getLocalGraphicsEnvironment().
                    getDefaultScreenDevice();
          if (gd == null)
               return;
          Window fwin = (Window) frame;
          Window win = gd.getFullScreenWindow();
          if (fwin != win) {
               originWin = win;
               bFullMode = false;
          }
          else
               bFullMode = true;
          bDoable = true;
      }

      private void openLoginBox() {
           check();
           if (!bDoable) {
               setVisible(false);
               return;
           }
           if (bFullMode)
               topMessage.setText(" Turn off full screen mode ");    
           else
               topMessage.setText(" Turn on full screen mode ");    

           Dimension ds = Toolkit.getDefaultToolkit().getScreenSize();
           Dimension d0 = getSize();
           int x = (ds.width - d0.width) / 2;
           int y = (ds.height - d0.height) / 2;
           if (x < 0)
               x = 0;
           if (y < 0)
               y = 100;
           setLocation(x, y);
           password.setText("");

           setVisible(true);
      }

      private void toggleFullScreen() {
           check();
           if (bFullMode) {
               gd.setFullScreenWindow(null);
               frame.removeNotify();
               frame.setResizable(true);
               frame.setUndecorated(false);
               frame.addNotify();
           }
           else {
               frameLoc = frame.getLocation();
               frameDim = frame.getSize();
               frame.removeNotify();
               frame.setResizable(false);
               frame.setUndecorated(true);
               frame.addNotify();
               gd.setFullScreenWindow(frame);
           }

      }

      public void actionPerformed(ActionEvent e) {
           String cmd = e.getActionCommand();
           if (!this.isShowing()) {
                return;
           }
           if (cmd == null)
                return;
           if (cmd.equals(okCmd)) {
                verifyLogin();
                return;
           }
           setVisible(false);
      }

      protected Object[] getOperators()
      {
          String strUser = System.getProperty("user.name");
          User user = LoginService.getDefault().getUser(strUser);
          ArrayList aListOperators = user.getOperators();
          if (aListOperators == null || aListOperators.isEmpty())
              aListOperators = new ArrayList();
          Collections.sort(aListOperators);
          if (aListOperators.contains(strUser))
              aListOperators.remove(strUser);
          aListOperators.add(0, strUser);
          return (aListOperators.toArray());
      }

      private void verifyLogin() {
          char[] strPassword = password.getPassword();
          String strUser = (String)cbUser.getSelectedItem();
          setCursor(Cursor.getPredefinedCursor(Cursor.WAIT_CURSOR));
          boolean blogin = WUserUtil.isOperatorNameok(strUser, false);
          if (strUser.length() < 1)
               return;
          if (blogin)
          {
               blogin = LoginBox.unixPassword(strUser, strPassword);
               if (!blogin)
                   blogin = LoginBox.vnmrjPassword(strUser, strPassword);
               if (blogin) {
                   SwingUtilities.invokeLater( new Runnable() {
                       public void run() {
                           toggleFullScreen();
                       }
                   } ); 
                   setVisible(false);
                   return;
               }
          }
          message1.setText(" Incorrect username/password ");
          message2.setText(" Please try again ");
          password.setText("");
      }

      private void build() {
           Container cont = getContentPane();
           cont.setLayout(new BorderLayout());

           JPanel topPanel = new JPanel();
           topPanel.setLayout(new SimpleVLayout(6, 4, false));
           cont.add(topPanel, BorderLayout.CENTER);

           JPanel pan0 = new JPanel();
           pan0.setLayout(new SimpleHLayout());
           topPanel.add(pan0);
           topMessage = new JLabel("    ");
           pan0.add(topMessage);

           JPanel pan1 = new JPanel();
           pan1.setLayout(new SimpleHLayout());
           topPanel.add(pan1);

           JLabel label = new JLabel(Util.getLabel("_Operator"));
           pan1.add(label);
           
           cbUser = new JComboBox(getOperators());
           cbUser.setEditable(true);



           pan1.add(cbUser);

           JPanel pan2 = new JPanel();
           pan2.setLayout(new SimpleHLayout());
           topPanel.add(pan2);
           label = new JLabel(Util.getLabel("_Password"));

           pan2.add(label);
           password = new JPasswordField();
           pan2.add(password);

           password.addKeyListener(new KeyAdapter()
           {
               public void keyPressed(KeyEvent e)
               {
                    if (e.getKeyCode() == KeyEvent.VK_ENTER)
                         verifyLogin();
               }
           });


         /***
           JPanel pan3 = new JPanel();
           pan3.setLayout(new SimpleHLayout());
           topPanel.add(pan3);
           label = new JLabel("    ");
           pan3.add(label);
          ***/

           JPanel pan4 = new JPanel();
           pan4.setLayout(new SimpleHLayout());
           topPanel.add(pan4);
           message1 = new JLabel("  message 1  ");
           pan4.add(message1);

           JPanel pan5 = new JPanel();
           pan5.setLayout(new SimpleHLayout());
           topPanel.add(pan5);
           message2 = new JLabel(" message 2   ");
           pan5.add(message2);

           JPanel bottomPanel = new JPanel();
           bottomPanel.setBorder(BorderFactory.createEmptyBorder(0, 5, 10, 5));
           bottomPanel.setLayout(new SimpleH2Layout(SimpleH2Layout.CENTER, 20, 5, false));
           cont.add(bottomPanel, BorderLayout.SOUTH);

           String okStr = Util.getLabel("blOk", "Ok");
           String cancelStr = Util.getLabel("blCancel", "Cancel");
           okButton = new JButton(okStr);
           cancelButton = new JButton(cancelStr);
           okButton.setActionCommand(okCmd);
           okButton.addActionListener(this);
           cancelButton.setActionCommand(cancelCmd);
           cancelButton.addActionListener(this);
           bottomPanel.add(okButton);
           bottomPanel.add(cancelButton);

           setAlwaysOnTop(true);
           setResizable(false);

           pack();
      }

}

