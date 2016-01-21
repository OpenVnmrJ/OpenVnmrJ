var messageState = 99;
var fileNum = 0;
var chartInitialized = false;
var channels = 2;
var tunesw = 10.0;
var tn = "H1";
var dn = "C13";
var dn2 = "";
var dn3 = "";
var dn4 = "";
var tuningUser = "vnmr1";
var serverURL = "";
var saved = {'tn': '', 'dn':'', 'dn2':'', 'dn3':'', 'dn4':''};
var state = {
    CONNECTED: 0, COLLECTING: 1, DISCONNECTED: 99
};

var getDataTimerId;
var stopFlag = false;
var loopMsg = "trtune:json";
var usingPolling = false;
var loopCount = 0;

var timedout = false;


window.onerror = function(msg, url, linenumber) 
{
	try
	{
	    var errorMsg = 'Error message: '+ msg +'\nURL: '+ url +'\nLine Number: '+ linenumber;
	    console.log(errorMsg);
	   handleError();
    }
	catch(e) 
	{
        console.log('A Server Error Has Occurred: '+ e.message);
        alert('A Server Error Has Occurred - Press OK to return to Remote Status Monitoring Page');
        
    	returnToRSU();
	}
	return true;
};

function handleError()
{
    stopRequest();

    if (timedout == false ) alert('A Server Error Has Occurred - Press OK to return to Remote Status Monitoring Page');
    
    returnToRSU();
}

function handleTimeout()
{
    console.log("Received timeout");
    timedout = true;
    returnToRSU();
}

function returnToRSU()
{
    var curPage  = document.location.href;
    var rsuPage = curPage.replace("tuning", "index"); 
    window.location.href = rsuPage;
}

function TuningConnectionError()
{
    var str = "Lost Connection to Host - Monitor Disabled";
    console.log("Connection Error: " + str);

    messageState = state.DISCONNECTED;
}

// called from tuning.html
function connectTuningServer(hostIP) 
{
    timedout = false;

    if ("WebSocket" in window) 
    {
        // the browser supports WebSockets
        //alert("WebSockets supported here!\r\n\r\nBrowser: " + navigator.userAgent);
        console.log("WebSockets supported here!\r\n\r\nBrowser: " + navigator.userAgent);
        
        if (hostIP == "notConnected") 
        {
            serverURL = prompt("Enter Server Address:", "ws://172.16.1.1:5678/");
        } 
        else 
        {
            serverURL = "ws://" + hostIP + ":5678/";
        }
        messageState = state.CONNECTED;
        usingPolling = false;
        setup(serverURL);
    } 
    else 
    {
        // the browser doesn't support WebSockets
        //alert("WebSockets NOT supported here!\r\n\r\nBrowser: " + navigator.userAgent);
        console.log("WebSockets NOT supported here!\r\n\r\nBrowser: " + navigator.userAgent);
        
        usingPolling = true;
        try 
        {
            // tell the server we're interested in tuning
            key = "trtune:json";
            request = new XMLHttpRequest();
            request.open("POST","http://"+location.host+"/query?"+ key, false);
            request.send(key);

    	    var msg = request.responseText;
    	    if (msg == 'timedout')
    		{
                handleTimeout();
    		    return;
    		}

            response = JSON.parse(request.responseText);
            messageState = state.COLLECTING;
            ProcessTuningData(response); 
        } 
        catch(e) 
        {
            console.log('A Server Error Has Occurred: '+e.message);
            handleError();
            return false;
        }
        messageState = state.CONNECTED;
    }
}

function stopTuningListening() 
{
    console.log("In stopTuningListening");

    clearInterval(getDataTimerId);
    stopFlag = true;
}

function pollingTuningQuery(key)
{
    var parser = location;

    request = new XMLHttpRequest();

    try 
	{
	    request.open("POST","http://"+parser.host+"/query?"+ key, false);
	    request.send(key);
	} 
    catch(z) 
	{
	    if (messageState != state.DISCONNECTED) 
		{
		    TuningConnectionError();
		}
	    return "";
	}

    if (messageState == state.DISCONNECTED) 
	{
	    errorStatus = 'NoError';
	    location.reload(true);  // reload page but not from cache
	    return "";
	}
       
    //console.log("pollingQuery('"+key+"') returned " + request.responseText);

    try
   	{
	    console.log("pollingQuery('"+key+"') returned " + request.responseText);
	    var  msg = request.responseText;
	    if (msg == 'timedout')
		{
            handleTimeout();
		    return;
		}
	}
    catch(e) 
    {
        handleError();
        return;
    }

    timedout = false;
    return request.responseText;
}

// called from a timer - which is immediately cleared
function longPollingLoop()
{
    clearInterval(getDataTimerId);
    if (stopFlag == true) return;
    
    stopFlag = false;
    try 
    {
        var tuning_data = pollingTuningQuery(loopMsg);
        if (messageState != state.DISCONNECTED) 
        {
            messageState = state.COLLECTING;
            ProcessTuningData(JSON.parse(tuning_data));
        }
        else 
        {
        	getDataTimerId = setInterval(longPollingLoop,4000);   // try again
        }
    } 
    catch(e) 
    {
        handleError();
        return false;
    }
}

// Process new data arriving at the client
function  ProcessTuningData(msg) // tuning_data) 
{
    if (messageState == state.COLLECTING) // query number of active channels 
    { 
        //msg = JSON.parse(tuning_data);
        var key = msg[0];
        var data = msg[1];
        tuningUser = data['trtune'];
        tmpchannels = +data['nf'];
        if ((chartInitialized == true) && (channels != tmpchannels)) 
        {
            location.reload();  // refresh page
        }
        channels = tmpchannels;
    
        console.log("Received trtuner="+tuningUser+" channels="+channels+" tunesw="+tunesw
                    +" tn="+data['tn']+" dn="+data['dn']+" dn2="+data['dn2']
                    +" dn3="+data['dn3']+" dn4="+data['dn4']);

        var tmptunesw = data['tunesw'] / 1e6;
        if ((chartInitialized == true) && (tunesw != tmptunesw)) 
        {
            location.reload();
        }
        tunesw = tmptunesw;
        
        var channel = ['tn', 'dn', 'dn2', 'dn3', 'dn4'];
        if (chartInitialized == true) 
        {
            for (var i = 0; i < channels; i++) 
            {
                if (data[channel[i]] != saved[channel[i]]) 
                    location.reload();
            }
        }
        
        for (var i = 0; i < channels; i++) 
        {
            var label = data[channel[i]];
            var pts = data['data'][channel[i]];
            if (chartInitialized == false)
                chart0(pts, channels, tunesw, data['tn'], data['dn'], data['dn2']);
            else
                chart(pts, i+1, label);
            saved[channel[i]] = label;
        }
        
        loopMsg = "read:json+"+tuningUser+".vnmrj.trtune+"+fileNum;
        fileNum += 1;
        
        getDataTimerId = setInterval(longPollingLoop,250); // fire another loop

    } 
    else if (messageState == state.DISCONNECTED) 
    {
        location.reload(true);  // reload page but not from cache
    }
}

// COMMON FUNCTIONS

function chart(fidData, channel, nucleus) 
{
    for (var i = 0; i < fidData.length; i++) 
        fidData[i] = (+fidData[i]);  // convert to numbers
    
    console.log('received '+fidData.length+' points from '+nucleus);
    updateChart(fidData, channel);
}

function chart0(fidData, channels, tunesw, tn, dn, dn2)
{
    for (var i = 0; i < fidData.length; i++) 
        fidData[i] = (+fidData[i]);  // convert to numbers
    
    console.log('received '+fidData.length+' initial points from '+tn);
    chartFunction(fidData, channels, tunesw, tn, dn, dn2);
    chartInitialized = true;
}


// WEB SOCKET FUNCTIONS

function startRequest()
{
    messageState = state.COLLECTING;
    sendMessage("trtune:json"); // str = "tune " + tuningUser + ".vnmrj.trtune 0";
    return messageState;
}

function setup(url)
{
    output = document.getElementById("output");
    ws = new WebSocket(url);

    // Listen for the connection open event then call the sendMessage function
    ws.onopen = function(e) 
    {
        console.log("Connected");
    };

    // Listen for the close connection event
    ws.onclose = function(e) 
    {
        console.log("Disconnected: " + e.reason);
    };

    // Listen for connection errors
    ws.onerror = function(e) 
    {
        console.log("Error ");
        alert("Connection Error - Not Connected");
    };

    // Listen for new messages arriving at the client
    ws.onmessage = function(e) 
    {
        var input = e.data;
        
        if (input == 'timedout')
        {
            handleTimeout();
            return;
        }

        if (messageState == state.CONNECTED) // connected 
        { 
            startRequest(); // sets messageState to 1
        } 
        else if (messageState == state.COLLECTING) // query number of active channels 
        {
        	try
        	{
                msg = JSON.parse(e.data);
            }
        	catch(e) 
        	{
                console.log('A JSON Parse Error Has Occurred: '+ e.message);
                handleError();
                return;
        	}

            var key = msg[0]
            var data = msg[1]
            tuningUser = data['trtune'];
            tmpchannels = +data['nf'];
            if ((chartInitialized == true) && (channels != tmpchannels))
            {
                location.reload();
            }
            channels = tmpchannels;
        
            console.log("Received trtuner="+tuningUser+" channels="+channels+" tunesw="+tunesw
                        +" tn="+data['tn']+" dn="+data['dn']+" dn2="+data['dn2']
                        +" dn3="+data['dn3']+" dn4="+data['dn4']);

            var tmptunesw = data['tunesw'] / 1e6;
            if ((chartInitialized == true) && (tunesw != tmptunesw))
            {
                location.reload();
            }
            tunesw = tmptunesw;

            var channel = ['tn', 'dn', 'dn2', 'dn3', 'dn4'];
            if (chartInitialized == true) 
            {
                for (var i = 0; i < channels; i++) 
                {
                    if (data[channel[i]] != saved[channel[i]]) 
                        location.reload();
                }
            }
           
            for (var i = 0; i < channels; i++) 
            {
                var label = data[channel[i]];
                var pts = data['data'][channel[i]];
                if (chartInitialized == false)
                    chart0(pts, channels, tunesw, data['tn'], data['dn'], data['dn2']);
                else
                    chart(pts, i+1, label);
                saved[channel[i]] = label;
            }
            
            sendMessage("read:json "+tuningUser+".vnmrj.trtune "+fileNum);
            fileNum += 1;
        }
        else if (messageState == state.DISCONNECTED) 
        {
            location.reload(true);  // reload page but not from cache
        }
    };
}

function sendMessage(msg) 
{
    ws.send(msg);
    console.log("Message sent: " + msg);
}

function stopRequest() 
{
    if (usingPolling == false) 
    {
        console.log("Closing WebSocket");
        messageState = state.DISCONNECTED;
        ws.close();
    } 
    else 
    {
        stopTuningListening();
    }
}
