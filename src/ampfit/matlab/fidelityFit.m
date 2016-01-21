% ==================================================================
% Generate phase and amplitude calibration file using Fidelity method
% ==================================================================
function fidelityFit(argstr)
% minlin=250;  % min dac units for "linear" region
% maxlin=280;  % max dac units for "linear" region 
minlin=100;  % min dac units for "linear" region
maxlin=180;  % max dac units for "linear" region 
fitplots='';
tblplots='';
rawplots=false;
shapeplots='';
savejpgs='';
fitmodel='Q';
WorkingDir='';
version='v1.0';
cpoints = 128;
qspans=8;   % number of spans in quad fit
qorders=2;   % number of orders in quad fit
porders=10;  % number of orders in poly fit
minslope='';

tfact=0.999;

cdir=pwd;
shapein =  'expPuls.RF';
shapeout = 'expPulsFit.RF';
fitdata_file =  'fitdata';
ampdata_file = 'ampdata';
tablesfile =  'inline';
infofile =  'expinfo';
plotname='';

tmin=20;
tmax=63;
tstep=0.5;
pstep=1;
tmode='std';

outdir = '';

refData='REFdata';
fidData='DUTdata';
i=1;
if(nargin==0)
   show_usage(tmin,tmax,pstep);
   return;
end
args=getArgs(argstr);

while i<= length(args)
    arg=args{i};
    if(strcmp(arg,'-u'))
        show_usage(tmin,tmax,pstep);
        return;
    elseif(strcmp(arg,'-v'))
        disp(version);
        return;
    elseif(strcmp(arg,'-a'))
        tablesfile='tables.txt';
    elseif(strcmp(arg,'-b'))
        tablesfile='tables.bin';
    elseif(strcmp(arg,'-m'))
        minslope=arg;
    elseif(strcmp(arg,'-f'))
        fitplots=arg;
    elseif(strcmp(arg,'-r'))
        rawplots=true;
    elseif(strcmp(arg,'-t'))
        tblplots=arg;
    elseif(strcmp(arg,'-s'))
        shapeplots=arg;
    elseif(strcmp(arg,'-j'))
        savejpgs=arg;
    elseif (strcmp(arg,'-P'))
        fitmodel='P';
    elseif (strcmp(arg,'-I'))
        fitmodel='I';
    elseif (strcmp(arg,'-Q'))
        fitmodel='Q';
    elseif (strcmp(arg,'-n'))
        i=i+1;
        cpoints=str2double(args{i});
    elseif (strcmp(arg,'-mn'))
        i=i+1;
        minlin=str2double(args{i});
    elseif (strcmp(arg,'-mx'))
        i=i+1;
        maxlin=str2double(args{i});
    elseif (strcmp(arg,'-tfact'))
        i=i+1;
        tfact=str2double(args{i});        
    elseif (strcmp(arg,'-W'))
        i=i+1;
        outdir = args{i};
    elseif (strcmp(arg,'-tmin'))
        i=i+1;
        tmin=str2double(args{i});
    elseif (strcmp(arg,'-tmax'))
        i=i+1;
        tmax=str2double(args{i});
    elseif (strcmp(arg,'-pstep'))
        i=i+1;
        pstep=str2double(args{i});
    elseif (strcmp(arg,'-si'))
        i=i+1;
        shapein=args{i};
    elseif (strcmp(arg,'-so'))
        i=i+1;
        shapeout=args{i};
    elseif (strcmp(arg,'-R'))
        i=i+1;
        refData=args{i};
    elseif (strcmp(arg,'-D'))
        i=i+1;
        fidData=args{i};
    elseif (strcmp(arg,'-p'))
        i=i+1;
        plotname=args{i};
    elseif arg(1) ~= '-'
        WorkingDir=arg;
    elseif (strcmp(arg,'-qspans'))
        i=i+1;
        qspans=str2double(args{i});
    elseif (strcmp(arg,'-qorders'))
        i=i+1;
        qorders=str2double(args{i});
    elseif (strcmp(arg,'-porders'))
        i=i+1;
        porders=str2double(args{i});
   end
    i=i+1;
end

if isempty(plotname) && ~isempty(WorkingDir)
    plotname=WorkingDir;
elseif isempty(plotname) && ~isempty(outdir)
    plotname=outdir;
end

plotname=sprintf('%s[%c]',plotname,fitmodel);

setappdata(0,'UseNativeSystemDialogs',false); 
warning off MCR:ClassInUseAtExit 
warning off MATLAB:ClassInstanceExists

refpath=strcat(refData,'.fid/');
fidpath=strcat(fidData,'.fid/');

if(~isempty(WorkingDir))
    cd(WorkingDir);
end

[p a c]=textread(shapein,'%f %f %f','commentstyle','shell');
if c(1)==0
    shapedata1=a(2:length(a));
else
    shapedata1=a(1:length(a));
end

maxdac=4095;   % fixed for now
[tpwr,~] = Vread_procpar(refpath,'procpar','tpwr'); 
[tpwrf,~] = Vread_procpar(refpath,'procpar','tpwrf'); 
rpower=tpower(tpwr,tpwrf);
fprintf('REF tpwr:%g tpwrf:%g power:%g\n',tpwr,tpwrf,rpower);
[tpwr,~] = Vread_procpar(fidpath,'procpar','tpwr'); 
[tpwrf,~] = Vread_procpar(fidpath,'procpar','tpwrf'); 
dpower=tpower(tpwr,tpwrf);
fprintf('DUT tpwr:%g tpwrf:%g power:%g\n',tpwr,tpwrf,dpower);


thisdir=pwd;
cd (refpath);
refdata=readFIDformat('fid',0);
cd (thisdir);
fd1=sqrt(refdata.*conj(refdata));
ls=2; % best guess bad start points (rcvr delay)

fd1=fd1(ls:length(fd1));
mx=max(fd1);
fs=mx/4095;
fd1=fd1./fs;

first=0;
last=0;
mid=0;
minval=30;
% find window in ref data that exactly contains shape
for i=1:length(fd1)
    if(first==0 && fd1(i) >minval)
        first=i;
    end
    if(mid==0 && fd1(i)>200)
        mid=i;
    end
    if fd1(i) <minval && mid>0 && last==0
        last=i-1;
    end
end
tmp=sprintf('first: %d last: %d pnts: %d',first+ls,last+ls,last-first);
disp(tmp);

refdata=refdata(ls+first:ls+last);

% find linear region and first max value in shape

s3=1;
s2=1;
s1=20;
sl=length(shapedata1);

for i=1:sl
    if(shapedata1(i) <=minlin)
        s1=i;
    end;
    if(shapedata1(i) <=maxlin)
        s2=i;
    end
    s3=i;
    if(shapedata1(i) >=maxdac)
        break;
    end
end


n = length(refdata);

compr = n/sl;  % shape-to-data numpoints compression factor
bgnlin=floor(s1*compr);  % index of first linear point in shape
endlin=floor(s2*compr);   % index of last linear point in shape
maxindex=floor(s3*compr); % index of first maxdac point in shape

% ====================================================================
% 1. Obtain scaling factors
% ====================================================================
% Problem: 
%   1. need to find a scaling factor that converts amplifier response
%      data to shape (dac) units
% Algorithm:
%   1. obtain the scaling factor between the shape and the 
%      reference data using all the data in each vector. 
%    - It is assumed that all the ref data is linear WRT the shape data
%   2. Measure the scaling factor between the reference and fid NMR data
%      using a ratio in just the linear region. 
%    - It is assumed that the shape->receiver scaling factor
%      will be the same for both NMR data sets and so will divide out
%   3. The shape-to-fid scaling factor is obtained by multiplying the 
%      scaling factors determined in the above two steps
% 
% ====================================================================

cd (fidpath);
fiddata=readFIDformat('fid',0);
cd (thisdir);

%fiddata=fiddata(ls+1:n); % bgnlin shift out pre-shape points
fiddata=fiddata(ls+first:ls+last); % bgnlin shift out pre-shape points

fiddata1=fiddata(:,1);
d=length(fiddata); % new adjusted data size
bgnlin=floor(bgnlin/2);

% rescale shape data size to match nmr data size

mins=min(shapedata1);

n1=length(shapedata1);

xs1=0:maxdac/(n1-1):maxdac;
xs1=xs1';

xs2=0:maxdac/(d-1):maxdac;
xs2=xs2';

sd = interp1(xs1,shapedata1,xs2,'linear',mins);

s2 = sd(bgnlin:d-bgnlin);
ss2 = sum(s2.*s2);

fd1=sqrt(fiddata1.*conj(fiddata1));
fd1=fd1(bgnlin:endlin);

%refdata=refdata(ls+1:n);
refdata1=refdata(:,1);

rd=sqrt(refdata1.*conj(refdata1));
rd1=rd(bgnlin:endlin);
rd2=rd(bgnlin:d-bgnlin);

rs2=sum(rd2.*s2);

fr1=sum(fd1.*rd1);
rr1=sum(rd1.*rd1);

scale3=fr1/rr1;        % scale3: ref->fid scale (linear part)
scale1=rs2/ss2;        % scale1: shape->ref scale (all data)
scale2=scale1*scale3;  % scale2: shape->fid scale (from scale1*scale3)
scales=sprintf('scale1: %g scale2: %g scale3: %g',scale1,scale2,scale3);
disp(scales);

% ====================================================================
% 2. create standard format amplifier reponse data file
%   - xaxis 0 to 1.0 * tpower(dut_tpwr,dut_tpwrf)
%   - yaxis dut data
% ====================================================================

outdata=zeros(d,2); 
 
mxpwr=dpower;

% rscale=rpower/scale1/maxdac;
% dscale=rpower*rpower/dpower/scale2/maxdac;

rscale=dpower/scale1/maxdac;
dscale=dpower/scale2/maxdac;

outdata(:,1)=fiddata1.*dscale;
outdata(:,2)=refdata1.*rscale;
%cd(WorkingDir);

x=outdata(bgnlin:maxindex,2); % REF
x=sqrt(x.*conj(x)); 

y=outdata(bgnlin:maxindex,1); % DUT
y=sqrt(y.*conj(y));
pend=bgnlin+20;

pf=angle(fiddata1(bgnlin:maxindex));
pr=angle(refdata1(bgnlin:maxindex));

pf=unwrap(pf)*(180.0/pi);
pr=unwrap(pr)*(180.0/pi);

pfs=angle(fiddata1(bgnlin:pend));
prs=angle(refdata1(bgnlin:pend));
pfs=unwrap(pfs)*(180.0/pi);
prs=unwrap(prs)*(180.0/pi);

pd=pf-pr;
psd=pfs-prs;

p=pd-mean(psd);

n=length(x);

xmax=max(x);
xf=mxpwr/xmax;
x=x.*xf;
yf=xf;

if(~isempty(minslope))
    tfact=min(y./x);          %target=minimum slope
else
    tfact=tfact*max(y)/mxpwr; %target=ymax/xmax
end

y=y.*yf;
ampdata=zeros(n,3);
ampdata(:,1)=x; %ref
ampdata(:,2)=y; %dut
ampdata(:,3)=p;

% Since x values may not be monotonicaly increasing in raw data
% need to sort to avoid errors in functions that use histc

ampdata=sortrows(ampdata); 

% cd to calibration data directory

if(~isempty(outdir))
    cd(outdir);
end

% ====================================================================
% 3. (Optional) show original fidelity plots
% ====================================================================
if rawplots
    plotargs=sprintf('-r %s -p %s -cf %s ',...
       savejpgs,plotname,fitdata_file);
    fidelityPlots(plotargs);    
end
save(ampdata_file,'ampdata','-ascii'); % save fit data for plots

% ====================================================================
% 4. generate correction file from experimental data file
% ====================================================================
calcargs=sprintf('%s %s %s -%s -n %d -p %s -tfact %g -porders %d -qorders %d -qspans %d',...
    fitplots,savejpgs,minslope,fitmodel,cpoints, plotname,tfact,porders,qorders,qspans);
calcFit(calcargs);

% ====================================================================
% 5. (Optional) correct shape file used in Fidelity test
% ====================================================================
 if ~strcmp(shapeout,'N')
    shapeargs=sprintf('%s %s -p %s -tfact %g -si %s -so %s',...
        savejpgs,shapeplots,plotname,tfact,shapein, shapeout);
    shapeFit(shapeargs);
 end
% ====================================================================
% 6. (Optional) generate tpwr calibration curves
% ====================================================================
 if ~strcmp(tablesfile,'N') 
     tablesargs=sprintf('%s %s %s -p %s -tmin %g -tmax %g -pstep %d -f %s',...
        savejpgs,tblplots, minslope,plotname,tmin,tmax,pstep,tablesfile);
     powerTables(tablesargs);
 end
% ====================================================================
% 7. generate fidelity info file
% ====================================================================
info='';
info=sprintf('%smethod    fidelity\n',info);
info=sprintf('%sfitmodel  %s\n',info,fitmodel);
info=sprintf('%sshape     %s\n',info,shapein);
info=sprintf('%srpower    %g\n',info,rpower);
info=sprintf('%sdpower    %g\n',info,dpower);
info=sprintf('%stfact     %g\n',info,tfact);
info=sprintf('%stpwr      %g\n',info,tpwr);

fid=fopen(infofile,'w');
fprintf(fid,'%s',info);
fclose(fid);

if(~isempty(WorkingDir))
   cd(cdir);
end

end

function show_usage(tmin,tmax,pstep)
    usage=sprintf('Usage:\tfidelityFit(options)\n');
    usage=sprintf('%s\tdatadir:    fidelity exp directory\n',usage);
    usage=sprintf('%s\toptions:    \n',usage);
    usage=sprintf('%s\t -u              print usage and return\n',usage);
    usage=sprintf('%s\t -v              print version and return\n',usage);
    usage=sprintf('%s\t -W path         path to working directory\n',usage);
    usage=sprintf('%s\t -D path         path to DUT data directory\n',usage);
    usage=sprintf('%s\t -R path         path to REF data directory\n',usage);
    usage=sprintf('%s\t -r              show raw plots\n',usage);
    usage=sprintf('%s\t -f              show fit plots\n',usage);
    usage=sprintf('%s\t -t              show table plots\n',usage);
    usage=sprintf('%s\t -j              save jpg files from figures\n',usage);
    usage=sprintf('%s\t -Q              use piecewise polynomial fit model [default]\n',usage);
    usage=sprintf('%s\t -P              use polynomial fit model\n',usage);
    usage=sprintf('%s\t -I              use interpolate fit model\n',usage);
    usage=sprintf('%s\t -qspans value   number of segments in piecewise polynomial fit [8]\n',usage);
    usage=sprintf('%s\t -qorders value  number of polynomial orders in piecewise polynomial fit [3]\n',usage);
    usage=sprintf('%s\t -porders value  number of polynomial orders in single polynomial fit [10]\n',usage);
    usage=sprintf('%s\t -n value        number of points in fit data\n',usage);
    usage=sprintf('%s\t -si path        path to input shape file\n',usage);
    usage=sprintf('%s\t -so path        path to output shape file [N=no shape correction]\n',usage);
    usage=sprintf('%s\t -mn value       dac value for start of linear data\n',usage);
    usage=sprintf('%s\t -mx value       dac value for end of linear data\n',usage);
    usage=sprintf('%s\t -tmax value     max tpwr in tables [%g]\n',usage,tmax);
    usage=sprintf('%s\t -tmin value     min tpwr in tables [%g]\n',usage,tmin);
    usage=sprintf('%s\t -pstep value    tpwr plot step [%d]\n',usage,pstep);
    disp(usage)
end
