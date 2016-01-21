/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.util.*;
import java.io.File;
import java.io.FilenameFilter;

import vnmr.util.*;
import vnmr.ui.*;


/**
 * This is a VSubMenu that populates itself with a list of the available
 * workspace numbers. 
 */
public class VExperimentSubMenu extends VSubMenu {


    /**
     * Standard constructor for VObj's.
     */
    public VExperimentSubMenu(SessionShare ss, ButtonIF vif, String typ) {
        super(ss, vif, typ);
    }

    /**
     * Override the method in VSubMenu to populate the menu at the
     * appropriate times.
     */
    public void updateMe() {
        removeAll();
        ExperimentInfo[] exps = getExperimentList();
        for (int i = 0; i < exps.length; i++) {
            String label = exps[i].number + " ";
            addItem(exps[i]);
        }
    }

    /**
     * Add a VSubMenuItem to this menu for a given experiment.
     * The item will be disabled if the experiment is already loaded
     * into any workspace.
     * The label is just the experiment number.
     * The action on selection is hard coded to "jexp1(N)", where N is
     * the experiment number.
     * @param exp The experiment to add to the menu.
     */
    protected void addItem(ExperimentInfo exp) {
        VSubMenuItem item = new VSubMenuItem(sshare, vnmrIf, "mchoice");
        boolean enabled = !exp.isLoaded;
        if (!enabled) {
            item.setEnabled(false);
        }
        item.setBackground(getBackground());
        String itemLabel = exp.number + "";
        item.setAttribute(LABEL, itemLabel);
        item.setAttribute(CMD, "jexp1(" + exp.number + ")");
        super.add(item);
    }

    /**
     * Construct an array of ExperimentInfo's containing all the
     * experiments in the user's vnmrsys directory.
     * @return The array of ExperimentInfo's.
     */
    protected ExperimentInfo[] getExperimentList() {
        String[] expNames = getExpNames();
        SortedSet<String> loadedExps = getLoadedExpNames();
        ExperimentInfo[] exps = new ExperimentInfo[expNames.length];
        for (int i = 0; i < exps.length; i++) {
            exps[i] = new ExperimentInfo();
            exps[i].name = expNames[i];
            exps[i].number = getExpNumber(expNames[i]);
            exps[i].isLoaded = loadedExps.contains(expNames[i]);
        }
        Arrays.sort(exps);
        return exps;
    }

    /**
     * Get the names of all the experiments that are loaded into workspaces.
     * @return A SortedSet containing the names.
     */
    protected SortedSet<String> getLoadedExpNames() {
        ExpViewIF expViewIf = ((ExpPanel)vnmrIf).getExpView();
        SortedSet<String> names;
        if (expViewIf instanceof ExpViewArea) {
            names = ((ExpViewArea)expViewIf).getExperimentNames();
        } else {
            names = new TreeSet<String>();
        }
        return names;
    }

    /**
     * Get the names of all the experiments belonging to this user.
     * @return An array of names.
     */
    static protected String[] getExpNames() {
        File userdir = new File(FileUtil.openPath("USER"));
        String[] names = userdir.list(new ExpNameFilter());
        return names;
    }
    
    /**
     * Extracts the experiment number from a given experiment name.
     * E.g., if the name is "exp22", returns 22.
     * @param name The experiment name.
     * @return The experiment number, or -1 if "name" is not a legal
     * experiment name.
     */
    static int getExpNumber(String name) {
        int number = -1;
        if (name == null || !name.startsWith("exp")) {
            return -1;
        }
        name = name.substring(3); // Strip off initial "exp"
        try {
            number = Integer.parseInt(name);
        } catch (NumberFormatException nfe) {
            return -1;
        }
        return number;
    }



    /**
     * A container class with the information about an experiment
     * that is relevant to the VExperimentSubMenu.
     * Implements Comparable so as to sort ExperimentInfo's in
     * numerical order.
     */
    static class ExperimentInfo implements Comparable<ExperimentInfo> {
        /** The experiment name, e.g., "exp12". */
        public String name;

        /** The experiment number, e.g., 12. */
        public int number; // The experiment number

        /** True if this experiment is currently in any of the VnrmBGs. */
        public boolean isLoaded = false;

        /**
         * To sort in numerical order.
         */
        public int compareTo(ExperimentInfo other) {
            return number - other.number;
        }
    }


    /**
     * A filename filter that selects files that look like legal
     * experiment directories.
     */
    static class ExpNameFilter implements FilenameFilter {

        /**
         * Determine if a file is an experiment directory.
         * This implementation only looks to see if the name is
         * of the form "expNNN", where NNN is a positive decimal
         * integer of any length.
         * @return True if this is an experiment directory.
         */
        public boolean accept(File dir, String name) {
            return (getExpNumber(name) > 0);
        }
    }

}

