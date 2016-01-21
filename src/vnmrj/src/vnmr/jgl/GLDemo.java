/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.jgl;

import java.awt.*;
import java.awt.event.*;
import java.awt.geom.*;
import java.awt.image.*;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.io.*;
import java.text.*;
import javax.imageio.*;
import javax.swing.*;
import javax.swing.border.Border;
import javax.swing.border.EtchedBorder;

import javax.media.opengl.*;
import javax.media.opengl.awt.GLJPanel;

import com.jogamp.opengl.util.Animator;

//import com.sun.opengl.util.*;

import vnmr.ui.VNMRFrame;
import vnmr.util.*;
import vnmr.bo.*;

public class GLDemo extends JDialog implements VObjDef,ActionListener,PropertyChangeListener {
    private GLJPanel  drawable=null;
    private Animator animator=null;
    private JToggleButton runButton;
    private JCheckBox checkBox;
    private JPanel optionsPan=null;
    private JPanel gradientPanel=null;
    private Container contentPane;
 
    private int NONE=0;
    private int GEARS=1;
    private int type=0;
    

    public GLDemo() {
        super(VNMRFrame.getVNMRFrame(), "Jogl Demo", false);
 
        DisplayOptions.addChangeListener(this);

        contentPane = getContentPane();
        contentPane.setLayout(new BorderLayout());

        gradientPanel = createGradientPanel();
        
        contentPane.add(gradientPanel, BorderLayout.CENTER);
 
        checkBox = new JCheckBox("Transparent", true);
        checkBox.setActionCommand("transparancy");
        checkBox.addActionListener(this);
        optionsPan=new JPanel();
        optionsPan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 5, 0, true,false));
        optionsPan.setBorder(new EtchedBorder(EtchedBorder.LOWERED));

        optionsPan.add(checkBox);
        
        runButton=new JToggleButton("Run");
        runButton.setActionCommand("run");
        runButton.setSelected(false);
        runButton.addActionListener(this);
        
        optionsPan.add(runButton);

        getContentPane().add(optionsPan, BorderLayout.SOUTH);
        setSize(300, 300);
        setLocation( 300, 300 );
        
        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                // Run this on another thread than the AWT event queue to
                // make sure the call to Animator.stop() completes before
                // exiting
               new Thread(new Runnable() {
                    public void run() {
                        stop();
                    }
                  }).start();
            }
          });
        setVisible(false);
    } 
    
    public void setDemo(String demo){ 
        setTitle("Jogl Demo: "+demo);
        if(animator==null){
            animator=new Animator();
            animator.setIgnoreExceptions(true);
        }
        //stop();
        if(drawable !=null){  
            gradientPanel.remove(drawable);
            animator.remove(drawable);
            drawable=null;
        }
        if(demo.equals("gears")){
            drawable=new JGears();
            type=GEARS;
        }
        //else if(demo.equals("graphics")){
        //    type=GRAPHICS;  
         //   drawable=new JGLGraphics();
        //}
        if(drawable !=null){
            gradientPanel.add(drawable, BorderLayout.CENTER);
            animator.add(drawable);
        }
   }
    public void stop(){
        runButton.setSelected(false);
        runButton.repaint();
        animator.stop();
     }
    public void start(){
        runButton.setSelected(true);
        runButton.repaint();
        animator.start();                
     }
    
    /** handle button events. */
    public void actionPerformed(ActionEvent e ) {
        String cmd = e.getActionCommand();
        if(cmd.equals("run"))  {
            if(animator.isAnimating())
                stop();           
            else
                start();
        }
        else if(cmd.equals("transparancy"))  {
            if(drawable !=null)
                drawable.setOpaque(!checkBox.isSelected());
        }
    }
     public static JPanel createGradientPanel() {
        JPanel gradientPanel = new JPanel() {
            Color c = Util.getBgColor();

            public void paintComponent(Graphics g) {
                ((Graphics2D)g).setPaint(new GradientPaint(0, 0, c.brighter(),
                        getWidth(), getHeight(), c.darker()));
                g.fillRect(0, 0, getWidth(), getHeight());
            }
        };
        gradientPanel.setLayout(new BorderLayout());
        return gradientPanel;
    }
    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt) {
        if(DisplayOptions.isUpdateUIEvent(evt)) {
            SwingUtilities.updateComponentTreeUI(this);
            gradientPanel.remove(drawable);
            JPanel newpanel = createGradientPanel();
            contentPane.remove(gradientPanel);
            gradientPanel = newpanel;
            gradientPanel.add(drawable, BorderLayout.CENTER);
            contentPane.add(gradientPanel, BorderLayout.CENTER);
        }
    }
}
