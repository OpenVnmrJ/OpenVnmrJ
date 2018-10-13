% ==================================================================
% Generate amplitude and phase correction tables from calibration data
% ==================================================================
function powerTables(argstr)
% defaults
tmin=20;
tmax=63;
tstep=0.5;
tfact=0.99999;
tblsize=64; % number of table points per tpwr
maxtbls=64; % fixed
pstep=1;

maxdac=65535;  % 2^16-1

plotname='';
minslope=false;

tablesfile ='inline';
mapfile='tables.map';
fitdata_file='fitdata';

tableplots=false;
savejpgs='';
pstep=1;
if nargin==0
   show_usage(tmin,tmax,tblsize,tstep,pstep);
   return;
end
args=getArgs(argstr);
binary_tables=false;
add_default=false;

i=1;
while i<= length(args)
    arg=args{i};
    if(strcmp(arg,'-u'))
        show_usage(tmin,tmax,tblsize,tstep,pstep);
        return;
    elseif(strcmp(arg,'-v'))
        disp(version);
        return;
    elseif(strcmp(arg,'-f'))
        i=i+1;
        tablesfile = args{i};
    elseif(strcmp(arg,'-map'))
        i=i+1;
        mapfile = args{i};
    elseif(strcmp(arg,'-c'))
        i=i+1;
        fitdata_file = args{i};
    elseif(strcmp(arg,'-d'))
        add_default=true;
    elseif(strcmp(arg,'-t'))
        tableplots=true;
    elseif(strcmp(arg,'-j'))
        savejpgs=arg;
    elseif(strcmp(arg,'-pstep'))
        i=i+1;
        pstep=str2double(args{i});
    elseif(strcmp(arg,'-tstep'))
        i=i+1;
        tstep=str2double(args{i});
    elseif (strcmp(arg,'-tmin'))
        i=i+1;
        tmin=str2double(args{i});
    elseif (strcmp(arg,'-tmax'))
        i=i+1;
        tmax=str2double(args{i});
    elseif (strcmp(arg,'-tblsize'))
        i=i+1;
        tblsize=str2double(args{i});
    elseif (strcmp(arg,'-maxtbls'))
        i=i+1;
        maxtbls=str2double(args{i});
    elseif(strcmp(arg,'-p'))
        i=i+1;
        plotname=args{i};
    elseif(strcmp(arg,'-m'))
        minslope=true;
    end
    i=i+1;
end

 if ~strcmp(tablesfile,'inline')
    k=strfind(tablesfile,'bin');
    if ~isempty(k)
       binary_tables=true; 
    end
end

fitdata=load(fitdata_file); % read calibration file
if isempty(fitdata)
    return; % cannot read calibration data file
end

[~,m] = size(fitdata);
cx=fitdata(:,1);
cy=fitdata(:,2);
cp=[];
if m>2
   cp=fitdata(:,3);
end
cpoints=length(cx);
mxpwr=cx(cpoints);
maxf=cy(cpoints);

c1=log(10.0)/20.0;  % constant = 2.303/20.0

dtpwr=floor(log(mxpwr)/c1+0.5);
if(tmax>dtpwr)
    tmax=dtpwr;
end
tsw=tmin; % tpwr where step changes from 2*tstep to tstep
ntbls=(tmax-tmin)/tstep+1;

if ntbls>maxtbls
    tdelta=tmax-tmin;
    n1=tdelta/tstep-maxtbls;
    tsw=tmin+2*tstep*n1;
end

tpwrx1 = tmin:2*tstep:tsw;
tpwrx2 = tsw+2*tstep:tstep:tmax; 

tpwrx =[tpwrx1 tpwrx2];
ntbls=length(tpwrx);

n1=length(tpwrx1);
n2=length(tpwrx2);

tx=exp(c1*tpwrx)';  % array of power values for tpwr

cf=tfact;
if(minslope)
    cf=min(cy./cx);
else
    cy=cy.*(mxpwr/maxf);
end
cfs=cx.*cf;   % target line

yt=interp1(cy,cx,cfs,'linear',0); % inverse LUT power-out ->power-in

tblstep=maxdac/tblsize;
dac=tblstep:tblstep:maxdac;
dac=dac';

pscale=360.0/maxdac;

newdac=interp1(cx,yt,dac./maxdac);
tst=newdac(tblsize);% power to dac units
dflt=dac.*(tst); % default table

% generate tables for amplitude correction
cnt=0;
if add_default
    start=2;
else
    start=1;
end

tbldata=zeros(ntbls,4);
tbldata(:,1)=tx;

for i=start:ntbls
    pwr=tbldata(i,1);
    rpwr=1.0;
    tpwr1=log(pwr)/c1; % original tpwr
    tpwr2=tpwr1
    tbldata(i,2)=tpwr1;
    pmx=interp1(cx,yt,pwr,'linear',mxpwr); % target line at tpwr_in
    if(~minslope && pmx>pwr)
        terr=(log(pmx)-log(pwr))/c1; % tpwr error (db)
        terr1=floor(2.0*(terr+0.5))*0.5; % find tpwr just above or below target        
        tpwr2=tpwr1+terr1; % adjusted tpwr
        newpwr=exp(c1*(tpwr2)); % power at adjusted tpwr_in
        rpwr=pwr/newpwr; % scale table values
        tbldata(i,2)=tpwr2;
    end
    tbldata(i,3)=rpwr;
    tbldata(i,4)=tpwr2-tpwr1;
end
% ====================================================================
% generate table info file
% ====================================================================
n1=length(tpwrx1);
n2=length(tpwrx2);

if n1==1 && n2==0
    nmaps=1;
else
    nmaps=2*n1+n2;
end
fid=fopen(mapfile,'w');
fprintf(fid,'file   %s\n',tablesfile);
fprintf(fid,'tmin   %g\n',tmin);
fprintf(fid,'tmax   %g\n',tmax);
fprintf(fid,'tstep  %g\n',tstep);
fprintf(fid,'tsize  %d\n',tblsize);
fprintf(fid,'ntbls  %d\n',ntbls);
fprintf(fid,'nmaps  %d\n',nmaps);

j=0;
missing=64-ntbls;
if missing<0
    missing=0;
end
for i=1:n1
    tpwr1=tpwrx1(i);    % map tpwr
    tpwr2=tbldata(j+1,2); % table tpwr_out
    fprintf(fid,'%-3.1f %-2d %-3.1f 1.0\n',tpwr1,j,tpwr2);
    if nmaps > 1
        fprintf(fid,'%-3.1f %-2d %-3.1f 1.0\n',tpwr1+tstep,j,tpwr2+tstep);
    end
    j=j+1;
end

for i=1:n2
    tpwr1=tpwrx2(i);
    tpwr2=tbldata(j+missing+1,2); % table tpwr_out
    fprintf(fid,'%-3.1f %-2d %-3.1f 1.0\n',tpwr1,j+missing,tpwr2);
    j=j+1;
end

if binary_tables
    % data format in saved binary file is:
    % tpwr[0] [a0p0] [a1p1] ... a[anpn]
    % tpwr[1] [a0p0] [a1p1] ... a[anpn]
    % ...

    fclose(fid);
    fid=fopen(tablesfile,'w','b'); 
    if add_default
        table=uint32(dflt);
        table=bitshift(table,16);
        if m>2 % phase enabled
            table=bitor(table,hex2dec('00008000'));
        end
        cnt=fwrite(fid,table,'uint32');
    end
    for i=start:ntbls
        pwr=tbldata(i,1);
        rpwr=tbldata(i,3);
        tscale=tfact*pwr/maxdac;
        newdac=interp1(cx,yt,dac.*tscale);
        newdac=newdac.*rpwr; % power to dac units
        newdac=newdac./tscale; % power to dac units
        newdac=min(newdac,maxdac); % prevent data overflow
        table=uint32(newdac);
        table=bitshift(table,16);
        table=bitand(table,hex2dec('ffff0000'));
        if m>2 % phase enabled
            % encode phase error as a 16 bit integer
            % 1. assume error < 180
            % 2. add 180 to error so that <180 = neq error >180 + error
            % 3. convert 0->360 encoded error to max dac value (2^16)
            phase=180+interp1(cx,cp,dac.*tscale);
            phase=phase./pscale; % convert to integer
            tphase=uint32(phase);
            tphase=bitand(tphase,hex2dec('0000ffff'));
            table=table+tphase;
        end
        cnt=cnt+fwrite(fid,table,'uint32');
    end
else
    % data format in saved text file is:
    % tpwr[0] a[0] p[0]\n a[1] p[1]\n ..
    % tpwr[1] a[0] p[0]\n a[1] p[1]\n ..
    % ...
    % a: 0 to 1.0 p: -180.0 to 180.0
    if ~strcmp(tablesfile,'inline')
        fclose(fid);
        fid=fopen(tablesfile,'w');
    end
    d=length(dflt);
    s=zeros(d,2);
    if add_default
        s(:,1)=dflt./maxdac;
        cnt=fprintf(fid,'%g %g\n',s');
    end
    for i=start:ntbls
        pwr=tbldata(i,1);
        rpwr=tbldata(i,3);
        fpwr=tfact*pwr/mxpwr; % prevent clipping for maxdac vaues
        tscale=fpwr*(mxpwr/maxdac);
        newdac=interp1(cx,yt,dac.*tscale);
        newdac=newdac.*rpwr; % power to dac units
        newdac=newdac./tscale; % power to dac units
        newdac=min(newdac,maxdac);
        s(:,1)=newdac./maxdac;
        phase=zeros(d,1);
        if m>2
            phase=interp1(cx,cp,dac.*tscale);
        end
        s(:,2)=phase;
        cnt=cnt+fprintf(fid,'%g %g\n',s');
        fprintf('%g %g\n',tbldata(i,2),tbldata(i,4),max(s(:,1)));
    end
end

fprintf('%d X %d tables saved in file: %s (%d bytes)\n',...
        ntbls, tblsize, tablesfile,cnt);

if tableplots
    plotargs=sprintf('%s -t -p %s -tf %s -pstep %d',...
        savejpgs, plotname,tablesfile,pstep);
    fidelityPlots(plotargs);
end

end

function show_usage(tmin,tmax,tblsize,tstep,pstep)
    usage=sprintf('\tUsage: powerTables([options])\n');
    usage=sprintf('%s\toptions:    \n',usage);
    usage=sprintf('%s\t -u         print usage and return\n',usage);
    usage=sprintf('%s\t -v         print version and return\n',usage);
    usage=sprintf('%s\t -j         save jpg files from figures\n',usage);
    usage=sprintf('%s\t -t         generate plots\n',usage);
    usage=sprintf('%s\t -f file    tables file name (ascii if *.txt)\n',usage);
    usage=sprintf('%s\t -map file  output file name(map)\n',usage);
    usage=sprintf('%s\t -p name    plot name\n',usage);
    usage=sprintf('%s\t -tmax      max tpwr in tables [%g]\n',usage,tmax);
    usage=sprintf('%s\t -tmin      min tpwr in tables [%g]\n',usage,tmin);
    usage=sprintf('%s\t -d         add default base table\n',usage);
    usage=sprintf('%s\t -tblsize   number of table points per tpwr [%d]\n',usage,tblsize);
    usage=sprintf('%s\t -tstep     tpwr step resolution [%g]\n',usage,tstep);
    usage=sprintf('%s\t -pstep     tpwr plot step [%d]\n',usage,pstep);
    disp(usage)
end

