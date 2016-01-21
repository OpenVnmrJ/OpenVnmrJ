/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

import javax.swing.SwingUtilities;

import org.w3c.dom.NamedNodeMap;

import vnmr.templates.ProtocolBuilder;
import vnmr.templates.SQActionRenderer;
import vnmr.templates.SQBuild;
import vnmr.templates.SQNode;
import vnmr.templates.SQNodeList;
import vnmr.templates.VElement;
import vnmr.util.DebugOutput;
import vnmr.util.FileChangeEvent;
import vnmr.util.FileListener;
import vnmr.util.FileWatcher;
import vnmr.util.Messages;
import vnmr.util.QuotedStringTokenizer;
import vnmr.util.Util;
import vnmr.util.VnmrParameter;

import static vnmr.templates.ProtocolBuilder.*;
import static vnmr.ui.StudyQueue.*;
import static vnmr.util.FileChangeEvent.*;

public class SQUpdater implements FileListener {

    private List<QInfo> m_qInfos;
    private String m_archiveDir;
    private String m_autodir;
    private String m_curSQExp;
    private String m_sqInfoDir;
    private String m_dataDir;
    private String m_sample;
    //private String m_sampleNumber;
    private String m_queueWatchPath;
    private String m_queueWatchPath2;
    private String m_autoWatchPath;
    private String m_dataWatchPath;
    private StudyQueue m_studyQueue;
    private String m_updateCmd = "";

    private String m_mode;
    private String m_activeLocdir;

    static private FileWatcher m_currentFileWatcher;


    public SQUpdater(StudyQueue sq, String mode, String svfdir, String sample) {
        m_studyQueue = sq;
        m_mode = mode;
        m_dataDir = svfdir + "/" + sample;
        m_dataWatchPath = m_dataDir + "/dirinfo/macdir/ACQlist";
        String[] paths = {m_dataWatchPath};
        quitWatching();
        m_currentFileWatcher = new FileWatcher(this, paths);
        m_currentFileWatcher.start();
    }

    public SQUpdater(StudyQueue sq, String mode, String studydir,
                     String autodir, String svfdir, String cmd) {
        this(sq, mode, studydir, autodir, svfdir);
        m_updateCmd = cmd;
    }

    public SQUpdater(StudyQueue sq, String[] args) {
        m_studyQueue = sq;
        m_mode = args[0];
        if (m_mode.equalsIgnoreCase("build")) {
            initBuild(args);
        } else if (m_mode.equalsIgnoreCase("active")) {
            initActive(args);
        }
    }


    private void initBuild(String[] args) {
        m_curSQExp = args[1];
        m_autodir = args[2];
        m_archiveDir = args[3];
        m_sample = args[4];
        m_updateCmd = args[5];

        ArrayList<String> alPaths = new ArrayList<String>();
        m_autoWatchPath = m_autodir + "/doneQ";
        //m_queueWatchPath = m_curSQExp + "/EXPLIST";
        //m_queueWatchPath2 = m_curSQExp + "/tmpstudy/enterSQ";
        //m_dataDir = getDataDir(m_autodir);
        m_dataWatchPath = m_archiveDir + "/" + m_sample + "/dirinfo/macdir/ACQlist";
        alPaths.add(m_dataWatchPath);
        alPaths.add(m_autoWatchPath);
        String[] paths = alPaths.toArray(new String[0]);

        quitWatching();
        m_currentFileWatcher = new FileWatcher(this, paths);
        m_currentFileWatcher.start();
        updateStatsBuild(true);
    }

    private void initActive(String[] args) {
        m_curSQExp = args[1];
        m_autodir = args[2];
        m_archiveDir = args[3];
        m_sample = args[4];
        m_updateCmd = args[5];

        ArrayList<String> alPaths = new ArrayList<String>();
       // m_autoWatchPath = m_autodir + "/enterQ.macdir/fidlog";
        m_autoWatchPath = m_autodir + "/doneQ";
        String autoWatchPath2 = m_autodir + "enterQ.macdir/cpdoneQ";
        m_dataWatchPath = m_archiveDir + "/" + m_sample + "/dirinfo/macdir/ACQlist";
        alPaths.add(m_autoWatchPath);
        alPaths.add(autoWatchPath2);
        alPaths.add(m_dataWatchPath);
        String[] paths = alPaths.toArray(new String[0]);

        quitWatching();
        m_currentFileWatcher = new FileWatcher(this, paths);
        m_currentFileWatcher.start();
        //updateStatsActive();
    }

    public SQUpdater(StudyQueue sq, String mode, String studydir,
                     String autodir, String svfdir) {
        m_studyQueue = sq;
        m_mode = mode;
        m_curSQExp = studydir;
        m_autodir = autodir;
        m_archiveDir = svfdir;

        ArrayList<String> alPaths = new ArrayList<String>();
        m_autoWatchPath = m_autodir + "/doneQ";
        m_queueWatchPath = m_curSQExp + "/EXPLIST";
        m_queueWatchPath2 = m_curSQExp + "/tmpstudy/enterSQ";
        m_dataDir = getDataDir(m_autodir);
        m_dataWatchPath = m_archiveDir + "/" + m_dataDir + "/dirinfo/macdir/ACQlist";
        if ("auto".equals(mode)) {
            //alPaths.add(m_dataWatchPath);
            alPaths.add(m_autoWatchPath);
        } else if ("build".equals(mode)) {
            alPaths.add(m_dataWatchPath);
            alPaths.add(m_autoWatchPath);
        } else {
            alPaths.add(m_autoWatchPath);
            alPaths.add(m_queueWatchPath);
            alPaths.add(m_queueWatchPath2);
            alPaths.add(m_dataWatchPath);
        }
        String[] paths = alPaths.toArray(new String[0]);

        quitWatching();
        m_currentFileWatcher = new FileWatcher(this, paths);
        m_currentFileWatcher.start();
    }

    static public boolean processCommand(StudyQueue sq, String[] args) {
        if (args.length < 1) {
            return false;
        }
        String cmd = args[0];

        if (cmd.equalsIgnoreCase("off")) {
            quitWatching();
        } else if (cmd.equalsIgnoreCase("bgstudy")) {
            new SQUpdater(sq, cmd, args[1], args[2]);
        } else if (cmd.equalsIgnoreCase("auto")
                   || cmd.equalsIgnoreCase("more")
                   || cmd.equalsIgnoreCase("submit")
                   || cmd.equalsIgnoreCase("build")
                   || cmd.equalsIgnoreCase("active")
                   )
        {
            if (cmd.equalsIgnoreCase("build")
                || cmd.equalsIgnoreCase("active"))
            {
                new SQUpdater(sq, args);
            } else if (args.length == 4) {
                new SQUpdater(sq, cmd, args[1], args[2], args[3]);
            } else if (args.length == 5) {
                new SQUpdater(sq, cmd, args[1], args[2], args[3], args[4]);
            } else {
                return false;
            }
        }
//        SQUpdater updater = new SQUpdater(sq, studydir, autodir, svfdir);
        return true;
    }


    /**
     *
     */
    private static void quitWatching() {
        if (m_currentFileWatcher != null) {
            //Util.sendToVnmr("write('line3','QUIT WATCHING')");
            m_currentFileWatcher.quit();
        }
    }

    @Override
    public void fileChanged(FileChangeEvent event) {
        int type = event.getEventType();
        String changedPath = event.getPath();
        // NB: TOUCHED ==> no change in file contents
        if (type != FILE_TOUCHED || m_mode.equals("auto")) {
            Messages.postDebug("SQUpdater", "FILE CHANGED: " + changedPath);
            if ((m_mode.equals("submit")
                 || m_mode.equals("more")
                 //|| m_mode.equals("auto")
                 || m_mode.equals("build"))
                && changedPath.equals(m_autoWatchPath))
            {
                String dataDir = getDataDir(m_autodir);
                if (dataDir != null && dataDir.length() > 0 && !dataDir.equals(m_dataDir)) {
                    m_currentFileWatcher.removePath(m_dataWatchPath);
                    Messages.postDebug("SQUpdater","Removed " + m_dataWatchPath);
                    m_dataDir = dataDir;
                    m_dataWatchPath = m_archiveDir + "/" + m_dataDir
                            + "/dirinfo/macdir/ACQlist";
                    m_currentFileWatcher.addPath(m_dataWatchPath);
                    Messages.postDebug("SQUpdater","Added " + m_dataWatchPath);
                }
            }
            if (m_mode.equals("auto") && type != FILE_INITIAL) {
                updateStatsAuto();
            } else if (m_mode.equals("bgstudy")) {
                updateStatsBgstudy();
            } else if (m_mode.equals("submit")) {
                updateStatsSubmit();
            } else if (m_mode.equals("more")) {
                updateStatsMore();
            } else if (m_mode.equals("build") && type != FILE_INITIAL) {
                updateStatsBuild(false);
            } else if (m_mode.equals("active") && type != FILE_INITIAL) {
                updateStatsActive();
            }
        }
    }

    public void updateStatsAuto() {
        // Have chempack update the automation run display
        Util.sendToVnmr(m_updateCmd);
        // Run this in the Event Thread because it updates the GUI
//        SwingUtilities.invokeLater(new Runnable() {
//            public void run() { updateStatsAutoUI(); } }
//        );
    }

    /**
    *
    */
   public void updateStatsAutoUI() {
       // (It's all been done by chempack)
   }

    public void updateStatsBgstudy() {
        // Run this in the Event Thread because it updates the GUI
        SwingUtilities.invokeLater(new Runnable() {
            public void run() { updateStatsBgstudyUI(); } }
        );

        // Workaround to avoid race condition
        try {
            Thread.sleep(2000);
        } catch (InterruptedException ie) {}
        // Run this in the Event Thread because it updates the GUI
        SwingUtilities.invokeLater(new Runnable() {
            public void run() { updateStatsBgstudyUI(); } }
        );
    }

    /**
    *
    */
    public void updateStatsBgstudyUI() {
       Messages.postDebug("SQUpdater","updateStatsBgstudyUI");
       List<String> nodes = getNodeIdsFromSQ(ProtocolBuilder.ACTIONS);
       Map<String, String> expStats = getExpStats(new File(m_dataWatchPath));
       File f = new File(m_dataDir + "/dirinfo/parlib/parliblist");
       Map<String, String> fidNames = getFidNames(f);
//       boolean done = nodes.size() > 0;
       ProtocolBuilder mgr = m_studyQueue.getMgr();
       for (String node : nodes) {
           VElement vnode = mgr.getElement(node);
           String title = mgr.getAttribute(vnode, ATTR_TITLE);
           String stat = getValueFromTitle(title, expStats);
           if (stat != null) {
               mgr.setAttribute(vnode, ATTR_STATUS, stat);
               if (stat.equals(SQ_COMPLETED)) {
                   String fidname = getValueFromTitle(title, fidNames);
                   if (fidname != null) {
                       mgr.setAttribute(vnode, ATTR_TITLE, fidname);
                   }
               }
           }

//           // See if this node still may change
//           stat = m_studyQueue.getAttribute(node, ATTR_STATUS);
//           if (stat != null) {
//               if (stat.equalsIgnoreCase(READY)
//                   || stat.equalsIgnoreCase(QUEUED)
//                   || stat.equalsIgnoreCase(EXECUTING)
//                   )
//               {
//                   done = false;
//               }
//           }
       }

//       // Clean up if we're done
//       // Keep getting false positives on this
//       // ... check how many updates we've done?
//       if (done) {
//           Util.sendToVnmr("SQWatch('off')");
//           Util.sendToVnmr("xmhaha('refreshSQ')");
//       }
   }

    private List<String> getNodeIdsFromSQ(int nodeType) {
       ProtocolBuilder mgr = m_studyQueue.getMgr();
       ArrayList<VElement> elementList;
       elementList = mgr.getElements(null, nodeType);
       ArrayList<String> ids = new ArrayList<String>();
       for (VElement elem : elementList) {
           ids.add(elem.getAttribute(ProtocolBuilder.ATTR_ID));
       }
       return ids;
   }

    private ArrayList<NamedNodeMap>
            getNodeAttributesFromSQ(Iterable<String> nodeIds) {

        ArrayList<NamedNodeMap> list = new ArrayList<NamedNodeMap>();
        for (String nodeId : nodeIds) {
            list.add(getNodeAttributesFromSQ(nodeId));
        }
        return list;
    }

    private SQNodeList getNodeInfoFromSQ(Iterable<String> nodeIds) {
        SQNodeList list = new SQNodeList();
        for (String nodeId : nodeIds) {
            list.add(getNodeInfoFromSQ(nodeId));
        }
        return list;
    }

    private NamedNodeMap getNodeAttributesFromSQ(String nodeId) {
        ProtocolBuilder mgr = m_studyQueue.getMgr();
        VElement velem = mgr.getElement(nodeId);
        NamedNodeMap attrs = velem.getAttributes();
        return attrs;
    }

    private SQNode getNodeInfoFromSQ(String nodeId) {
        SQNode nodeInfo = null;
        ProtocolBuilder mgr = m_studyQueue.getMgr();
        VElement velem = mgr.getElement(nodeId);
        if (velem != null) {
            nodeInfo = new SQNode();
            for (String attr : SQNode.getStandardAttributes()) {
                String value = mgr.getAttribute(velem, attr);
                nodeInfo.setAttr(attr, value);
            }
        }
//        String when = "";
//        String[] whenEnums = {"_day", "_night"};
//        for (String w : whenEnums) {
//            if (psLabel.contains(w)) {
//                when = w;
//                psLabel = psLabel.replace(w, "");
//            }
//            if (title.contains(w)) {
//                when = w;
//                title = title.replace(w, "");
//            }
//        }
        return nodeInfo;
    }

    private void printAllNodeAttributes(List<String> ids) {
        List<NamedNodeMap> maps = getNodeAttributesFromSQ(ids);
        for (int j = 0; j < ids.size(); j++) {
            NamedNodeMap map = maps.get(j);
            String id = ids.get(j);
            Messages.postDebug("\nNode " + id + ":");
            for (int i = 0; i < map.getLength(); i++) {
                org.w3c.dom.Node attr = map.item(i);
                String name = attr.getNodeName();
                String value = attr.getNodeValue();
                Messages.postDebug("  " + name + "=" + value);
            }
        }
    }


    private boolean updateNodeInSQ(String id,
                                   SQNode srcNode,
                                   List<String> changed) {
        boolean ok = false;
        ProtocolBuilder mgr = m_studyQueue.getMgr();
        VElement elem = mgr.getElement(id);
        if (elem != null) {
            for (String attr : changed) {
                String value = srcNode.getAttr(attr);
                mgr.setAttribute(elem, attr, value);
            }
            ok = true;
        }
        return ok;
    }

    /**
     *  Set properties in studies "prop" file for the given SQ node.
     *  A typical location:
     *  <BR><CODE>
     *  vnmrsys/studies/exp2/tmpstudy/info/n002/prop
     *  </CODE>
     * @param cursqexp Experiment directory for the SQ.
     * @param id The ID of the SQ node.
     * @param node The node to write about.
     */
    private void writePropFile(String cursqexp,
                               String id, SQNode node) {

        // The attributes that are written into the "props" file
        // (e.g., studies/exp2/tmpstudy/info/n002/prop), in the order
        // they should appear. Order is important, because macros
        // may read the value of a particular line number without checking
        // the key.
        // In xmaction it is noted:
        // "$id=sqval[1]    $type=sqval[2] $status=sqval[3]  - fixed position"
        // (ID is added to the front of the list after reading the prop file.)
        final String[] propAttributes = {
                                         ATTR_TYPE,
                                         ATTR_STATUS,
                                         ATTR_LOCK,
                                         ATTR_TITLE,
                                         ATTR_EXP,
                                         ATTR_TIME,
                                         ATTR_MACRO,
                                         ATTR_DATA,
        };

        String path = cursqexp + "/tmpstudy/info/" + id;
        File filePath = new File(path, "/prop");
        PrintWriter out = null;
        try {
            new File(path).mkdirs();
            out = new PrintWriter(filePath);
            for (String name : propAttributes) {
                out.println(name + " " + node.getAttr(name));
            }
        } catch (FileNotFoundException e) {
            Messages.postDebug("Could not write file "
                               + filePath.getPath());
        } finally {
            try {
                out.close();
            } catch (Exception e) {}
        }
    }

    public void updateStatsSubmit() {
        // Run this in the Event Thread because it updates the GUI
        SwingUtilities.invokeLater(new Runnable() {
            public void run() { updateStatsSubmitUI(); } }
        );
    }

    /**
    *
    */
   public void updateStatsSubmitUI() {

       File explist = new File(m_curSQExp + "/EXPLIST");
       File enterSQ = new File(m_curSQExp + "/tmpstudy/enterSQ");
       Map<String, String> titleToIdMap = getTitleToIdMap(explist, enterSQ);

       String curSampleDir = getDataDir(m_autodir);

       File acqlist = new File(m_archiveDir + "/" + curSampleDir
                               + "/dirinfo/macdir/ACQlist");
       Map<String, String> expStats = getExpStats(acqlist);

       Set<String> titles = titleToIdMap.keySet();
       for (String title : titles) {
           String node = titleToIdMap.get(title);
           String status = expStats.get(title);
           if (status == null) {
               status = SQ_READY;
           } else if (status.equals(SQ_ACTIVE)) {
               status = SQ_EXECUTING;
           }
           m_studyQueue.processCommand(SET + " " + node + " "
                                       + ATTR_STATUS + " " + status);
       }
       // Set the parent title to show the current sample number
//       m_studyQueue.processCommand(SET + " tmpstudy " + ATTR_TITLE
//                                   + " \"Sample " + m_sampleNumber + "\"");
//       m_studyQueue.processCommand(SET + " tmpstudy " + ATTR_TOOLTEXT
//               + " \"\"");
   }

    public void updateStatsMore() {
        // Run this in the Event Thread because it updates the GUI
        SwingUtilities.invokeLater(new Runnable() {
            public void run() { updateStatsMoreUI(); } }
        );
    }

    public void updateStatsMoreUI() {

        List<String> stats = null;
        String curSampleDir = getDataDir(m_autodir);

        File acqlist = new File(m_archiveDir + "/" + curSampleDir
                + "/dirinfo/macdir/ACQlist");
        stats = getStatList(acqlist);

        File enterSQ = new File(m_curSQExp + "/tmpstudy/enterSQ");
        List<String> nodes = getNodeList(enterSQ);

        int len = nodes.size();
        int statsLen = stats.size();
        for (int i = 0; i < len; i++) {
            String node = nodes.get(i);
            String status = i < statsLen ? stats.get(i) : SQ_READY;
            if (status.equals(SQ_ACTIVE)) {
                status = SQ_EXECUTING;
            }
            m_studyQueue.processCommand(SET + " " + node + " "
                    + ATTR_STATUS + " " + status);
        }
    }

    public void updateStatsBuild(boolean isInitial) {
        String archsamp = m_archiveDir + "/" + m_sample;
        String samplename = new File(archsamp).getName();
        List<String> filelist = new ArrayList<String>();
        filelist.add("fidlog");
        filelist.add("ACQlist");
        filelist.add("explist");
        filelist.add("EXPLIST");
        filelist.add("LONGLIST");
        SQBuild builder = new SQBuild(archsamp, samplename, m_autodir,
                filelist, false);
        String locdir = builder.getActiveLocdir();
        if (locdir != null) {
            locdir += "/current.fid/fid";
            if (!locdir.equals(m_activeLocdir)) {
                Messages.postDebug("SQActive","New active locdir: " + locdir);
                m_currentFileWatcher.removePath(m_activeLocdir);
                m_activeLocdir = locdir;
                m_currentFileWatcher.addPath(m_activeLocdir);
            }
        }
        final SQNodeList nodeList = builder.getNodeList();
        final boolean initial = isInitial;

        // Run this in the Event Thread because it updates the GUI
        SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                updateStatsBuildUI(initial, nodeList, m_curSQExp);
                } }
        );
    }

    public void updateStatsActive() {
        // Check if sample has changed:
        //String archsamp = m_archiveDir + "/" + m_sample;
        String newSample = getLastSampleFromFidlog(m_autodir);
        newSample = getDataDir(m_autodir);
        //String newArchdir = getArchDirFromGlobal(m_autodir, m_archiveDir);
        Messages.postDebug("SQUpdateActive",
                "updateStatsActive: m_sample=" + m_sample);
        Messages.postDebug("SQUpdateActive",
                "................. newSample=" + newSample);

        if (newSample.equals(m_sample)) {
            updateStatsBuild(false);
            Messages.postDebug("SQUpdateActive", "SAME SAMPLE");
        } else {
            m_sample = newSample;
            Util.sendToVnmr(m_updateCmd);
            Messages.postDebug("SQUpdateActive", "NEW SAMPLE");
        }
    }

    public void updateStatsBuildUI(boolean isInitial,
                                   SQNodeList nodeList, String cursqexp) {
        if (DebugOutput.isSetFor("AllSQUpdater")) {
            Messages.postDebug("--------------------- Calc node list:");
            SQBuild.printNodeList(nodeList);
            Messages.postDebug("--------------------- End calc nodes");
        }
        // TODO: All of updateStatsBuildUI needs cleaning up.
        // Get list of action nodes visible in the SQ
        List<String> visibleIds = getNodeIdsFromSQ(ProtocolBuilder.ACTIONS);
        String sampleInfoId = removeSampleInfoNode(visibleIds);
        if (DebugOutput.isSetFor("AllSQUpdater") && sampleInfoId != null) {
            Messages.postDebug("SampleInfo node ignored");
        }

        if (!isInitial && nodeList.size() != visibleIds.size()) {
            Messages.postDebug("SQUpdater", "calc nodes: " + nodeList.size()
                               + ", vis nodes: " + visibleIds.size());
            Util.sendToVnmr(m_updateCmd);
            if (DebugOutput.isSetFor("AllSQUpdater")) {
                Messages.postDebug(visibleIds.size() + " nodes visible in SQ; "
                        + nodeList.size() + " calculated");

                // Print out the visibleNodes
                Messages.postDebug("--------------------- Visible node list:");
                SQNodeList visibleNodes = getNodeInfoFromSQ(visibleIds);
                for (SQNode node : visibleNodes) {
                    Messages.postDebug(node.toString());
                }
                Messages.postDebug("--------------------- End visible nodes");
            }

        } else {
            // Get all attributes from the visible SQ nodes
            // Build list of the visible SQ Nodes
            SQNodeList visibleNodes = getNodeInfoFromSQ(visibleIds);

            if (DebugOutput.isSetFor("AllSQUpdater")) {
                Messages.postDebug("--------------------- Visible node attrs:");
                printAllNodeAttributes(visibleIds);
                Messages.postDebug("--------------------- End visible attrs");
            }

            // Find differences between nodeList and SQ
            boolean isDiff = false;
            if (DebugOutput.isSetFor("AllSQUpdater")) {
                Messages.postDebug("--------------------- Node diffs:");
            }
            for (int i = 0; i < nodeList.size(); i++) {
                SQNode node = nodeList.get(i);
                SQNode vnode = visibleNodes.get(i);
                String diffs = vnode.diff(node);
                if (diffs.length() > 0) {
                    isDiff = true;
                    if (DebugOutput.isSetFor("AllSQUpdater")) {
                        Messages.postDebug("Node " + i + " ("
                                + visibleIds.get(i) + ") differs:");
                        Messages.postDebug(diffs);
                    }

//                    // TODO: May not need to check this stuff
//                    String vstat = vnode.getStatus();
//                    String cstat = node.getStatus();
//                    Boolean completed = (cstat.equals(COMPLETED)
//                                         && (vstat.equals(EXECUTING)
//                                             || vstat.equals(ACTIVE)));
//                    String vtitle = vnode.getTitle().trim();
//                    // Displayed title may have other stuff after true title
//                    String[] titleTokens = vtitle.split(" +", 2);
//                    if (node.getTitle().contains(titleTokens[0]) || completed) {

                    // Set some SQ node attributes to match calculated node
                    List<String> changed = vnode.update(node);
                    if (changed.size() > 0) {
                        String nodeId = visibleIds.get(i);
                        updateNodeInSQ(nodeId, vnode, changed);
                        writePropFile(cursqexp, nodeId, vnode);
                    }
//                    }
                }
            }
            if (DebugOutput.isSetFor("AllSQUpdater")) {
                Messages.postDebug("--------------------- End node diffs");
            }
            if (!isDiff) {
                if (DebugOutput.isSetFor("SQUpdater")) {
                    Messages.postDebug("NO DIFFERENCES");
                }
//            } else {
//                m_sqInfoDir = m_curSQExp + "/tmpstudy/info";
            }
        }
    }

    private String removeSampleInfoNode(List<String> ids) {
        String id = null;
        if (ids.size() > 0) {
            ProtocolBuilder mgr = m_studyQueue.getMgr();
            VElement vnode = mgr.getElement(ids.get(0));
            String title = mgr.getAttribute(vnode, ATTR_TITLE);
            if (title.contains("SampleInfo")) {
                id = ids.remove(0);
            }
        }
        return id;
    }

    private String getValueFromTitle(String title, Map<String,String> map) {
        String value = null;
        Collection<String> keys = map.keySet();
        for (String key : keys) {
            // NB: title attribute may have other stuff after the true title
            if (title.startsWith(key)) {
                // title is good enough match to the key
                value = map.get(key);
            }
        }
        return value;
    }

    private List<String> getStatList(File acqlist) {
        List<String> list = new ArrayList<String>();
        BufferedReader in = null;
        // Look for Title and node IDs in EXPLIST file
        try {
            in = new BufferedReader(new FileReader(acqlist));
            String line;
            while ((line = in.readLine()) != null) {
                String[] toks = line.split(" +");
                int ntoks = toks.length;
                if (ntoks == 5) {
                    list.add(toks[4]);
                }
            }
        } catch (FileNotFoundException e) {
        } catch (IOException e) {
        } finally {
            try {
                in.close();
            } catch (Exception e) {}
        }
        // Any "Active" nodes before the last one are errors
        boolean gotActive = false;
        for (int i = list.size() - 1; i >= 0; --i) {
            if ("Active".equalsIgnoreCase(list.get(i))) {
                if (gotActive) {
                    list.set(i, "Error");
                } else {
                    gotActive = true;
                }
            }
        }
        return list;
    }


    private List<String> getNodeList(File enterSQ) {
        List<String> list = new ArrayList<String>();
        BufferedReader in = null;
        // Look for Title and node IDs in enterSQ file
        try {
            in = new BufferedReader(new FileReader(enterSQ));
            String line;
            while ((line = in.readLine()) != null) {
                String[] toks = line.split(" +");
                int ntoks = toks.length;
                if (ntoks >= 2) {
                    String title = toks[1];
                    if (!title.equals("parent") && !title.equals("SampInfo")) {
                        list.add(toks[0]); // The node ID
                    }
                }
            }
        } catch (FileNotFoundException e) {
        } catch (IOException e) {
        } finally {
            try {
                in.close();
            } catch (Exception e) {}
        }
        return list;
    }

    private String getLastSampleFromFidlog(String autodir) {
        String sample = "";
        String path = autodir + "/enterQ.macdir/fidlog";

        BufferedReader in = null;
        try {
            in = new BufferedReader(new FileReader(path));
            String line;
            while ((line = in.readLine()) != null) {
                QuotedStringTokenizer toker = new QuotedStringTokenizer(line);
                if (toker.countTokens() == 2) {
                    toker.nextToken();
                    sample = toker.nextToken();
                }
            }
        } catch (FileNotFoundException e) {
        } catch (IOException e) {
        } finally {
            try {
                in.close();
            } catch (Exception e) {}
        }
        sample = new File(sample).getParent();
        return sample;
    }

    private String getArchDirFromGlobal(String autodir, String archdir) {
        String globalPath = autodir + "/enterQ.macdir/currentsampleglobal";
        Map<String, VnmrParameter> pmap;
        String[] pars = {"archivedir"};
        pmap = VnmrParameter.readParameters(globalPath, pars);
        if (pmap != null) {
            archdir = pmap.get("archivedir").getStringValue();
        }

        return archdir;
    }

    /**
     * Read all the info in the "doneQ" in the given "autodir".
     * @return The name of the current sample directory in "svfdir".
     */
    private String getDataDir(String autodir) {
        String curSampleDir = "";
        File doneQ = new File(autodir + "/doneQ");
        m_qInfos = getQFileInfo(doneQ);
        int nInfos = m_qInfos.size();
        if (nInfos > 0) {
            QInfo curInfo = m_qInfos.get(m_qInfos.size() - 1);
            //m_sampleNumber = curInfo.sampleNumber;
            curSampleDir = curInfo.sampleDir;
            Messages.postDebug("SQUpdater", "LAST AUTO STATUS = "
                               + curInfo.status);
        }
        return curSampleDir;
    }

    /**
     * Construct a map of experiment stats, keyed by title.
     * @param acqlist Path to the "ACQlist" file under "dirinfo".
     * @return The map.
     */
    static public Map<String, String> getExpStats(File acqlist) {
        Map<String, String> map = new TreeMap<String, String>();
        if (acqlist == null) {
            return map;
        }
        BufferedReader in = null;
        try {
            in = new BufferedReader(new FileReader(acqlist));
            String line;
            while ((line = in.readLine()) != null) {
                String[] toks = line.split(" +");
                int ntoks = toks.length;
                if (ntoks == 5) {
                    // title = toks[0]
                    String stat = toks[4];
                    if (stat.equalsIgnoreCase("Active")) {
                        stat = "Executing";
                    }
                    map.put(toks[0], stat);
                }
            }
        } catch (FileNotFoundException e) {
        } catch (IOException e) {
        } finally {
            try {
                in.close();
            } catch (Exception e) {}
        }
        return map;
    }

    /**
     * Construct a map of experiment fid names, keyed by title.
     * @param parliblist Path to the "parliblist" file under "dirinfo".
     * @return The map.
     */
    static public Map<String, String> getFidNames(File parliblist) {
        Map<String, String> map = new TreeMap<String, String>();
        if (parliblist == null) {
            return map;
        }
        BufferedReader in = null;
        try {
            in = new BufferedReader(new FileReader(parliblist));
            String line;
            while ((line = in.readLine()) != null) {
                String[] toks = line.split("[: ]+", 2);
                int ntoks = toks.length;
                if (ntoks == 2) {
                    // title = toks[0]
                    int idx = toks[1].lastIndexOf("/");
                    String fidname = toks[1].substring(idx + 1);
                    map.put(toks[0], fidname);
                }
            }
        } catch (FileNotFoundException e) {
        } catch (IOException e) {
        } finally {
            try {
                in.close();
            } catch (Exception e) {}
        }
        return map;
    }

    static public Map<String, String> getTitleToIdMap(File explist,
                                                      File enterSQ) {
        Map<String, String> map = new TreeMap<String, String>();
        BufferedReader in = null;
        // Look for Title and node IDs in EXPLIST file
        try {
            in = new BufferedReader(new FileReader(explist));
            String line;
            while ((line = in.readLine()) != null) {
                String[] toks = line.split(" +");
                int ntoks = toks.length;
                if (ntoks == 5 && !toks[3].startsWith("xx")) {
                    map.put(toks[0], toks[3]);
                }
            }
        } catch (FileNotFoundException e) {
        } catch (IOException e) {
        } finally {
            try {
                in.close();
            } catch (Exception e) {}
        }

        if (map.size() == 0) {
            // Nothing found in EXPLIST - try enterSQ
            try {
                in = new BufferedReader(new FileReader(enterSQ));
                String line;
                while ((line = in.readLine()) != null) {
                    String[] toks = line.split(" +");
                    int ntoks = toks.length;
                    if (ntoks == 4) {
                        String title = toks[3];
                        int idx = title.lastIndexOf("/");
                        if (idx >= 0) {
                            title = title.substring(idx + 1);
                        }
                        map.put(toks[0], title);
                    }
                }
            } catch (FileNotFoundException e) {
            } catch (IOException e) {
            } finally {
                try {
                    in.close();
                } catch (Exception e) {}
            }

        }
        return map;
    }

    /**
     * Get info for all the entries in a xxxxQ file.
     * (Currently we only use the last entry and only the "sampleDir".)
     * @param qPath The path to the xxxxQ file to read.
     * @return A list containing the QInfo for each element in the file.
     */
    static private List<QInfo> getQFileInfo(File qPath) {
        List<QInfo> infos = new ArrayList<QInfo>();
        BufferedReader in = null;
        try {
            in = new BufferedReader(new FileReader(qPath));
            String line;
            QInfo info = new QInfo();
            while ((line = in.readLine()) != null) {
                if (line.startsWith("----------")) {
                    if (!(info.sampleNumber == QInfo.NO_SAMPLE_NUMBER)) {
                        infos.add(info);
                        info = new QInfo();
                    }
                }
                String[] toks = line.split(":", 2);
                int ntoks = toks.length;
                if (ntoks == 2) {
                    String key = toks[0].trim();
                    String value = toks[1].trim();
                    if (key.equalsIgnoreCase("SAMPLE#")) {
                        info.sampleNumber = QInfo.NULL_SAMPLE_NUMBER;
                        try {
                            info.sampleNumber = Integer.parseInt(value);
                        } catch (NumberFormatException nfe) {}
                    } else if (key.equalsIgnoreCase("USER")) {
                        info.user = value;
                    } else if (key.equalsIgnoreCase("MACRO")) {
                        info.macro = value;
                    } else if (key.equalsIgnoreCase("SOLVENT")) {
                        info.solvent = value;
                    } else if (key.equalsIgnoreCase("TEXT")) {
                        info.text = value;
                    } else if (key.equalsIgnoreCase("SampleDir")) {
                        info.sampleDir = value;
                    } else if (key.equalsIgnoreCase("USERDIR")) {
                        info.userdir = value;
                    } else if (key.equalsIgnoreCase("DATA")) {
                        info.data = value;
                    } else if (key.equalsIgnoreCase("STATUS")) {
                        info.status = value;
                    }
                }
            }
        } catch (FileNotFoundException e) {
        } catch (IOException e) {
        } finally {
            try {
                in.close();
            } catch (Exception e) {}
        }
        return infos;
    }

    static class QInfo {
        public static final int NO_SAMPLE_NUMBER = -1;
        public static final int NULL_SAMPLE_NUMBER = 0;

        public int sampleNumber = NO_SAMPLE_NUMBER;
        public String user;
        public String macro;
        public String solvent;
        public String text;
        public String sampleDir;
        public String userdir;
        public String data;
        public String status;
    }
}
