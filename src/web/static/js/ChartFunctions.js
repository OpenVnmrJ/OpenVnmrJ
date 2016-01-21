
var d3vis;
var line;
var ymax;
var ymin;
var xrange;
var padding = 5;

var color_hash = 
{  
	0 : ["Chan 1", "steelblue"],
	1 : ["Chan 2", "lightgreen"],
	2 : ["Chan 3", "magenta"],
	3 : ["Chan 4", "yellow"],
	4 : ["Chan 5", "blue"]
}              

function chartFunction(datas, numChannels, span, ch1, ch2, ch3)
{
	var adiv = document.getElementById("vis");
	var curr_width = adiv.clientWidth;
	ymax = 1.0;
	ymin = 5.0;
	//xrange = 10.0;
	xrange = span;
	color_hash[0][0] = ch1;
	color_hash[1][0] = ch2;
	color_hash[2][0] = ch3;
	var datas2 = new Array(datas.length);
	var datas3 = new Array(datas.length);
    //for (var i = 0; i < arr.length; i++)

    for (var i = 0; i < datas.length; i++) { datas[i] = +datas[i]; } // convert to numbers

	for (var i = 0; i < datas.length; i++) {
	    datas2[i] = 100;
	    datas3[i] = 100;
	    //var num = parseFloat(arr[i]);
		if (datas[i] > ymax) ymax = datas[i];
		if (datas[i] < ymin) ymin = datas[i];
	}
	
	var yScale = d3.scale.linear()
    .domain([0, ymax])
    .range([0, 100]);

	var xScale = d3.scale.linear()
	    .domain([0, datas.length+1])
	    //.domain([0, datas.length])
	    .range([-1*(xrange/2), xrange/2]);


	//var dat = arr,
	var dat = datas,
		w = curr_width,
		h = 528,
		//margin = 20,
		margin = 10,
		//y = d3.scale.linear().domain([0, 100]).range([0 + margin, h - margin]),
		y = d3.scale.linear().domain([-12, 100]).range([-12 + margin, h - margin]),
		//x = d3.scale.linear().domain([0, dat.length]).range([0 + margin, w - margin])
		x = d3.scale.linear().domain([-1*(xrange/2), xrange/2]).range([0 + margin, w - margin])

	var dat2 = datas2,
		w = curr_width,
		h = 528,
		//margin = 20,
		margin = 10,
		//y = d3.scale.linear().domain([0, 100]).range([0 + margin, h - margin]),
		y = d3.scale.linear().domain([-12, 100]).range([-12 + margin, h - margin]),
		//x = d3.scale.linear().domain([0, dat.length]).range([0 + margin, w - margin])
		x = d3.scale.linear().domain([-1*(xrange/2), xrange/2]).range([0 + margin, w - margin])

	var dat3 = datas3,
		w = curr_width,
		h = 528,
		//margin = 20,
		margin = 10,
		//y = d3.scale.linear().domain([0, 100]).range([0 + margin, h - margin]),
		y = d3.scale.linear().domain([-12, 100]).range([-12 + margin, h - margin]),
		//x = d3.scale.linear().domain([0, dat.length]).range([0 + margin, w - margin])
		x = d3.scale.linear().domain([-1*(xrange/2), xrange/2]).range([0 + margin, w - margin])

	//d3vis = d3.select("body")
	d3vis = d3.select("#vis")
		.append("svg:svg")
		.attr("width", w)
		.attr("height", h)
		.style("background-color", "black"); 
		
	var g = d3vis.append("svg:g")
		.attr("transform", "translate(0,528)");
	
	line = d3.svg.line()
		.interpolate("basis")
		.x(function(d,i) {return (xScale(x(i))); })
		//.y(function(d) {return -1*y(d); })
		.y(function(d) {return -1*(yScale(y(d))); })
		
	g.append("svg:path")
        .attr("d", line(dat))
        .attr("class", "blue line")
		    .style("shape-rendering", "crispEdges" )
        .style("stroke-width", 3.0)
    	.style("stroke", color_hash[0][1]);
//    	.style("fill", "blue");

	if (numChannels > 1) {
	    g.append("svg:path")
            .attr("d", line(dat2))
            .attr("class", "green line")
            .style("shape-rendering", "crispEdges" )
            .style("stroke-width", 3.0)
            .style("stroke", color_hash[1][1]);
	    //        .style("fill","green");
	}
	
	if (numChannels > 2) {
	    g.append("svg:path")
            .attr("d", line(dat3))
            .attr("class", "magenta line")
            .style("shape-rendering", "crispEdges" )
            .style("stroke-width", 3.0)
            .style("stroke", color_hash[2][1]);
	    //        .style("fill","yellow");
	}

	// axis line
	g.append("svg:line")
	    //.attr("x1", x(0))
	    .attr("x1", x(-1*(xrange/2)))
	    .attr("y1", -1 * y(-10))
	    //.attr("x2", x(w))
	    .attr("x2", x(-1*(xrange/2)))
	    .attr("y2", -1 * y(-10))
	    .style("stroke", "white");
	
	// Y vp vertical position line
	g.append("svg:line")
	//.attr("x1", x(0))
	.attr("x1", x(-1*(xrange/2)) + (xrange/10))
	//.attr("y1", -1 * y(0))
	.attr("y1", -1 * yScale(y(0)))
	//.attr("x2", x(w))
	.attr("x2", x(xrange/2) - (xrange/10))
	//.attr("y2", -1 * y(0))
	.attr("y2", -1 * yScale(y(0)))
    .style("stroke-width", 3.0)
	.style("stroke", "cyan");
	//.style("stroke", "lightsteelblue");

//	g.append("svg:line")
//		.attr("x1", x(-1*(xrange/2)))
//		//.attr("y1", -1 * y(ymin))
//		.attr("y1", -1 * (yScale(y(ymin))))
//		.attr("x2", x(xrange/2))
//		//.attr("y2", -1 * y(ymin))
//		.attr("y2", -1 * (yScale(y(ymin))))
//		.style("stroke", "lightsteelblue");

	// X zero line
    g.append("svg:line")
	    .attr("x1", x(0))
	    //.attr("y1", -1 * y(0))
	    .attr("y1", -1 * y(-10))
    	//.attr("y1", -1 * y(ymax))
	    .attr("x2", x(0))
	    //.attr("y2", -1 * y(d3.ymax(dat)))
	    .attr("y2", -1 * y(ymax))
	    .style("stroke", "white");

	g.selectAll(".xLabel")
    	.data(x.ticks(5))
    	.enter().append("svg:text")
    	.attr("class", "xLabel")
    	.text(String)
    	.attr("x", function(d) { return x(d) })
    	//.attr("y", 0)
    	.attr("y", -12)
    	.attr("text-anchor", "middle")
	    .style("stroke", "white");

	//Create X axis label   
	d3vis.append("text")
	    .attr("transform", "translate("+ (w-50) +","+(h-(padding/3))+")")  // below axis on right
	    .style("text-anchor", "middle")
	    .text("delta MHz")
		.style("stroke", "white");

//	g.selectAll(".yLabel")
//	    .data(y.ticks(6))
//	    .enter().append("svg:text")
//	    .attr("class", "yLabel")
//	    .text(String)
//	    //.attr("x", 0)
//	    .attr("x", -4.0)
//	    .attr("y", function(d) { return -1 * y(d) })
//	    .attr("text-anchor", "right")
//	    .attr("dy", 6)
//	    .style("stroke", "white");
	
	g.selectAll(".xTicks")
	    .data(x.ticks(10))
	    .enter().append("svg:line")
	    .attr("class", "xTicks")
	    .attr("x1", function(d) { return x(d); })
	    //.attr("y1", -1 * y(0))
	    .attr("y1", -1 * y(-10))
	    .attr("x2", function(d) { return x(d); })
	    //.attr("y2", -1 * y(-0.3))
	    .attr("y2", -1 * y(-10.8))
	    .style("stroke", "white");

//	g.selectAll(".yTicks")
//	    .data(y.ticks(6))
//	    .enter().append("svg:line")
//	    .attr("class", "yTicks")
//	    .attr("y1", function(d) { return -1 * y(d); })
//	    //.attr("x1", x(-0.3))
//	    .attr("x1", x(-4.3))
//	    .attr("y2", function(d) { return -1 * y(d); })
//	    //.attr("x2", x(0))
//	    .attr("x2", x(-4.0))
//	    .style("stroke", "white");
	    
	//d3vis.selectAll("xLabel").style("color", "white");
	
	// add legend 
	for (var i = 0; i<numChannels; i++)
	{
		d3vis.append("text")
	    .attr("transform", "translate("+ (w-50) +","+((i * 20) + 20) + ")")  // top right
	    .style("text-anchor", "middle")
	    .text(color_hash[i][0])
		.style("stroke", color_hash[i][1]);
	}
	
}

function updateChart(datas, channel)
{
	//var max = 1.0;
    //var arr = new Array(256);
    //for (var i = 0; i < arr.length; i++)
	//for (var i = 0; i < datas.length; i++) {
	    //arr[i] = datas[i][' Y'];	
	    //var num = parseFloat(arr[i]);
	    //if (datas[i] > max) max = datas[i];
	//}

    //var dat = arr,
	
	for (var i = 0; i < datas.length; i++) {
		if (datas[i] > ymax) ymax = datas[i];
		if (datas[i] < ymin) ymin = datas[i];
	}

    if (channel == 1) {
        d3vis.selectAll("path.blue.line")
            .data([datas])
            .attr("d", line);
    }
    else if (channel == 2) {
        d3vis.selectAll("path.green.line")
            .data([datas])
            .attr("d", line);
    }
    else if (channel == 3) {
        d3vis.selectAll("path.magenta.line")
            .data([datas])
            .attr("d", line);
    }

}		
