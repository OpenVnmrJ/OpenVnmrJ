var cryomon_source;
var cryomon_status;
var cryogenErrorStatus = 'NoCryoError';
var gettingCryogenStatusLevels = false;
var system_status;
var experiment_stat;
var error_msg;
var hostname_source;
var experimentTime;
var systemStatus = 'Initial';
var errorStatus;
var userid;
var expid;
var heartbeat_source;
var heartbeatTimerId;
var getStatusTimerId;
var reloadTimerId;
var heartbeatTick;
var heartbeatsSkipped = 0;
var usePolling = false;

window.onerror = function(msg, url, linenumber) 
{
    console.log("In window.onerror msg: " + msg + "  url: " + url + " line: " + linenumber);

    try
    {
        var errorMsg = 'Error message: '+ msg +'\nURL: '+ url +'\nLine Number: '+ linenumber;
        console.log(errorMsg);
        handleError(msg);
    }
    catch(e) 
    {
        console.log('A Server Error Has Occurred: '+ e.message);
        location.reload(true);  // reload page but not from cache
    }
    return true;
};

function handleError(msg)
{
    console.log("In handleError msg: " + msg);

    stopListening();
    setupErrorAlertArea('A Server Error Has Occurred: '+msg);  // display message in error area
    reloadTimerId = setInterval(reloadPage,10000);   // reload this page after 10 seconds
}

function testError(errno, errmsg) 
{
    var parser = location;
    request = new XMLHttpRequest();

    var query= 'error+'+errno+'+'+errmsg;
    request.open("POST","http://"+parser.host+"/query?"+ query, false);
    request.send();
    
    console.log("testError('"+key+"') returned " + request.responseText);
}

function raiseError(errno, errmsg) 
{
    var parser = location;
    request = new XMLHttpRequest();

    var query= 'raise+'+errno+'+'+errmsg;
    request.open("POST","http://"+parser.host+"/query?"+ query, false);
    request.send();

    console.log("raiseError('"+key+"') returned " + request.responseText);
}

// called from a timer - which is immediately cleared
function reloadPage()
{
    clearInterval(reloadTimerId);
    location.reload(true);  // reload page but not from cache
}

function pollingQuery(key)
{
    var parser = location;
    request = new XMLHttpRequest();

    // set request.open 3rd parameter to true to make the call asynchronous
    try 
    {
        request.open("POST","http://"+parser.host+"/query?"+ key, false);
        request.send(key);
    } 
    catch(z) 
    {
        if (errorStatus != 'NoConnection')
        {
            ConnectionError();
        }
        return "";
    }

    if (errorStatus == 'NoConnection')  // connection has been reestablished
    {
        errorStatus = 'NoError';
        location.reload(true);  // reload page but not from cache
        return "";
    }
       
    return request.responseText;
}
    
function getStatus()
{
    clearInterval(getStatusTimerId);
    var cryoStatus = pollingQuery('cryomonstatus');
    var stat;
    try
    {
        stat = JSON.parse(cryoStatus);
    }
    catch(e) 
    {
        console.log('A Web Parse Error Has Occurred: '+e.message);
        handleError(e.message);
        return;
    }
    processCryoMonStatus(stat);

    var system_update_data = pollingQuery("systemState");
	
    if (errorStatus != 'NoConnection')
    {
        SetSystemStatus(system_update_data);
    }
}
    
function processMsg(msgStr)
{
    if (msgStr != "" && msgStr != 'clear')
    {
        setupErrorAlertArea(msgStr);
        errorStatus = 'Error';
    }
    else
    {
	if (errorStatus == 'Error')
        {
            errorStatus = 'NoError';
            console.log("In messageEvent - Error Cleared");
            if (systemStatus == 'Acquiring')
            {
                setupAcquireAlertArea();
                $('#experimentName').html(expid);
                $('#experimentOwner').html(userid);
            }
            else
            {
                setupStatusAlertArea(systemStatus);
            }
        }
    }
};

function processCryoMonStatus(cryomonStatus)
{
    // cryogen monitor status, i.e. is CryoMon server running, is the hardware pingable
    // if  Cryogen Monitor is pingable and CryoMon is running: - not an error
    //    {'cryoping': 1, 'cryoproc': 1} 
    // if Cryogen Monitor is not pingable and CryoMon is not running:
    //    {'cryoping': 0, 'cryoproc': 0} "Error - can't communicate with cryogen monitor hardware"
    // if Cryogen Monitor is pingable and CryoMon is not running, but getting levels  - not an error 
    //    {'cryoping': 1, 'cryoproc': 0, 'cryolevel': '14:05:04:17:00,083,082,0'}
    // if Cryogen Monitor is pingable and CryoMon is not running, but not responsive (no levels)
    //    {'cryoping': 1, 'cryoproc': 0} "Error - can't read cryogen levels"
    // 
	
    var cryogenErrorMsg = "";
    var cryogenMonitorStatus = 1;
    var cryogenServerStatus = 1;
    var cryogenLevel = "";

    if (cryomonStatus.hasOwnProperty('cryoping'))
    {
    	cryogenMonitorStatus = cryomonStatus['cryoping'];
    }

    if (cryomonStatus.hasOwnProperty('cryoproc'))
    { 
    	cryogenServerStatus = cryomonStatus['cryoproc'];
    }

    if (cryomonStatus.hasOwnProperty('cryolevel'))
    { 
        cryogenLevel = cryomonStatus['cryolevel'];
    }
    
    gettingCryogenStatusLevels = false;
    
    if (cryogenMonitorStatus == 0)
    	cryogenErrorMsg = 'Cryogen monitor network error';
    else if ((cryogenServerStatus == 0) && (cryogenLevel == ""))
    	cryogenErrorMsg = 'Reading cryogen monitor...'
    else if ((cryogenServerStatus == 0) && (cryogenLevel != ""))
    {
    	if (cryogenErrorStatus == 'CryoError')
        {
        	setupCryomon();
        	cryogenErrorStatus = "NoCryoError";
        }

	gettingCryogenStatusLevels = true;
    	processCryoMonMessage(cryogenLevel);
    }
    
    if (cryogenErrorMsg != "")
    {
    	setupCryomonError(cryogenErrorMsg);
    	cryogenErrorStatus = "CryoError";
    }
    else if (cryogenErrorStatus == 'CryoError')
    {
    	setupCryomon();
    	cryogenErrorStatus = "NoCryoError";
    }
}

function processCryoMonMessage(str)
{   
    var HeLevelStr;
    var N2LevelStr;
    if (str == "")
    {
        HeLevelStr = 0;
        N2LevelStr = 0;
    }
    else
    {
        var n=str.split(",");
        
        HeLevelStr = n[1];
        N2LevelStr = n[2];
    }
    
    var HeLevel = parseFloat(HeLevelStr)/100.0; 
    var N2Level = parseFloat(N2LevelStr)/100.0; 
    updateChart(0, N2Level);
    updateChart(1, HeLevel);
}

function SetSystemStatus(data)
{
    var stat;
    try
    {
        stat = JSON.parse(data);
    }
    catch(e) 
    {
        console.log('A Web Parse Error Has Occurred: '+e.message);
        handleError(e.message);
        return false;
    }
        
    if (stat.hasOwnProperty('acqmsg')) 
    {
        var msg = stat['acqmsg'];
        processMsg(msg);
    }
        
    if (stat.hasOwnProperty('AcqLockLevel')) 
    {
        var locklev = stat['AcqLockLevel'];
        $('#lockLevel').html(locklev);
    }
        
    if (stat.hasOwnProperty('AcqVTAct')) 
    {
        var sampletmp = parseFloat(stat['AcqVTAct']);
        sampletmp = (sampletmp / 10.0).toFixed(1);
        $('#sampleTemp').html(sampletmp);
    } 
    else
    {
        //console.log("AcqVTAct : no sample temperature\n");
    }

    /*
    if (stat.hasOwnProperty('cryomon')) 
    {
	if ((cryogenErrorStatus != "cryomonError") && (gettingCryogenStatusLevels == false))
	{
	    str = stat['cryomon'];
	    processCryoMonMessage(str);
	}
    }
    */
    if (stat.hasOwnProperty('AcqChannelBitsConfig1')) 
    {
        var channelexist = stat["AcqChannelBitsConfig1"];
        setChannelExist(channelexist);
    }
        
    if (stat.hasOwnProperty('AcqChannelBitsActive1')) 
    {
        var channelactive = stat["AcqChannelBitsActive1"];
        updateChannelActivePolling(channelactive);
    }
        
    if (stat.hasOwnProperty('ProcUserID')) 
    {
        userid = stat["ProcUserID"];
        $('#experimentOwner').html(userid);
    }
        
    if (stat.hasOwnProperty('ProcExpID')) 
    {
        expid = stat["ProcExpID"];
        $('#experimentName').html(expid);
    }
        
    if (stat.hasOwnProperty('AcqState')) 
    {
        var currentStatus = systemStatus;
        systemStatus = stat["AcqState"];

        if ((errorStatus != 'Error') && (currentStatus != systemStatus))  // if not error, and status has changed
        {
            if (systemStatus != "Acquiring")
            {
                setupStatusAlertArea(systemStatus);
            }
            else
            {
                setupAcquireAlertArea();
                $('#experimentName').html(expid);
                $('#experimentOwner').html(userid);
            }
        }
    }
        
    if (stat.hasOwnProperty('ExpTime')) 
    {
        experimentTime = stat["ExpTime"];
    }
        
    if (stat.hasOwnProperty('RemainingTime')) 
    {
        var remainingTime = stat["RemainingTime"];
        $('#acquireTimeLeft').html(ConvertSeconds(remainingTime));
        
        if (experimentTime > 0)
        {
            var acquirepct = ((experimentTime - remainingTime) / experimentTime ) * 100;
            $('#acquireBar').width(acquirepct.toString()+"%");
        }
    }

    getStatusTimerId = setInterval(getStatus,500);
};


function updateChannelActivePolling(channelactive)
{
    var channel1bit = 2;
    var channel2bit = 4;
    var channel3bit = 8;
    var channelPFGbit = 512;

    if ((activeChannel1 >= 0) && ((channelactive & channel1bit) > 0))
        {
            activeChannel1 = 1;
        }
    else if (activeChannel1 == 1)
        {
            activeChannel1 = 0;
        }
                
    if ((activeChannel2 >= 0) && ((channelactive & channel2bit) > 0))
        {
            activeChannel2 = 1;
        }
    else if (activeChannel2 == 1)
        {
            activeChannel2 = 0;
        }
                
    if ((activeChannel3 >= 0) && ((channelactive & channel3bit) > 0))
        {
            activeChannel3 = 1;
        }
    else if (activeChannel3 == 1)
        {
            activeChannel3 = 0;
        }
                
    if ((activeChannelPFG >= 0) && ((channelactive & channelPFGbit) > 0))
        {
            activeChannelPFG = 1;
        }
    else if (activeChannelPFG == 1)
        {
            activeChannelPFG = 0;
        }
}


function ConnectionError()
{
	var str = "Lost Connection to Host - Monitor Disabled";
	console.log("Connection Error: " + str);
	
	setupErrorAlertArea(str);
	errorStatus = 'NoConnection';

	// Set cryo chart levels, temperature, and lock level to zero
	updateChart(0, 0);
	updateChart(1, 0);

	$('#lockLevel').html(0);
	$('#sampleTemp').html(0);
}

function checkHeartbeat()
{
    heartbeatTick = heartbeatTick + 1;
    if (heartbeatTick > 25) 
    {
        ConnectionError();
	heartbeatsSkipped++;
	if (heartbeatsSkipped > 10)  // try again after 10 secs
        {
	    heartbeatsSkipped = 0;
	    location.reload(false);  // reload page from cache
	}
    }
    else
    {
	HeartbeatsSkipped = 0;
    }
}

function heartbeatEvent(event)
{
    heartbeatTick = 0;
    if (errorStatus == 'NoConnection')
    {
        errorStatus = 'NoError';
        location.reload(true);  // reload page but not from cache
    }
}

function SetupServerEvents(serverName)
{
    heartbeatTick = 0;
        
    if (serverName == "notConnected")
    {
        alert("Not Connected to ProPulse Web Server");
        return false;
    }
        
    serverURL = "http://" + serverName + "/monitor/poll/";
        
    errorStatus = 'NoError';
    experimentTime = 0;

    // check if this is Firefox - use HTML4 polling instead of server sent events
    if (navigator.userAgent.indexOf("Firefox") > 0)
    {
        //alert('Found Firefox');
        console.log('Found Firefox');
        usePolling = true;
    }
    else 
    {
        if(typeof(EventSource) != "undefined")  // check if server sent events are supported
        {
            usePolling = false;
        }
        else
        {
            usePolling = true;
        }
    }

    if (usePolling == true)
    {
        //alert('Using Polling');
        console.log('Using Polling');

        try
        {
            var system_initial_data = getCurrentState("systemState");
            SetSystemStatus(system_initial_data);

            getStatusTimerId = setInterval(getStatus,500);
        }
        catch(e)
        {
            stopListening();
            alert('A Server Error Has Occurred: '+e.message);
            return false;
        }
    }
    else    
    {
        console.log('Using Server Sent Events');

        try
        {
            systemStatus = getCurrentState('AcqState');

            console.log("In SetupServerEvents");
            system_status = new EventSource(serverURL + "expstat");
            system_status.onmessage = systemStatusEvent;

            hostname_source = new EventSource(serverURL + "hostname");
            hostname_source.onmessage = hostnameEvent;
        
            //cryomon_source = new EventSource(serverURL + "cryomon");
            //cryomon_source.onmessage = receiveCryoMonMessageEvent;
            
    	    cryomon_status = new EventSource(serverURL + "cryomonstatus");
    	    cryomon_status.onmessage = receiveCryoMonStatusEvent;
                        
            heartbeat_source = new EventSource(serverURL + "heartbeat");
            heartbeat_source.onmessage = heartbeatEvent;

            experiment_stat = new EventSource(serverURL + "expstat!AcqState!AcqLockLevel!AcqVTAct!AcqChannelBitsConfig1!AcqChannelBitsActive1!ProcUserID!ProcExpID!ExpTime!RemainingTime");
            experiment_stat.onmessage = experimentStatusEvent;
                
            error_msg = new EventSource(serverURL + "acqmsg");
            error_msg.onmessage = messageEvent;

            heartbeatTimerId = setInterval(checkHeartbeat,1000);  //1000 milliseconds
        }
        catch(e)
        {
            stopListening();
            alert('An Web Event Register Error Has Occurred: '+e.message);
            return false;
        }
    }
    return true;
}

function initCryoData()
{
    var cryoStatus = getCurrentState('cryomon');
    var cryoStr  = JSON.parse(cryoStatus);  // {'cryomon': "14:05:19:17:00,040,090,0"};
    processCryoMonMessage(cryoStr['cryomon']);
}

function stopListening() 
{
    console.log("In stopListening");
    if (usePolling == true)
    {
        clearInterval(getStatusTimerId);
    }
    else
    {
        if (typeof system_status != 'undefined')
        { 
            system_status.close();
            experiment_stat.close();
            cryomon_source.close();
            hostname_source.close();
            error_msg.close();
            heartbeat_source.close();
            
            clearInterval(heartbeatTimerId);
        }
    }
}

function receiveCryoMonStatusEvent(event) 
{
    var stat;
    try
    {
	stat = JSON.parse(event.data); // {"cryoping": 1, "cryoproc": 0, "cryolevel": "14:05:19:17:00,010,020,0"};
    }
    catch(e) 
    {
        console.log('A Web Parse Error Has Occurred: '+e.message);
        handleError(e.message);
        return false;
    }
    processCryoMonStatus(stat);
}

function lockLevel(event)
{
    console.log("In lockLevel: " + event.data);
    var locklev = parseFloat(event.data, 10);
    $('#lockLevel').html(locklev);
}

function sampleTemp(event)
{
    console.log("In sampleTemp: " + event.data);
    var sampletmp = parseFloat(event.data);
    sampletmp = (sampletmp / 10.0).toFixed(1);
    $('#sampleTemp').html(sampletmp);
}

function setChannelExist(channelexist)
{
    var channel1bit = 2;
    var channel2bit = 4;
    var channel3bit = 8;
    var channelPFGbit = 512;

    if ((channelexist & channel1bit) > 0) activeChannel1 = 0;
    else activeChannel1 = -1;
        
    if ((channelexist & channel2bit) > 0) activeChannel2 = 0;
    else activeChannel2 = -1;
        
    if ((channelexist & channel3bit) > 0) activeChannel3 = 0;
    else activeChannel3 = -1;
        
    if ((channelexist & channelPFGbit) > 0) activeChannelPFG = 0;
    else activeChannelPFG = -1;
}

function setChannelActive(channelactive)
{
    var channel1bit = 2;
    var channel2bit = 4;
    var channel3bit = 8;
    var channelPFGbit = 512;

    if ((activeChannel1 >= 0) && ((channelactive & channel1bit) > 0))
        {
            activeChannel1 = 1;
        }
    else if (activeChannel1 == 1)
        {
            activeChannel1 = 0;
        }
                
    if ((activeChannel2 >= 0) && ((channelactive & channel2bit) > 0))
        {
            activeChannel2 = 1;
        }
    else if (activeChannel2 == 1)
        {
            activeChannel2 = 0;
        }
                
    if ((activeChannel3 >= 0) && ((channelactive & channel3bit) > 0))
        {
            activeChannel3 = 1;
        }
    else if (activeChannel3 == 1)
        {
            activeChannel3 = 0;
        }
                
    if ((activeChannelPFG >= 0) && ((channelactive & channelPFGbit) > 0))
        {
            activeChannelPFG = 1;
        }
    else if (activeChannelPFG == 1)
        {
            activeChannelPFG = 0;
        }
                

    // update Acquire status area
    if ((errorStatus != 'Error') && (systemStatus == 'Acquiring'))  
        {
            setupAcquireAlertArea();
            $('#experimentName').html(expid);
            $('#experimentOwner').html(userid);
        }
}

function channelExist(event)
{
    var channelexist = event.data;
    setChannelExist(channelexist);
}

function channelActive(event)
{
    var channelactive = event.data;
    setChannelActive(channelactive);
}

function experimentOwner(event)
{
    var expOwn = event.data;
    $('#experimentOwner').html(expOwn);
}

function experimentName(event)
{
    var expName = event.data;
    $('#experimentName').html(expName);
}

function remainingTime(event)
{
    var remainTime = event.data;
    $('#acquireTimeLeft').html(ConvertSeconds(remainTime));

    if (experimentTime > 0)
        {
            var acquirepct = (remainTime / experimentTime) * 100;
            $('#acquireBar').width(acquirepct.toString()+"%");
        }
}

function systemStatusEvent(event)
{
    var stat;
    try
    {
        stat = JSON.parse(event.data);
    }
    catch(e) 
    {
        console.log('A Web Parse Error Has Occurred: '+e.message);
        handleError(e.message);
        return false;
    }
        
    if (stat.hasOwnProperty('AcqLockLevel')) 
    {
        var locklev = stat['AcqLockLevel'];
        $('#lockLevel').html(locklev);
    }
    
    if (stat.hasOwnProperty('AcqVTAct')) 
    {
        var sampletmp = parseFloat(stat['AcqVTAct']);
        sampletmp = (sampletmp / 10.0).toFixed(1);
        $('#sampleTemp').html(sampletmp);
    } 

    if (stat.hasOwnProperty('AcqChannelBitsConfig1')) 
    {
        var channelexist = stat["AcqChannelBitsConfig1"];
        setChannelExist(channelexist);
    }
        
    if (stat.hasOwnProperty('AcqChannelBitsActive1')) 
    {
        var channelactive = stat["AcqChannelBitsActive1"];
        setChannelActive(channelactive);
    }
        
    if (stat.hasOwnProperty('ProcUserID')) 
    {
        userid = stat["ProcUserID"];
        $('#experimentOwner').html(userid);
    }
        
    if (stat.hasOwnProperty('ProcExpID')) 
    {
        expid = stat["ProcExpID"];
        $('#experimentName').html(expid);
    }
        
    if (stat.hasOwnProperty('AcqState')) 
    {
        systemStatus = stat["AcqState"];
        if (errorStatus != 'Error')
        {
            if (systemStatus != "Acquiring")
            {
                setupStatusAlertArea(systemStatus);
            }
            else
            {
                setupAcquireAlertArea();
                $('#experimentName').html(expid);
                $('#experimentOwner').html(userid);
            }
        }
    }
        
    if (stat.hasOwnProperty('ExpTime')) 
    {
        experimentTime = stat["ExpTime"];
    }
        
    if (stat.hasOwnProperty('RemainingTime')) 
    {
        var remainingTime = stat["RemainingTime"];
        $('#acquireTimeLeft').html(ConvertSeconds(remainingTime));
    
        if (experimentTime > 0)
        {
            var acquirepct = ((experimentTime - remainingTime) / experimentTime ) * 100;
            $('#acquireBar').width(acquirepct.toString()+"%");
        }
    }

};


function experimentStatusEvent(event)
{
    var stat;
    try
    {
        stat = JSON.parse(event.data);
    }
    catch(e) 
    {
        console.log('A Web Parse Error Has Occurred: '+e.message);
        handleError(e.message);
        return false;
    }

    if (stat.hasOwnProperty('AcqLockLevel')) 
    {
        var locklev = stat['AcqLockLevel'];
        $('#lockLevel').html(locklev);
    }
        
    if (stat.hasOwnProperty('AcqVTAct')) 
    {
        var sampletmp = parseFloat(stat['AcqVTAct']);
        sampletmp = (sampletmp / 10.0).toFixed(1);
        $('#sampleTemp').html(sampletmp);
    }
        
    if (stat.hasOwnProperty('AcqChannelBitsConfig1')) 
    {
        var channelexist = stat["AcqChannelBitsConfig1"];
        setChannelExist(channelexist);
    }
        
    if (stat.hasOwnProperty('AcqChannelBitsActive1')) 
    {
        var channelactive = stat["AcqChannelBitsActive1"];
        setChannelActive(channelactive);
    }
        
    if (stat.hasOwnProperty('ProcUserID')) 
    {
        userid = stat["ProcUserID"];
        $('#experimentOwner').html(userid);
    }
        
    if (stat.hasOwnProperty('ProcExpID')) 
    {
        expid = stat["ProcExpID"];
        $('#experimentName').html(expid);
    }
        
    if (stat.hasOwnProperty('AcqState')) 
    {
        systemStatus = stat["AcqState"];
        if (errorStatus != 'Error')
        {
            if (systemStatus != "Acquiring")
            {
                setupStatusAlertArea(systemStatus);
            }
            else
            {
                setupAcquireAlertArea();
                $('#experimentName').html(expid);
                $('#experimentOwner').html(userid);
            }
        }
    }
        
    if (stat.hasOwnProperty('ExpTime')) 
    {
        experimentTime = stat["ExpTime"];
    }
        
    if (stat.hasOwnProperty('RemainingTime')) 
    {
        var remainingTime = stat["RemainingTime"];
        $('#acquireTimeLeft').html(ConvertSeconds(remainingTime));
    
        if (experimentTime > 0)
        {
            var acquirepct = ((experimentTime - remainingTime) / experimentTime ) * 100;
            $('#acquireBar').width(acquirepct.toString()+"%");
        }
    }

};

function hostnameEvent(event)
{
    var stat;
    try
    {
        stat = JSON.parse(event.data);
    }
    catch(e) 
    {
        console.log('A Web Parse Error Has Occurred: '+e.message);
        handleError(e.message);
        return false;
    }

    var str = "";
    if (stat.hasOwnProperty('hostname')) 
    {
        str = stat['hostname'];
    }

    $('#specName').html(str);
}

function getCurrentState(key) {
    var parser = location;
    
    //set request.open 3rd parameter to true to make the call asynchronous
    request = new XMLHttpRequest()
    request.open("POST","http://"+parser.host+"/query?"+ key, false);
    request.send(key);
    return request.responseText;

    // Do this to get the data Asynchronously
    //request.onreadystatechange = function() {
    //  if (request.readyState == 4) {
    //    console.log("getCurrentState('"+key+"') returned " + request.responseText);
    //    someGlobalVariable[key] = request.responseText;
    //  }
    //}
}

function messageEvent(event)
{
        
    var stat;
    try
    {
        stat = JSON.parse(event.data);
    }
    catch(e) 
    {
        console.log('A Web Parse Error Has Occurred: '+e.message);
        handleError(e.message);
        return false;
    }
        
    var str = "";
    if (stat.hasOwnProperty('acqmsg')) 
    {
        str = stat['acqmsg'];
    }
        
    if (str != "" && str != 'clear')
    {
        setupErrorAlertArea(str);
        errorStatus = 'Error';
    }
    else
    {
        errorStatus = 'NoError';
        console.log("In messageEvent - Error Cleared");
        if (systemStatus == 'Acquiring')
        {
            setupAcquireAlertArea();
            $('#experimentName').html(expid);
            $('#experimentOwner').html(userid);
        }
        else
        {
            setupStatusAlertArea(systemStatus);
        }
    }
};
