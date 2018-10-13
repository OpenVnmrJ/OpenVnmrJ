/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package	 vnmr.ui;

import java.util.*;

import	vnmr.bo.*;
import	vnmr.ui.shuf.*;
import	vnmr.util.*;

/**
 * A session-share object is created per login.	 It is the vehicle for
 * sharing services, models, and data, and for passing messages among
 * different components.  Not every component needs to be passed the
 * session-share object -- only those that need to access the services
 * supported by SessionShare.
 *
 * <p>Having this session-share mechanism is not strictly necessary,
 * but is preferable to creating global/static dependencies.
 *
 * @author Mark Cao
 */
public class SessionShare {
    // ==== instance variables
    /** Application installer. Strictly speaking, this object doesn't really
     * belong here, since sessions are created after the user's application
     * is installed.  However, it is convenient and it doesn't hurt to
     * reference the installer from sessions. */
    private AppInstaller appInstaller;
    /** the logged in user */
    private User user;
    /** session storage service; left unexposed because it is used only
     * within notifyLogout() */
    private SessionStorage sessionStorage;
    /** User info, to be stored. Initialized to empty; it is up to other
     * objects to write to table, and read from table. This is saved
     * to disk upon logout. */
    private Hashtable<String, Object> userInfoTable = null;
    /** shuffler service */
    private LocatorHistoryList locatorHistoryList=null;
    /** data model for holding-area table */
    private HoldingDataModel holdingDataModel = null;
    /** parameters bin */
/*
    private ParamsBin paramsBin;
*/
    /** cut-and-paste service */
    private CutCopyPaste cutCopyPaste;
    /** printer service */
    private PrinterService printerService;
    /** cache of search results */
    private Hashtable searchCache = null;
    /** current Statement */
    private StatementDefinition currentStatementType;
    /** Spotter button */
    private SpotterButton spotterButton;

    /**
     * constructor
     * @param appInstaller application installer
     * @param user user
     */
    @SuppressWarnings("unchecked")
    public SessionShare(AppInstaller appInstaller, User user) {
        this.appInstaller = appInstaller;
        this.user = user;

        sessionStorage = SessionStorage.getDefaultStorage();
        if(user !=null) { // support for quick test classes
            userInfoTable = (Hashtable<String, Object>)sessionStorage.restore(user);

            String persona = System.getProperty("persona");
            boolean bAdm = (persona != null && persona.equalsIgnoreCase("adm")) ? true : false;

            // for admin interface, don't need to fill the locator.
            if (!bAdm) {
                FillLocatorHistoryList();
            }
        }
        if (userInfoTable == null)
            userInfoTable = new Hashtable<String, Object>();
        cutCopyPaste = new CutCopyPaste();
        printerService = PrinterService.getDefault();

    } // SessionShare()


    /** FillLocatorHistoryList
     */
    public void FillLocatorHistoryList() {
	         locatorHistoryList = new LocatorHistoryList();
    	 if (!FillDBManager.locatorOff()){
	         StatementHistory history = statementHistory();
	         if(history !=null)
	            history.updateWithoutNewHistory();
	         // if (searchCache == null)
	         //    searchCache = new Hashtable();
    	 }
         if(holdingDataModel == null)
            holdingDataModel = new HoldingDataModel();
         if (searchCache == null)
            searchCache = new Hashtable();
    }

    /**
     * get application installer
     */
    public AppInstaller appInstaller() {
         return appInstaller;
    }

    public void setAppInstaller(AppInstaller a) {
         appInstaller = a;
    }

    /**
     * get currently logged in user
     * @return user
     */
    public User user() {
        return user;
    } // user()

    @SuppressWarnings("unchecked")
    public void setUser(User u) {
        if (u == null)
            return;
        boolean bNewUser = true;
        if (user != null) {
            String oldName = user.getAccountName();
            String newName = u.getAccountName();
            if (oldName != null && (oldName.equals(newName)))
                 bNewUser = false;
        }
        user = u;
        if (bNewUser || userInfoTable == null) {
            userInfoTable = (Hashtable<String, Object>)sessionStorage.restore(user);
        }
        if (userInfoTable == null)
            userInfoTable = new Hashtable<String, Object>();
    }

    /**
     * get user info
     * @return user info
     */
    public Hashtable<String,Object> userInfo() {
        if (userInfoTable == null)
            userInfoTable = new Hashtable<String, Object>();
        return userInfoTable;
    } // userInfo()

    /**
     * logout notification
     */
    public void notifyLogout() {
        sessionStorage.store(user, userInfoTable);
    } // notifyLogout()

    /**
     * shuffler service
     */
    public ShufflerService shufflerService() {
    	if(locatorHistoryList==null)
    		return null;
    	LocatorHistory lh = locatorHistoryList.getLocatorHistory();
        if(lh == null) {
            Messages.postError("No LocatorHistory of this type available");
            return null;
        }
        ShufflerService shufServ = lh.getShufflerService();
        return shufServ;
    } // shufflerService()

    /**
     * get statement history
     * @return statement history
     */
    public StatementHistory statementHistory() {
    	if(FillDBManager.locatorOff())
    		return null;
    	LocatorHistory lh = locatorHistoryList.getLocatorHistory();
        if(lh == null) {
            Messages.postError("No LocatorHistory of this type available");
            return null;
        }

    StatementHistory stateHistory = lh.getStatementHistory();
    return stateHistory;
    } // statementHistory()

    public StatementHistory statementHistory(String objType) {
    StatementHistory stateHistory;
    LocatorHistory lh = locatorHistoryList.getLocatorHistory();
    stateHistory = lh.getStatementHistory(objType);
    return stateHistory;
    } // statementHistory()

    /**
     * data model for holding-area table
     */
    public HoldingDataModel holdingDataModel() {
        return holdingDataModel;
    } // holdingDataModel()

    public LocatorHistory getLocatorHistory() {
    LocatorHistory lh = locatorHistoryList.getLocatorHistory();
    return lh;
    }

    public LocatorHistoryList getLocatorHistoryList() {
       return locatorHistoryList;
    }


    /**
     * Set paramsBin. This method must be called early, prior to any
     * use of paramsBin.
     * @param bin params bin
     */
/*
    public void setParamsBin(ParamsBin paramsBin) {
    this.paramsBin = paramsBin;
    }
*/

/*
    public ParamsBin paramsBin() {
    return paramsBin;
    }
*/

    /**
     * cut-copy-paste service
     */
    public CutCopyPaste cutCopyPaste() {
       return cutCopyPaste;
    } // cutCopyPaste()

    /**
     * printer service
     */
    public PrinterService printerService() {
       return printerService;
    } // printerService()

    /**
     * search cache
     */
    public Hashtable searchCache() {
       return searchCache;
    } // searchCache()

    public StatementDefinition getCurrentStatementType() {
       return currentStatementType;
    }

    public void setCurrentStatementType(StatementDefinition newStatement) {
       currentStatementType = newStatement;
    }

    public void setSpotterButton(SpotterButton spotterButton) {
       this.spotterButton = spotterButton;
    }

    public SpotterButton getSpotterButton() {
       return spotterButton;
    }

    public synchronized void putProperty(String key, Object obj) {
        if (userInfoTable == null)
            return;
        userInfoTable.put(key, obj);
    }

    public synchronized Object getProperty(String key) {
        if (userInfoTable == null)
            return null;

        return userInfoTable.get(key);
    }

} // class SessionShare
