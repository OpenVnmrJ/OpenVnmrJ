/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.FileFilter;
import java.util.Collection;
import java.util.SortedSet;
import java.util.TreeSet;
import java.util.Vector;

import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;

import static vnmr.apt.AptDefs.NL;


public class ProbeNamePopup implements Runnable {
    private static final String OK_KEY = "OK_KEY";
    private static final String CANCEL_KEY = "CANCEL_KEY";
    private static final String PROBE_NAME_KEY = "PROBE_NAME_KEY";
    private static final String USR_PROBE_NAME_KEY = "USR_PROBE_NAME_KEY";

    private static String sm_okLabel = "OK";
    private static String sm_cancelLabel = "Cancel";
    private static String sm_probeNameLabel = "Probe Name: ";
    private static String sm_usrProbeNameLabel = "Probe ID: ";

    private static JFrame sm_dialog;
    private static JComboBox sm_usrProbeNameMenu;

    protected static String sm_probe;
    protected static String sm_usrProbeName;

    /**
     * Bring up a window for the user to enter the probe name and a second
     * "specific" probe name for the user persistence files.
     * When dismissed, the popup will instantiate ProbeTune with the selected
     * names.
     */
    public static void startProbeTune() {
        sm_probe = System.getProperty("apt.probeName");
        sm_usrProbeName = sm_probe;
        getPreviousProbeNameAndId();
        ActionHandler actionHandler = new ActionHandler();
        sm_dialog = new JFrame("Probe Chooser");
        JPanel mainPanel = makeMainPanel(actionHandler);
        JPanel bottomPanel = makeBottomPanel(actionHandler);

        sm_dialog.add(mainPanel, BorderLayout.CENTER);
        sm_dialog.add(bottomPanel, BorderLayout.SOUTH);

        sm_dialog.pack();
        Dimension dialogDim = sm_dialog.getPreferredSize();
        Toolkit tk = Toolkit.getDefaultToolkit();
        Dimension screenDim = tk.getScreenSize();
        int x = (int)((screenDim.getWidth() - dialogDim.getWidth()) / 2);
        int y = (int)((screenDim.getHeight() - dialogDim.getHeight()) / 2);
        sm_dialog.setLocation(x, y);
        sm_dialog.setVisible(true);
    }

    /**
     * Construct the bottom panel for the popup.
     * @param actionHandler Who handles the events.
     * @return The bottom panel.
     */
    private static JPanel makeBottomPanel(ActionHandler actionHandler) {
        JPanel bottomPanel = new JPanel();

        JButton okButton = new JButton(sm_okLabel);
        okButton.setActionCommand(OK_KEY);
        okButton.addActionListener(actionHandler);
        bottomPanel.add(okButton);

        JButton cancelButton = new JButton(sm_cancelLabel);
        cancelButton.setActionCommand(CANCEL_KEY);
        cancelButton.addActionListener(actionHandler);
        bottomPanel.add(cancelButton);

        return bottomPanel;
    }

    /**
     * Construct the main panel for the pop-up.
     * @param actionHandler Who handles the events.
     * @return The panel.
     */
    private static JPanel makeMainPanel(ActionListener actionHandler) {
        JPanel mainPanel = new JPanel();
        BoxLayout layout = new BoxLayout(mainPanel, BoxLayout.Y_AXIS);
        mainPanel.setLayout(layout);

        mainPanel.add(Box.createRigidArea(new Dimension(0, 5)));
        JPanel probeRow = new JPanel();
        probeRow.setLayout(new BoxLayout(probeRow, BoxLayout.X_AXIS));
        JLabel probeLabel = new JLabel(sm_probeNameLabel);
        probeRow.add(probeLabel);
        JComboBox probeMenu = getProbeMenu();
        probeMenu.setActionCommand(PROBE_NAME_KEY);
        probeMenu.addActionListener(actionHandler);
        String tip = "The probe name used to find the system persistence files";
        probeMenu.setToolTipText(tip);
        probeLabel.setToolTipText(tip);
        probeRow.add(probeMenu);
        mainPanel.add(probeRow);

        mainPanel.add(Box.createRigidArea(new Dimension(0, 5)));
        JPanel idRow = new JPanel();
        idRow.setLayout(new BoxLayout(idRow, BoxLayout.X_AXIS));
        JLabel idLabel = new JLabel(sm_usrProbeNameLabel);
        idRow.add(idLabel);
        sm_usrProbeNameMenu = getUsrProbeNameMenu();
        sm_usrProbeNameMenu.setActionCommand(USR_PROBE_NAME_KEY);
        sm_usrProbeNameMenu.addActionListener(actionHandler);
        tip = "The probe name used to save the user persistence files";
        sm_usrProbeNameMenu.setToolTipText(tip);
        idLabel.setToolTipText(tip);
        idRow.add(sm_usrProbeNameMenu);
        mainPanel.add(idRow);

        return mainPanel;
    }

    private static JComboBox getProbeMenu() {
        String tunePath = ProbeTune.getVnmrSystemDir() + "/tune";
        Vector<String> items = new Vector<String>(getSupportedProbes(tunePath));
        JComboBox menu = new JComboBox(items);
        menu.setSelectedItem(sm_probe);
        return menu;
    }

    private static JComboBox getUsrProbeNameMenu() {
        String tunePath = ProbeTune.getVnmrUserDir() + "/tune";
        Vector<String> items = new Vector<String>(getSupportedProbes(tunePath));
        JComboBox menu = new JComboBox(items);
        menu.setEditable(true);
        menu.setSelectedItem(sm_usrProbeName);
        return menu;
    }

   private static Collection<String> getSupportedProbes(String tunePath) {
        File[] files = new File(tunePath).listFiles(new ProbeNameFilter());
        SortedSet<String> names = new TreeSet<String>();
        for (File file : files) {
            names.add(file.getName());
        }
        Vector<String> probes = new Vector<String>(names);
        return probes;
    }

    public static void saveProbeNameAndId() {
        StringBuffer sb = new StringBuffer();
        sb.append(PROBE_NAME_KEY).append(" ").append(sm_probe).append(NL);
        sb.append(USR_PROBE_NAME_KEY).append(" ").append(sm_usrProbeName);
        sb.append(NL);
        String path = ProbeTune.getVnmrSystemDir() + "/tmp/ptuneBenchTest";
        TuneUtilities.writeFile(path, sb.toString());
    }

    public static void getPreviousProbeNameAndId() {
        String path = ProbeTune.getVnmrSystemDir() + "/tmp/ptuneBenchTest";
        String buf = TuneUtilities.readFile(path, false);
        String [] lines = buf.split("[\\r\\n]+");
        for (String line : lines) {
            String[] tokens = line.split("[ ]+");
            if (tokens[0].equals(PROBE_NAME_KEY)) {
                sm_probe = tokens[1];
            } else if (tokens[0].equals(USR_PROBE_NAME_KEY)) {
                sm_usrProbeName = tokens[1];
            }
        }
    }


    static class ProbeNameFilter implements FileFilter {

        @Override
        public boolean accept(File file) {
            if (file.isDirectory()) {
                if (new File(file, "chan#0").isFile()) {
                    return true;
                }
            }
            return false;
        }
    }


    static class ActionHandler implements ActionListener {

        @Override
        public void actionPerformed(ActionEvent e) {
            String cmd = e.getActionCommand();
            Messages.postDebug("ProbePopup", "cmd=" + cmd);
            if (OK_KEY.equals(cmd)) {
                sm_dialog.dispose();
                    updateUsrProbeName(sm_usrProbeNameMenu);
                    saveProbeNameAndId();
                    new RunProTune(sm_probe, sm_usrProbeName).start();
            } else if (PROBE_NAME_KEY.equals(cmd)) {
                JComboBox menu = (JComboBox)e.getSource();
                sm_probe = (String)menu.getSelectedItem();
                System.setProperty("apt.probeName", sm_probe);
                Messages.postDebug("ProbePopup", "probe=" + sm_probe);
            } else if (USR_PROBE_NAME_KEY.equals(cmd)) {
                JComboBox menu = (JComboBox)e.getSource();
                updateUsrProbeName(menu);
            } else if (CANCEL_KEY.equals(cmd)) {
                sm_dialog.dispose();
            }
        }

        /**
         * Updates the probeId variable based on the entry box.
         */
        private void updateUsrProbeName(JComboBox menu) {
            sm_usrProbeName = (String)menu.getSelectedItem();
            System.setProperty("apt.usrProbeName", sm_usrProbeName);
            Messages.postDebug("ProbePopup", "usrProbeName=" + sm_usrProbeName);
        }
    }

    @Override
    public void run() {
        startProbeTune();
    }


    static class RunProTune extends Thread {
        private String m_probeName;
        private String m_usrProbeName;

        public RunProTune(String probeName, String usrProbeName) {
            m_probeName = probeName;
            m_usrProbeName = usrProbeName;
        }

        @Override
        public void run() {
            try {
                new ProbeTune(m_probeName, m_usrProbeName);
            } catch (InterruptedException e) {
            }
        }
    }
}
