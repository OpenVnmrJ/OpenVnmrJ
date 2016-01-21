/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.templates;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

import vnmr.util.Messages;
import static vnmr.templates.ProtocolBuilder.*;

/**
 * Singleton class with methods to build the Study Queue tree by examining
 * files in Vnmr.
 */
public class SQBuilder {
    private static SQBuilder m_sqBuilder = null;

    /**
     * Constructor is private; called only by getSQBuilder.
     */
    private SQBuilder() {
    }

    public static SQBuilder getSQBuilder() {
        if (m_sqBuilder == null) {
            m_sqBuilder = new SQBuilder();
        }
        return m_sqBuilder;
    }

    /**
     * Get the info necessary to show the items queued according to
     * the given directory. The directory is "ChemPack" style found
     * in, e.g., vnmrsys/studies/exp1.
     * @param dir The "studies/expn" directory with the queue info.
     * @return The queue items to show from the given directory. Never null.
     */
    public ArrayList<VActionElement> getQueuedActions(String dir) {
        long t0 = System.currentTimeMillis();

        // Get list of queued experiments (status not valid)
        ArrayList<VActionElement> actions = getEXPLIST(dir + "/EXPLIST");

        // Fill in correct status of each experiment
        for (VActionElement act : actions) {
            String id = act.getAttribute(ATTR_ID);
            String propPath = dir + "/tmpstudy/info/" + id + "/prop";
            Map<String,String> props = parseKeyValFile(new File(propPath));
            act.setAttribute(ATTR_STATUS, props.get("status"));
        }

        long now = System.currentTimeMillis();
        Messages.postDebug("SQBuilder",
                           "getQueuedActions: Time=" + (now - t0) + "ms");
        return actions;
    }

//    public ArrayList<VActionElement> getDataActions(String dir) {
//        // Get list of acquired experiments (ID not valid)
//        ArrayList<VActionElement> actions = getEXPLIST(dir + "/macdir/ACQlist");
//
//        // Fill in status of each experiment
//        for (VActionElement act : actions) {
//            String id = act.getAttribute(ATTR_ID);
//            String propPath = dir + "/tmpstudy/info/" + id + "/prop";
//            Map<String,String> props = parseKeyValFile(new File(propPath));
//            act.setAttribute(ATTR_STATUS, props.get("status"));
//        }
//        return actions;
//    }

    /**
     * Get the items queued according to the given EXPLIST file. In
     * "ChemPack" style, this is found in, e.g., vnmrsys/studies/exp1/EXPLIST.
     * 
     * @param path The path of the EXPLIST file.
     * @return The items in the file, with the properties from the file
     * filled in. Never null.
     */
    private ArrayList<VActionElement> getEXPLIST(String path) {
        ArrayList<VActionElement> actions = new ArrayList<VActionElement>();
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader(path));
            String str;
            while ((str = reader.readLine()) != null) {
                String[] tokens = str.split("[ \t]+");
                if (tokens.length > 3) {
                    VActionElement action = new VActionElement();
                    action.setAttribute(ATTR_TITLE, tokens[0]);
                    action.setAttribute(ATTR_TIME, tokens[1]);
                    action.setAttribute(ATTR_EXP, tokens[2]);
                    action.setAttribute(ATTR_ID, tokens[3]);
                    actions.add(action);
                } else {
                    Messages.postDebug("Bad line in "
                                       + path + ": \"" + str + "\"");
                }
            }
        } catch (FileNotFoundException e) {
            //Messages.writeStackTrace(e);
        } catch (IOException e) {
            Messages.writeStackTrace(e);
        } finally {
            try {
                if (reader != null) {
                    reader.close();
                }
            } catch (IOException e) { }
        }
        
        return actions;
    }

    /**
     * Key is separated from Value by white space. Value may contain
     * white space.
     * @param file The file to parse.
     * @return A Map of the Values, indexed by the Keys. Never null.
     */
    private Map<String,String> parseKeyValFile(File file) {
        Map<String,String> map = new TreeMap<String,String>();
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader(file));
            String str;
            while ((str = reader.readLine()) != null) {
                String[] keyVal = str.split("[ \t]+", 2);
                if (keyVal.length == 2) {
                    map.put(keyVal[0], keyVal[1]);
                } else {
                    Messages.postDebug("Bad key/value line in "
                                       + file.getAbsolutePath()
                                       + ": \"" + str + "\"");
                }
            }
        } catch (FileNotFoundException e) {
            Messages.writeStackTrace(e);
        } catch (IOException e) {
            Messages.writeStackTrace(e);
        } finally {
            try {
                if (reader != null) {
                    reader.close();
                }
            } catch (IOException e) { }
        }
        return map;
    }
    
    /**
     * Run an SQBuilder test a few times and print out the results and
     * elapsed time for each test.
     * The first command line argument is the name of the test.
     * Remaining arguments depend on the test.
     * @param args The command line arguments.
     */
    public static void main(String[] args) {
        if (args.length < 2) {
            System.out.println("Arg options: keyval path");
        }
        SQBuilder builder = getSQBuilder();
        for (int i = 0; i < 3; i++) {
            long t0 = System.currentTimeMillis();
            runTest(args, builder);
            long now = System.currentTimeMillis();
            System.out.println("Elapsed time: " + (now - t0) + "ms");
        }
    }

    /**
     * Run an SQBuilder test.
     * @param args The command line arguments.
     * @param builder The SQBuilder instance.
     */
    private static void runTest(String[] args, SQBuilder builder) {
        if (args[0].equalsIgnoreCase("keyval")) {
            System.out.println("Parse key-value file: " + args[1]);
            File file = new File(args[1]);
            Map<String, String> map = builder.parseKeyValFile(file);
            Set<Map.Entry<String, String>> entries = map.entrySet();
            for (Map.Entry<String, String> entry : entries) {
                System.out.println("Key=\"" + entry.getKey()
                                   + "\", Value=\"" + entry.getValue() + "\"");
            }
        } else if (args[0].equalsIgnoreCase("queue")) {
            System.out.println("Get queued actions in: " + args[1]);
            ArrayList<VActionElement> acts = builder.getQueuedActions(args[1]);
            for (VActionElement act : acts) {
                System.out.println(act);
            }
        }
    }
}
