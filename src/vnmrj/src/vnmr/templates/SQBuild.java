/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.templates;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import vnmr.util.FileUtil;
import vnmr.util.Messages;
import vnmr.util.VnmrParameter;

import static vnmr.templates.ProtocolBuilder.*;


public class SQBuild {

    // Standard files to check for node lists:
    private static final String LONGLIST = "LONGLIST";

    private static final String EXPLIST = "EXPLIST";

    private static final String EXPLIST_PARAM = "explist";

    private static final String CMDNODES = "CMDnodes";

    private static final String ACQLIST = "ACQlist";

    private static final String FIDLOG = "fidlog";


    private String activeLocdir = null;

    private SQNodeList nodeList = new SQNodeList();

    private String archsamp;

    private String samplename;

    private String autodir;

    private List<String> buildList;

    private String archivedirSample;

    private String fidlog;

    private String fidlist;

    private String explistpar;

    private String cmdNodesPath;

    private String acqlog;

    private String explist;

    private String longlist;

    private String sample;

    private int m_bsCompleted;

    private int m_fidsCompleted;



    public static void main(String[] args) {
        List<String> dirlist = new ArrayList<String>();
        SQNodeList nodes = null;
        for (int i = 0; i < 10; i++) {
            dirlist.clear();
            for (int j = 3; j < args.length; j++) {
                dirlist.add(args[j]);
            }
            long t0 = System.currentTimeMillis();
            SQBuild sqb = new SQBuild(args[0], args[1], args[2], dirlist, false);
            nodes = sqb.getNodeList();
            long t1 = System.currentTimeMillis();
            //printNodeList(nodes);
            System.out.println("time=" + (t1 - t0) + " ms");
        }
        printNodeList(nodes);
    }

    public SQBuild(String archsamp, String samplename, String autodir,
                   List<String> fileList, boolean locked) {
        this.archsamp = archsamp;
        this.samplename = samplename;
        this.autodir = autodir;
        this.buildList = fileList;
        definePaths(archsamp);

        // NB: nodeList is the master list of nodes we're building up

        if (fileList.contains(FIDLOG)) {
            fileList.remove(FIDLOG);
            // Add completed experiments (they have saved FIDs)
            SQNodeList sampNodes = getSampInfoNodes(archsamp);
            nodeList.addAll(sampNodes);
            //System.out.println(sampNodes.size() + " SampInfo nodes");/*DBG*/
        }

        // Get all nodes in ACQlist file
        SQNodeList acqListNodes = getAllListedNodes(acqlog);

        if (fileList.contains(ACQLIST)) {
            fileList.remove(ACQLIST);
            // Add the ERROR and ACQUIRING nodes in ACQlist to nodeList
            addErrAndAcqNodes(nodeList, acqListNodes);
        }

        SQNodeList cmdNodes = getCmdNodes(cmdNodesPath);
        if (fileList.contains(CMDNODES)) {
            fileList.remove(CMDNODES);
            // Add "command" protocols in appropriate locations
            mergeCmdNodes(nodeList, cmdNodes, acqListNodes);
        }

        // Get all nodes mentioned in the explist file
        SQNodeList explistNodes = getAllListedNodes(explist);

        if (fileList.contains(EXPLIST_PARAM)) {
            fileList.remove(EXPLIST_PARAM);
            // Get queued expts from explist parameter
            // Note that an experiment cannot be Active and also
            // in explist. If so, it is in the SetUp mode and should
            // be considered Executing or Error.
            addExplistNodes(nodeList, explistpar,
                            explistNodes, acqListNodes, cmdNodes);
        }

        if (fileList.contains(EXPLIST)) {
            fileList.remove(EXPLIST);
            // Get queued expts from EXPLIST file
            addQueuedNodes(nodeList, explistNodes, cmdNodes, "_day");
        }

        if (fileList.contains(LONGLIST)) {
            fileList.remove(LONGLIST);
            // Get queued expts from LONGLIST (night queue) file
            SQNodeList longListNodes = getAllListedNodes(longlist);
            addQueuedNodes(nodeList, longListNodes, cmdNodes, "_night");
        }

        // Get queued experiments from "extra" files
        for (String file : fileList) {
            // Probably a path to an EXPLIST or LONGLIST
            String path = file;
            if (!new File(file).isAbsolute()) {
                path = this.archsamp + "/" + path;
            }
            SQNodeList nodes = getAllListedNodes(path);
            String when = (file.contains("EXPLIST")) ? "_day" : "_night";
            addQueuedNodes(nodeList, nodes, cmdNodes, when);
        }

        // Look for active node
        Messages.postDebug("AllSQActive", "Look for active among "
                           + nodeList.size() + " nodes.");
        SQNode node = getActiveNode();
        if (node != null) {
            String exp = node.getCpName();
            String dataPath = isCurrentFid(archsamp, exp, autodir);
            Messages.postDebug("SQActive", node.getTitle() + " is active"
                               + ", path=" + dataPath);
            if (dataPath != null) {
                node.setData(dataPath);
            }
        }
    }

    public String getActiveLocdir() {
        return activeLocdir;
    }

    public SQNode getActiveNode() {
        // NB: more reliable to look through the list backwards
        for (int i = nodeList.size() - 1; i >= 0; --i) {
            SQNode node = nodeList.get(i);
            if (node.getStatus().equals(SQ_EXECUTING)) {
                return node;
            }
        }
        return null;
    }

    /**
     * Get the master node list.
     * @return The list of nodes.
     */
    public SQNodeList getNodeList() {
        return nodeList;
    }

    /**
     * Use the base directory of this SQBuild to determine the paths to
     * various files needed to build the SQ.
     * @param dirpath The base directory.
     */
    private void definePaths(String dirpath) {
        if (dirpath.contains(".macdir")) {
            // Collecting info from automation directory
            Map<String, VnmrParameter> pmap;
            String[] pars = {"sample", "archivedir", "samplename"};
            pmap = VnmrParameter.readParameters(dirpath +"/sampleglobal", pars);
            sample = pmap.get("sample").getStringValue();
            String archivedir = pmap.get("archivedir").getStringValue();
            samplename = pmap.get("samplename").getStringValue();
            archivedirSample = archivedir + "/" + sample;
            buildList.remove(FIDLOG);
            buildList.remove(CMDNODES);
            buildList.remove(ACQLIST);
            buildList.remove(EXPLIST);
            fidlog = archsamp + "/fidlog";
            fidlist = archsamp + "/fidlist";
            explistpar  = archsamp + "/explist";
            cmdNodesPath = archsamp + "/CMDnodes";
            acqlog = archsamp + "/ACQlist";
            explist = archsamp + "/EXPLIST";
            longlist = archsamp + "/LONGLIST";
        } else {
            // Collecting info from data directory
            if (buildList.contains(FIDLOG)) {
                buildList.add(CMDNODES);
            }
            File tmpfile = new File(dirpath);
            sample = tmpfile.getName();
            archivedirSample = dirpath;
            fidlog = archsamp + "/dirinfo/fidlog";
            fidlist = archsamp + "/dirinfo/fidlist";
            explistpar  = archsamp + "/dirinfo/macdir/explist";
            cmdNodesPath = archsamp + "/dirinfo/macdir/CMDnodes";
            acqlog = archsamp + "/dirinfo/macdir/ACQlist";
            explist = archsamp + "/dirinfo/macdir/EXPLIST";
            longlist = archsamp + "/dirinfo/macdir/LONGLIST";
        }
    }

    private SQNodeList getSampInfoNodes(String dirpath) {
        SQNodeList nodes = new SQNodeList();
        //
        BufferedReader in = null;
        try {
            in = new BufferedReader(new FileReader(fidlog));
            String line;
            while ((line = in.readLine()) != null) {
                SQNode node = getNodeFromFidlogLine(line);
                if (node != null) {
                    if (nodes.containsCpName(node.getCpName())) {
                        // Already have a node with that name
                        node.setCpName("");
                    }
                    if (node.getCpName().length() == 0) {
                        node.setCpName(node.getPsLabel());
                    }
                    nodes.add(node);
                }
            }
        } catch (IOException ioe) {
            // OK if the file is not there
            //Messages.postError("SQBuild.getSampInfoNodes: "
            //                   + "Error reading file: " + fidlog);
        }
        try { in.close(); } catch (Exception e) {}

        return nodes;
    }

    private SQNodeList getCmdNodes(String path) {
        SQNodeList nodes = new SQNodeList();
        //
        BufferedReader in = null;
        try {
            in = new BufferedReader(new FileReader(path));
            String line;
            while ((line = in.readLine()) != null) {
                SQNode node = getNodeFromListLine(line);
                if (node.getStatus().equals(SQ_COMPLETED)
                    && node.getPsLabel().startsWith("CMD_")) // TODO: CMD???
                {
                    nodes.add(node);
                }
            }
        } catch (IOException ioe) {
            //Don't expect file to necessarily be there
        }
        try { in.close(); } catch (Exception e) {}

        return nodes;
    }

    /**
     * Get all nodes that are listed in the given experiment list file.
     * @param explist Path to the list file.
     * @return The node list.
     */
    private SQNodeList getAllListedNodes(String explist) {
        SQNodeList nodes = new SQNodeList();
        BufferedReader in = null;
        try {
            in = new BufferedReader(new FileReader(explist));
            String line;
            while ((line = in.readLine()) != null) {
                SQNode node = getNodeFromListLine(line);
                nodes.add(node);
            }
        } catch (IOException ioe) {
            //Don't expect file to necessarily be there
        }
        try { in.close(); } catch (Exception e) {}
        return nodes;
    }

    /**
     * Add nodes that occur in the "explist" parameter to the master list.
     * For a node to be added to the master list it must:
     * <OL>
     * <LI>Be in the "explist" parameter in the "explistpar" file.
     * <LI>Not appear in the acqListNodes, i.e., not in ACQlist file.
     * <LI>Be in expNodes, i.e., the EXPLIST file.
     * </OL><BR>
     * Nodes are added in the order that they occur in the "explist" parameter.
     * If an expNode appears in the list of commandNodes,
     * it's type will be "CMD".
     * @param nodes The master list to be added onto.
     * @param explistpar A parameter file containing the "explist" parameter.
     * @param expNodes The nodes from the EXPLIST file.
     * @param acqListNodes The nodes that are in ACQlist.
     * @param commandNodes The "command" nodes.
     */
    private void addExplistNodes(SQNodeList nodes,
                                 String explistpar,
                                 SQNodeList expNodes,
                                 SQNodeList acqListNodes,
                                 SQNodeList commandNodes) {
        if (expNodes.size() > 0) {
            String[] parExps = VnmrParameter.getStringValues(explistpar,
                                                             EXPLIST_PARAM);
            for (String cpname : parExps) {
                int expIdx = expNodes.indexOfCpName(cpname);
                if (expIdx >= 0) {
                    // This cpname is in the expNodes list
                    boolean isInACQlist = acqListNodes.containsCpName(cpname);
                    if (cpname.length() == 0 || isInACQlist) {
                        expNodes.remove(expIdx);
                    } else {
                        // This cpname is not in ACQlist
                        SQNode expNode = expNodes.get(expIdx);
                        expNode.setStatus(SQ_QUEUED);
                        expNode.setType(isCmd(expNode, commandNodes)
                                        ? "CMD" : "LIB");
                        expNode.setWhen("");
                        expNode.setData("null");
                        expNode.setTitle(cpname);
                        expNode.setLocked(true);
                        nodes.add(expNode);
                    }
                }
            }
        }
    }

    /**
     * Merge a list of commandNodes into the master list of nodes
     * at the appropriate places.
     * The ACQlist nodes are used to determine where to insert the
     * command node in the master list.
     * @param nodes The master list of nodes.
     * @param commandNodes The commandNodes to merge in.
     * @param acqListNodes The nodes currently in ACQlist.
     */
    private void mergeCmdNodes(SQNodeList nodes,
                               SQNodeList commandNodes,
                               SQNodeList acqListNodes) {
        for (int cmdIdx = commandNodes.size() - 1; cmdIdx >= 0; --cmdIdx) {
            SQNode cnode = commandNodes.get(cmdIdx);
            int acqIdx = acqListNodes.indexOfCpName(cnode.getCpName());
            if (acqIdx >= 0) {
                // Found an ACQlist node corresponding to this command node
                SQNode acqNode = acqListNodes.get(acqIdx);
                String pslabel = acqNode.getPsLabel();
                // The pslabel in ACQlist should start with "CMD_"
                if (pslabel.startsWith("CMD_")) {
                    pslabel = pslabel.substring(4);
                    int nodeIdx = nodes.indexOfCpName(pslabel);
                    // nodeIdx = where to insert the command node
                    if (nodeIdx >= 0) {
                        SQNode node = nodes.get(nodeIdx);
                        String cname = cnode.getCpName();
                        int usIdx = cname.lastIndexOf("_");
                        if (usIdx > 0) {
                            pslabel = cname.substring(0, usIdx);
                        } else {
                            pslabel = cname;
                        }
                        cnode.setPsLabel(pslabel);
                        cnode.setAcqTime(0);
                        cnode.setStatus(SQ_COMPLETED);
                        cnode.setData("");
                        cnode.setType("CMD");
                        cnode.setWhen(node.getWhen());
                        cnode.setTitle(node.getCpName());
                        cnode.setLocked(true);
                        nodes.add(nodeIdx, cnode);
                    }
                }
            }
        }
    }

    private static void addErrAndAcqNodes(SQNodeList nodeList,
                                          SQNodeList addList) {
        // Sort additions like ChemPack:
        //  1) ERROR nodes
        //  2) ACTIVE/ACQUIRING nodes that we turn into ERROR nodes
        //  3) The last ACTIVE node (turned into ACQUIRING status)
        // TODO: OR insert nodes into nodeList in original positions?

        // Put ERROR nodes first
        for (SQNode node : addList) {
            if (node.getStatus().equals(SQ_ERROR)) {
                nodeList.add(node);
            }
        }

        // Put EXECUTING nodes last
        int nadds = addList.size();
        for (int i = 0; i < nadds; i++) {
            SQNode node = addList.get(i);
            String status = node.getStatus();
            if (status.equals(SQ_EXECUTING) || status.equals(SQ_ACTIVE)) {
                if (i == nadds - 1) {
                    // NB: Only the last executing node gets EXECUTING status
                    node.setStatus(SQ_EXECUTING);
                } else {
                    node.setStatus(SQ_ERROR);
                    node.setData("");
                }
                nodeList.add(node);
            }
        }
    }

    private static void addQueuedNodes(SQNodeList nodeList,
                                       SQNodeList addList,
                                       SQNodeList cmdList, String when) {
        for (SQNode node : addList) {
            String cpname = node.getCpName();
            if (!nodeList.containsCpName(node.getCpName())) {
                node.setStatus(SQ_QUEUED);
                node.setType(isCmd(node, cmdList) ? "CMD" : "LIB");
                node.setWhen(when);
                node.setData("null");
                node.setTitle(cpname);
                node.setLocked(true);
                nodeList.add(node);
            }
        }
    }

    private static boolean isCmd(SQNode node, SQNodeList cmdList) {
        return cmdList.containsCpName(node.getCpName());
    }

//    /**
//     * Get the location of a node with the given cpName in the given node list.
//     * @param nodes The node list.
//     * @param cpName The cpName to look for.
//     * @return The index of the node, or -1 on failure.
//     */
//    private static int findCpName(SQNodeList nodes, String cpName) {
//        for (int i = 0; i < nodes.size(); i++) {
//            if (nodes.get(i).getCpName().equals(cpName)) {
//                return i;
//            }
//        }
//        return -1;
//    }

    /**
     * Create a node from a line of a "fidlog" file.
     * @param line The line to parse.
     * @return The new node.
     */
    private SQNode getNodeFromFidlogLine(String line) {
        SQNode node = null;
        String[] toks = line.split(" +", 2);
        if (toks.length == 2) {
            String title = toks[1];
            String fullpath = archsamp + "/" + toks[1];
            String fidpath = FileUtil.getFidPath(fullpath);
            if (fidpath != null) {
                // The data has been saved
                String pfid = fidpath + "/procpar";
                String data = fullpath;
                // Strip possible "$vnmruser/data" from path
                String defaultDataDir = FileUtil.usrdir() + "/data/";
                if (data.startsWith(defaultDataDir)) {
                    data = data.substring(defaultDataDir.length());
                }
                Map<String, VnmrParameter> pmap;
                String[] pars = {"explist", "pslabel", "ACQtime"};
                pmap = VnmrParameter.readParameters(pfid, pars);
                String cpname = pmap.get("explist").getStringValue();
                String pslabel = pmap.get("pslabel").getStringValue();
                double acqtime = pmap.get("ACQtime").getDoubleValue();
                String status = SQ_COMPLETED;
                String type = "LIB";
                String when = "";
                boolean locked = true;
                node = new SQNode(cpname, pslabel, acqtime,
                                      type, status, locked,
                                      when, data, title);
            }
        }
        return node;
    }

    /**
     * Parse a line from a "list" file (ACQlist, etc.) to construct a node.
     * <BR>
     * Format of line:<BR>
     * <CODE>PROTON_02  24  PROTON  xx000  Active</CODE>
     * @param line The line to parse.
     */
    private SQNode getNodeFromListLine(String line) {
        SQNode node = null;
        String[] toks = line.split(" +");
        if (toks.length > 2) {
            // Has at least cpName, time, psLabel
            String cpname = toks[0];
            String title = toks[0];
            double acqtime = 0;
            try {
                acqtime = Double.parseDouble(toks[1]);
            } catch (NumberFormatException nfe) {}
            String pslabel = toks[2];
            String status = (toks.length > 4) ? toks[4] : SQ_ERROR;
            String type = "LIB";
            String when = "";
            boolean locked = true;
            String data = "";
//            if (status.equals(EXECUTING) || status.equals(ACTIVE)) {
//                data = getCurrentFidPath(cpname, archsamp, autodir);
//            }
            node = new SQNode(cpname, pslabel, acqtime,
                                  type, status, locked,
                                  when, data, title);
        }
        return node;
    }

    private String isCurrentFid(String reqArchdir,
                                       String reqFirstExp,
                                       String autodir) {
        String rtn = null;
        String md = autodir + "/enterQ.macdir";

        File global = new File(md + "/currentsampleglobal");
        if (!global.exists()) {
            Messages.postDebug("SQActive", "No currentsampleglobal");
            return null;
        }

        if (!(new File(md + "/currentsampleinfo")).exists()) {
            Messages.postDebug("SQActive", "No currentsampleinfo");
            return null;
        }

        if (!(new File(md + "/currentQ")).exists()) {
            Messages.postDebug("SQActive", "No currentQ");
            return null;
        }

        if (!(new File(md + "/currentstudypar")).exists()) {
            Messages.postDebug("SQActive", "No currentstudypar");
            return null;
        }

        String archive = null;
        String sample = null;
        Map<String, VnmrParameter> pmap;
        String[] pars = {"archivedir", "sample"};
        pmap = VnmrParameter.readParameters(global.getPath(), pars);
        if (pmap != null) {
            archive = pmap.get("archivedir").getStringValue();
            sample = pmap.get("sample").getStringValue();
        }
        if (archive == null) {
            Messages.postDebug("SQActive", "No archive dir");
            return null;
        }
        if (sample == null) {
            Messages.postDebug("SQActive", "No sample name");
            return null;
        }

        if (!(archive + "/" + sample).equals(reqArchdir)) {
            Messages.postDebug("SQActive", "archsamp dir doesn't match: "
                               + "archive/sample=" + (archive + "/" + sample)
                               + ", reqArchdir=" + reqArchdir);
            return null;
        }

        String path = md + "/currentstudypar";
        String locdir  = VnmrParameter.readStringParameter(path, "locdir");
        if (locdir == null) {
            Messages.postDebug("SQActive", "No locdir");
            return null;
        }
        Messages.postDebug("SQActive", "locdir=" + locdir);
        activeLocdir = md + "/" + locdir;

        if (!(new File(md + "/" + locdir + "/current.fid")).exists()) {
            Messages.postDebug("SQActive", "No current.fid dir");
            return null;
        }

        if (reqFirstExp == null) { // Don't know if this is useful
            rtn = md + "/" + locdir;
        } else {
            String firstExp = getFirstExpFromCurrentQ(md + "/currentQ");
            Messages.postDebug("SQActive","firstExp=" + firstExp);
            if (!reqFirstExp.equals(firstExp)) {
                Messages.postDebug("SQActive", "exp doesn't match currentQ");
                return null;
            }

            path = md + "/" + locdir + "/current.fid/procpar";
            String explist = VnmrParameter.readStringParameter(path, "explist");
            Messages.postDebug("SQActive","explist=" + explist);
            if (explist == null || !reqFirstExp.equals(explist)) {
                Messages.postDebug("SQActive", "exp doesn't match explist");
                return null;
            }

            // See if the FID has data in it
            path = md + "/" + locdir + "/current.fid/fid";
            m_fidsCompleted = getNumberOfFidsInData(path);
            Messages.postDebug("SQActive","m_fidsCompleted=" + m_fidsCompleted);
            if (m_fidsCompleted == 0) {
                Messages.postDebug("SQActive", "No FIDs completed");
                return null;
            }

            rtn = md + "/" + locdir + "/current";
        }
        Messages.postDebug("SQActive","returning: " + rtn);
        return rtn;
    }

    /**
     * Get the number of transients saved so far.
     * @param path The path to the "fid" file.
     * @return The number of FID transients acquired and saved.
     */
    private int getNumberOfFidsInData(String path) {
        final int bytesPerBlockHdr = 28;
        int nfids = 0;
        DataInputStream in = null;
        try {
            in = new DataInputStream
            (new BufferedInputStream
             (new FileInputStream(path)));

            // File Header
            int nBlocks = in.readInt();
            in.skipBytes(4*4);
            //int ntraces = in.readInt();
            //int np = in.readInt();
            //int bytesPerElem = in.readInt();
            //int bytesPerTrace = in.readInt();
            int bytesPerBlock = in.readInt();
            in.skipBytes(2*2 + 4);
            //short version = in.readShort();
            //short status = in.readShort();
            //int nBlockHdrs = in.readInt();
            Messages.postDebug("AllSQActive", "File header: "
                               + "nblocks=" + nBlocks
                               + ", bytesPerBlock=" + bytesPerBlock);

            for (int i = 0; i < nBlocks; i++) {
                if (i > 0) {
                    // Skip the previous block - except for the header we read
                    int nskip = bytesPerBlock - bytesPerBlockHdr;
                    Messages.postDebug("AllSQActive", "Skip block: "
                                       + nskip + " bytes");
                    in.skipBytes(nskip);
                }
                // Block Header
                in.skipBytes(4*2);
                //short scale = in.readShort();
                //short blkStatus = in.readShort();
                //short blkIndex = in.readShort();
                //short blkMode = in.readShort();
                int ct = in.readInt();
                in.skipBytes(4*4); // Skip floats: lpval, rpval, lvl, tlt
                if (ct == 0) {
                    break;
                }
                nfids += ct;
                Messages.postDebug("AllSQActive", "Block " + (i + 1) + ": "
                                   + "ct=" + ct + ", nfids=" + nfids);
            }
        } catch (IOException ioe) {
        } finally {
            try {
                in.close();
            } catch (Exception e) {}
        }

        return nfids;
    }

//    /**
//     * Get the number of block-sizes done so far.
//     * @param path The path to the "log" file in the FID directory.
//     * @return The last block-size, or -1 if the experiment has completed.
//     */
//    private int getLastBsFromLog(String path) {
//        int bs = 0;
//        BufferedReader in = null;
//        String lastBsLine = null;
//        try {
//            in = new BufferedReader(new FileReader(path));
//            String line;
//            while ((line = in.readLine()) != null) {
//                if (line.matches(".* BS +[0-9]+ +completed")) {
//                    lastBsLine = line;
//                } else if (line.matches(".*Acquisition complete.*")) {
//                    // Catch case where bs is not set and experiment completes
//                    bs = -1;
//                }
//            }
//        } catch (FileNotFoundException e) {
//        } catch (IOException e) {
//        } finally {
//            try { in.close(); } catch (Exception e) {}
//        }
//        if (lastBsLine != null && bs == 0) {
//            String[] toks = lastBsLine.split(" +");
//            bs = Integer.parseInt(toks[toks.length - 2]);
//        }
//        Messages.postDebug("SQActive", "last bs=" + bs);
//        return bs;
//    }

    private static String getFirstExpFromCurrentQ(String path) {
        String rtn = null;
        BufferedReader in = null;
        try {
            in = new BufferedReader(new FileReader(path));
            String line;
            while ((line = in.readLine()) != null) {
                String[] tokens = line.split(" +");
                if (tokens[0].length() > 0
                    && !tokens[0].equals("SAMPLE_CHANGE_TIME"))
                {
                    rtn = tokens[0];
                    break;
                }
            }
        } catch (FileNotFoundException e) {
        } catch (IOException e) {
        } finally {
            try { in.close(); } catch (Exception e) {}
        }
        return rtn;
    }

//    private String getCurrentFidPath(String cpname, String archsamp2,
//                                     String autodir2) {
//        // TODO: Grab code from chempack "iscurrentfid"?
//        return "";
//    }

    public static void printNodeList(SQNodeList nodeList) {
        for (SQNode node : nodeList) {
            System.out.println(node.toString());
        }
    }
}
