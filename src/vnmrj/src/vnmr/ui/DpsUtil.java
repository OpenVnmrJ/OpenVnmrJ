/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import javax.swing.*;

public class DpsUtil {
    private static DpsScopePanel scopePan;
    private static DpsDataPanel dataPan;
    private static DpsChannelName namePan;
    private static DpsScopeContainer scopeContainer;
    private static JScrollPane nameScrollPan;
    private static JPanel infoPan;
    private static DpsTimeLine timeLinePan;
    private static DpsOptions optionPan;
    private static int timeLineHeight = 20;
    private static double seqTime;
    private static double ppt; // pixel per micro second
    private static DpsWindow  mainWindow;
    private static JPanel toolPanel;

    private DpsUtil()  { }

    public static DpsScopePanel getScopePanel() {
        return scopePan;
    }

    public static void setScopePanel(DpsScopePanel panel) {
        scopePan = panel;
    }

    public static DpsScopeContainer getScopeContainer() {
        return scopeContainer;
    }

    public static void setScopeContainer(DpsScopeContainer panel) {
        scopeContainer = panel;
    }

    public static DpsDataPanel getDataPanel() {
        return dataPan;
    }

    public static void setDataPanel(DpsDataPanel panel) {
        dataPan = panel;
    }

    public static DpsChannelName getNamePanel() {
        return namePan;
    }

    public static void setNamePanel(DpsChannelName panel) {
        namePan = panel;
    }

    public static JScrollPane getNameScrollPanel() {
        return nameScrollPan;
    }

    public static void setNameScrollPanel(JScrollPane panel) {
        nameScrollPan = panel;
    }

    public static JPanel getInfoPanel() {
        return infoPan;
    }

    public static void setInfoPanel(JPanel panel) {
        infoPan = panel;
    }

    public static DpsOptions getOptionPanel() {
        return optionPan;
    }

    public static void setOptionPanel(DpsOptions panel) {
        optionPan = panel;
    }

    public static DpsTimeLine getTimeLinePanel() {
        return timeLinePan;
    }

    public static void setTimeLinePanel(DpsTimeLine panel) {
        timeLinePan = panel;
    }

    public static int getTimeLineHeight() {
        return timeLineHeight;
    }

    public static void setTimeLineHeight(int h) {
        timeLineHeight = h;
    }

    public static double getSeqTime() {
        return seqTime;
    }

    public static void setSeqTime(double n) {
        seqTime = n;
    }

    public static double getTimeScale() {
         return ppt;
    }

    // set pixel per time
    public static void setTimeScale(double n) {
         ppt = n;
    }

    public static void showDpsObjInfo(DpsObj obj) {
        if (dataPan != null)
            dataPan.showDpsObjInfo(obj); 
    }

    public static void setDpsWindow(DpsWindow win) {
        mainWindow = win;
    }

    public static DpsWindow getDpsWindow() {
        return mainWindow;
    }

    public static JPanel getToolPanel() {
        if (toolPanel != null)
            return toolPanel;
        if (mainWindow != null)
            toolPanel = mainWindow.getToolPanel();
        return toolPanel;
    }
}
