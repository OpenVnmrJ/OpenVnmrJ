% ==================================================================
% Correct shaped pulse using amplifier calibration data
% ==================================================================
function shapeFit(argstr)
shapein =  'expPuls.RF';
shapeout = 'Test.RF';

tpwr=63;
tpwrf=4095;
tfact=0.99;

jpgstr='';
shapeplots=false;

plotname='';
fitdata_file='fitdata';

cdir=pwd;
dir='';

if(nargin==0)
   show_usage();
   return;
end

args=getArgs(argstr);

i=1;
while i<= length(args)
    arg=args{i};
    if(strcmp(arg,'-u'))
        show_usage();
        return;
    elseif(strcmp(arg,'-v'))
        disp(version);
        return;
    elseif(strcmp(arg,'-j'))
        jpgstr=arg;
   elseif(strcmp(arg,'-s'))
        shapeplots=true;
    elseif (strcmp(arg,'-si'))
        i=i+1;
        shapein=args{i};
    elseif (strcmp(arg,'-so'))
        i=i+1;
        shapeout=args{i};
    elseif (strcmp(arg,'-tpwr'))
        i=i+1;
        tpwr=str2double(args{i});
    elseif (strcmp(arg,'-tprwf'))
        i=i+1;
        tpwrf=str2double(args{i});
    elseif (strcmp(arg,'-tfact'))
        i=i+1;
        tfact=str2double(args{i});
    elseif(strcmp(arg,'-d'))
        i=i+1;
        dir=args{i};
    elseif(strcmp(arg,'-p'))
        i=i+1;
        plotname=args{i};
    end
    i=i+1;
end
if(~isempty(dir))
    cd(dir);
    if(isempty(plotname))
        plotname=dir;
    end
end

pwr=tpower(tpwr,tpwrf);

[p a c]=textread(shapein,'%f %f %f','commentstyle','shell');
if c(1)==0
    p=p(2:length(p));
    a=a(2:length(a));
end    
shapephs1=p; 
shapeamp1=sqrt(a.*conj(a));    

maxdac=max(shapeamp1);

fitdata=load(fitdata_file); % read calibration file 

[n,m] = size(fitdata);
cx=fitdata(:,1);
cy=fitdata(:,2);
cy(1)=0;
cp=[];
if m>2
   cp=fitdata(:,3);
end
points=length(cx);

mxpwr=cx(points);

maxf=cy(points);

%cf=min(cy./cx);
cf=maxf/mxpwr;


% need this tweak to prevent NAN for mxpwr values (4095) in shape
% problem probably results from precision errors in acsii data files

%fpwr=tfact*pwr/mxpwr;
fpwr=0.9999*pwr/mxpwr;

tscale=fpwr*mxpwr/maxdac;
%tscale=cf*pwr/maxdac

original=shapeamp1;   % original shape    
oldshape=original.*tscale;   % convert from dac to power units

%cf=maxf/mxpwr;
cfs=cx.*cf;
yt=interp1(cy,cx,cfs,'linear',mxpwr); % inverse LUT power-out ->power-in

shape_pts=length(oldshape);
newshape=interp1(cx,yt,oldshape);
newshape=(newshape./tscale); % power to dac units
if ~isempty(cp)   
    phaseshape=interp1(cx,cp,oldshape);
    pe=phaseshape;
else
    pe=zeros(length(oldshape),1);
end

pe=pe+shapephs1;
sp=shape_pts+1;

outshape=ones(sp,3);
outshape(1,1)=0;
outshape(1,2)=maxdac;
outshape(1,3)=0;
outshape(2:sp,1)=pe;
outshape(2:sp,2)=newshape;

fid = fopen(shapeout, 'w');
fprintf(fid, '%6.3f %6.3f %1.1f\n', outshape');
fclose(fid);

if shapeplots
    if ~isempty(plotname)
        plotargs=sprintf('%s -s -s1 %s -s2 %s -p %s',...
            jpgstr,shapein,shapeout,plotname);
    else
        plotargs=sprintf('%s -s -s1 %s -s2 %s', jpgstr,shapein,shapeout);
    end
    fidelityPlots(plotargs);
end

if(~isempty(dir))
   cd(cdir);
end

end

function show_usage
usage=sprintf('Usage:\tshapeFit([options])\n');
usage=sprintf('%s\toptions:    \n',usage);
usage=sprintf('%s\t -u            print usage and return\n',usage);
usage=sprintf('%s\t -v            print version and return\n',usage);
usage=sprintf('%s\t -s            show plots\n',usage);
usage=sprintf('%s\t -j            save figures as jpg files\n',usage);
usage=sprintf('%s\t -p name       plot name\n',usage);
usage=sprintf('%s\t -tpwr value   tpwr level for shape [63]\n',usage);
usage=sprintf('%s\t -tpwrf value  tpwrf level for shape [4095]\n',usage);
usage=sprintf('%s\t -si path      path to input shape file\n',usage);
usage=sprintf('%s\t -so path      path to output shape file\n',usage);
usage=sprintf('%s\t -cf path      path to calibration data file\n',usage);
disp(usage)
end

