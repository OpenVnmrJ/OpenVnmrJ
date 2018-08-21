/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.util.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.bo.*;

public class StartSearch extends Thread {
    Hashtable statement;

    // Static so that this only causes a sleep the first time we are
    // ever called when starting vnmrj
    static boolean firstCall=true;
    static int overallCallNumber=0;

    public StartSearch(Hashtable statement) {
	this.statement = statement;
    }

    public void run() {
	String objType;
        String limitPath="";
        overallCallNumber++;
        int thisCallNumber = overallCallNumber;
        java.util.Date endtime;
        long timems;

        // When starting vnmrj, this search is started before a browser is
        // created, if indeed one is going to be created.  If we just keep
        // going here, the locator could have the wrong results.
        // Wait here a bit and let the browser get created.
        // 1 second is not enough, 2 works on Blade 1000 (750MHz).
        if(firstCall) {
            try {
                sleep(5000);
                // do not set firstCall here, use it for the cursor setting
                // and then set it below.
            }
            catch (Exception e) {};
        }

        objType = (String) statement.get("ObjectType");

        // Get the active LocatorHistory type
        SessionShare ssshare = ResultTable.getSshare();
        LocatorHistoryList lhl = ssshare.getLocatorHistoryList();
        String activeHistory = lhl.getLocatorHistoryName();

        // If BrowserLimited is set in the statement, use its true or false
        // value.  If not, for backwards compatibility, decide based on
        // objType whether or not to limit the search based on the browser.
        String browserLimited =  (String) statement.get("BrowserLimited");

        if(browserLimited == null) {
            // Check to see if the browser is open.  If so, we need to limit
            // the locator to only the open directory and below.  Not if type
            // is DB_AVAIL_SUB_TYPES or if LocatorHistory type is EDIT_PANEL_LH
            // Also do not limit for WORKSPACE, PROTOCOL, SHIMS
            if(isBrowserOpen() == true && 
               !objType.equals(Shuf.DB_AVAIL_SUB_TYPES) &&
               !objType.equals(Shuf.DB_WORKSPACE) &&
               !objType.equals(Shuf.DB_TRASH) &&
               !objType.equals(Shuf.DB_PROTOCOL) &&
               !objType.equals(Shuf.DB_SHIMS) &&
               !objType.equals(Shuf.DB_VNMR_PAR) &&
               !activeHistory.equals(LocatorHistoryList.EDIT_PANEL_LH)){
                // Get host_fullpath for all symbolic links below the root dir
                FillDBManager dbm = FillDBManager.fillDBManager;
                String root = LocatorBrowser.getRootPath();
                ArrayList symList;
                symList = dbm.symLinkMap.getHostFullpathForLinksBelow(root);

                // Add these onto limitPath as a space separated list of dirs
                for(int i=0; i < symList.size(); i++ ) {     
                    String path = (String)symList.get(i);
                    limitPath = limitPath.concat(path);
                    // no space after last one
                    if(i <  symList.size() -1)
                        limitPath = limitPath.concat(" ");
                }
            }
            else {
                limitPath = "all";
            }
        }
        else if(browserLimited.startsWith("t")) {
            // Check to see if the browser is open.  If so, we need to limit
            // the locator to only the open directory and below.  Not if type
            // is DB_AVAIL_SUB_TYPES or if LocatorHistory type is EDIT_PANEL_LH
            if(isBrowserOpen() == true && 
               !objType.equals(Shuf.DB_AVAIL_SUB_TYPES) &&
               !activeHistory.equals(LocatorHistoryList.EDIT_PANEL_LH)){
                // Get host_fullpath for all symbolic links below the root dir
                FillDBManager dbm = FillDBManager.fillDBManager;
                String root = LocatorBrowser.getRootPath();
                ArrayList symList;
                symList = dbm.symLinkMap.getHostFullpathForLinksBelow(root);

                // Add these onto limitPath as a space separated list of dirs
                for(int i=0; i < symList.size(); i++ ) {                    
                    String path = (String)symList.get(i);
                    limitPath = limitPath.concat(path);
                    // no space after last one
                    if(i <  symList.size() -1)
                        limitPath = limitPath.concat(" ");
                }
            }
            else {
                limitPath = "all";
            }
 
        }
        else {
            limitPath = "all";
        }

        if(DebugOutput.isSetFor("StartSearch"))
            Messages.postDebug("StartSearch limiting Path = " + limitPath);

        // Set the host_fullpath as determined for the search.
        statement.put("dirLimitValues", limitPath);
        ExpPanel exp=Util.getDefaultExp();

        // Catch all otherwise uncaught exceptions
        try {
            java.util.Date starttime=null;
            if(DebugOutput.isSetFor("StartSearch") || 
                       DebugOutput.isSetFor("locTiming"))
                starttime = new java.util.Date();

            // Do a reshuffle
            SessionShare sshare = ResultTable.getSshare();
            ShufflerService shufflerService = sshare.shufflerService();
            Hashtable searchCache = sshare.searchCache();
            StatementDefinition curStatement = sshare.getCurrentStatementType();
            ResultTable resultTable = Shuffler.getresultTable();
            ResultDataModel dataModel = resultTable.getdataModel();
            objType = (String) statement.get("ObjectType");
            if(DebugOutput.isSetFor("StartSearch"))
                 Messages.postDebug("StartSearch: objType = " + objType);

            SearchResults[] results;
            String match = "match";

            // Setting the cursor before the Result panels are created for
            // the first time is not so good, skip the first time at startup
            if(!firstCall && exp != null)
                exp.setBusy(true);

            // Do the locator search for Match
            results = shufflerService.searchDB(searchCache, objType, 
                                             statement, curStatement, match);
            // Put Match the results into the locator window unless we have
            // been called again on top of this call, then abort.
            if(thisCallNumber == overallCallNumber) {
                dataModel.setResults(results);
                resultTable.updateAllHeaders(statement);
            }

            if(exp != null)
                exp.setBusy(false);

            if(DebugOutput.isSetFor("StartSearch") || 
                       DebugOutput.isSetFor("locTiming")) {
                endtime = new java.util.Date();
                timems = endtime.getTime() - starttime.getTime();
                Messages.postDebug("StartSearch: Time(ms) for matching "
                                   + "search (" + results.length 
                                   + ") = " + timems);
            }


            // Do we need to do non matching also?  If so, we need to get the
            // result array and then create a new result array containing
            // both the matching and non matching items and call setResults
            // with the combined list.  The reason for calling setResults with
            // the matching list alone, is to get that part of the locator
            // displayed more quickly.
            // If thisCallNumber != overallCallNumber, it means that another
            // thread has been started running this method.  In that case,
            // we do not want to waste time doing the non matching.
            if(!MatchDialog.getshowMatchingOnly() && 
               thisCallNumber == overallCallNumber) {
                SearchResults[] resultsNon;
                SearchResults[] resultsAll;
                int totRows;
                if(DebugOutput.isSetFor("StartSearch") || 
                   DebugOutput.isSetFor("locTiming"))
                    starttime = new java.util.Date();

                if(!firstCall && exp != null)
                    exp.setBusy(true);

                match = "nonmatch";
                // Do the locator search for NonMatch
                resultsNon = shufflerService.searchDB(searchCache, objType, 
                                               statement, curStatement, match);

                // If there were no non matching items, it will have one
                // row which will contain the word Error for objType.
                // In that case, do not include this line.  If there were
                // an error we wanted to display, it would be in the
                // matching search.
                if(resultsNon.length == 1 && 
                               resultsNon[0].objType.equals("Error")) {
                    resultsNon = new SearchResults[0];
                    
                }
                // Transfer all of the matching and non matching results to
                // a new bigger array to hold them all
                totRows = results.length + resultsNon.length;
                resultsAll = new SearchResults[totRows];
                int i;
                for(i=0; i < results.length; i++)
                    resultsAll[i] = results[i];

                for(int k=0; i < totRows; i++, k++)
                    resultsAll[i] = resultsNon[k];

                // Put all the results into the locator window
                dataModel.setResults(resultsAll);
                resultTable.updateAllHeaders(statement);

                if(!firstCall && exp != null)
                    exp.setBusy(false);

                if(DebugOutput.isSetFor("StartSearch") || 
                   DebugOutput.isSetFor("locTiming")) {
                    endtime = new java.util.Date();
                    timems = endtime.getTime() - starttime.getTime();
                    Messages.postDebug("StartSearch: Time(ms) for non matching "
                                       + "search (" + resultsNon.length  
                                       + ") = " + timems);
                }
            }

            // Clear newlySavedFile because we only want to highlight it once.
            Shuf.newlySavedFile.clear();

            firstCall = false;



        }
        catch (Exception e) {
            Messages.postError("Problem Doing Locator Search");
            Messages.writeStackTrace(e);
            if(!firstCall && exp != null)
                exp.setBusy(false);
        }

    }


    static public boolean isBrowserOpen() {
        VToolPanel toolPanel;
        boolean result;

        // Go through all vToolPanels and see if any have a key by this
        // name in use.
        VTabbedToolPanel vToolPanel = Util.getVTabbedToolPanel();
        ArrayList vToolPanels = vToolPanel.getToolPanels();

        for(int i=0; i < vToolPanels.size(); i++) {
            toolPanel = (VToolPanel) vToolPanels.get(i);
            result = toolPanel.isPaneOpen("LocatorBrowser");
            if(result == true)
                return true;
        }
        return false;
    }


}
