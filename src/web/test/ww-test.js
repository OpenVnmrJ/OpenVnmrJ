var i=1;
var status=1;

function timedCount()
{
    if (status > 0) {
        i=i+1;
        postMessage({'type':'data', 'value':i});
        setTimeout("timedCount()",500);
        if (i>5) {
            status = 0;
            postMessage({'type':'alert', 'value':"Connection timed out"});
            log('hit 5 sec mark ');
        }
    } else {
        log('.');
        setTimeout("timedCount()",500);
    }
}

function log(msg) {
    postMessage({'type':'log', 'value':msg});
};

onmessage = function(event) {
    if (event.data == '') {
	
    } else if (event.data == 'pause') {
        i=0;
        status=0;
    } else if (event.data == 'resume') {
        status=1;
    }
    postMessage({'type':'ack', 'value':"Got an event"});
};

timedCount();
