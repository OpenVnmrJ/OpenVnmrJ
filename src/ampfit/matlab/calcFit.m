% ==================================================================
% Generate amplitude and phase calbration file from experimental data
% ==================================================================
function calcFit(argstr)

cpoints=128; % number of points in calibration files
qspans=8;   % number of spans in quad fit
qorders=2;   % number of orders in quad fit
porders=10;  % number of orders in poly fit

minslope=false;
fitmodel='Q';
version='v1.0';
i=1;
plotname='calcFit';
fitplots='';
savejpgs='';
fitdata_file='fitdata';
infofile='fitinfo';
ampdata_file='ampdata';
tfact=0.999;
if(nargin==0)
   show_usage();
   return;
end

% ====================================================================
% 1. extract x,y,p data vectors from experimental data
% ====================================================================

args=getArgs(argstr);

while i<= length(args)
    arg=args{i};
    if(strcmp(arg,'-u'))
        show_usage();
        return;
    elseif(strcmp(arg,'-v'))
        disp(version);
        return;
    elseif(strcmp(arg,'-j'))
        savejpgs=arg;
    elseif(strcmp(arg,'-f'))
        fitplots=arg;
    elseif(strcmp(arg,'-m'))
        minslope=true;
    elseif (strcmp(arg,'-P'))
        fitmodel='P';
    elseif (strcmp(arg,'-I'))
        fitmodel='I';
    elseif (strcmp(arg,'-Q'))
        fitmodel='Q';
    elseif (strcmp(arg,'-n'))
        i=i+1;
        cpoints=str2double(args{i});
    elseif (strcmp(arg,'-tfact'))
        i=i+1;
        tfact=str2double(args{i});        
    elseif (strcmp(arg,'-qspans'))
        i=i+1;
        qspans=str2double(args{i});
    elseif (strcmp(arg,'-qorders'))
        i=i+1;
        qorders=str2double(args{i});
    elseif (strcmp(arg,'-porders'))
        i=i+1;
        porders=str2double(args{i});
    elseif(strcmp(arg,'-p'))
        i=i+1;
        plotname=args{i};
    elseif(strcmp(arg,'-af'))
        i=i+1;
        ampdata_file=args{i};
    elseif(strcmp(arg,'-cf'))
        i=i+1;
        fitdata_file=args{i};
    end
    i=i+1;
end

expdata=load(ampdata_file);
if isempty(expdata)
    err=strcat('could not load data file: ',ampdata_file);
    disp(err)
    return;
end 

[n,m] = size(expdata);
p=[];

% get errors in histc if x data is not unique and sorted
expdata=sortrows(expdata);

x=expdata(:,1);
y=expdata(:,2); 
if m>2
   p=expdata(:,3);
end

dpoints=length(x);
 
mxpwr=x(dpoints);

if(fitmodel(1)=='Q')
    cv=quadFit(ampdata_file,cpoints,qspans,qorders);
    cx=cv(:,1);
    cy=cv(:,2);
    if m >2 
        cp=cv(:,3);
    end
elseif (fitmodel(1)=='P')
    cv=polyFit(ampdata_file,cpoints,porders);
    cx=cv(:,1);
    cy=cv(:,2);
    if m >2 
        cp=cv(:,3);
    end
else
    cx=0:mxpwr/(cpoints-1):mxpwr; 
    cx=cx';
    cy=interp1(x,y,cx); % fixes first point
    % fix first points 
    for i=1:cpoints
        if(isnan(cy(i)))
            cy(i)=cx(i);
        else
            break;
        end
    end

    if m>2
       cp=interp1(x,p,cx,'linear',p(1));
    end
end

% ====================================================================
% 2. save amplifier calibration file
% ====================================================================

savedata=zeros(cpoints,m);

savedata(:,1)=cx;
savedata(:,2)=cy;
if(minslope)
    tfact=min(cy./cx);
else
    tfact=max(cy)/max(cx);
end
if m > 2
    savedata(:,3)=cp;
end
save(fitdata_file,'savedata','-ascii');

if fitmodel(1)=='Q'
    fprintf('calcFit QuadFit[spans=%d] file:%s length:%d\n',...
        qspans,fitdata_file,cpoints);
elseif fitmodel(1)=='P'
    fprintf('calcFit PolyFit[order=%d] file:%s length:%d\n',...
        porders,fitdata_file,cpoints);
else
    fprintf('calcFit InterpFit file:%s length:%d\n',...
        fitdata_file,cpoints);
end

% save function attributes
% ====================================================================
% 2. generate correction info file
% ====================================================================
info=sprintf('fitmodel  %s\n',fitmodel);
info=sprintf('%spoints    %d\n',info,cpoints);
info=sprintf('%sxmax      %g\n',info,cx(cpoints));
info=sprintf('%symax      %g\n',info,cy(cpoints));
info=sprintf('%stfact     %d\n',info,tfact);
info=sprintf('%sqspans    %d\n',info,qspans);
info=sprintf('%sqorders   %d\n',info,qorders);
info=sprintf('%sporders   %d\n',info,porders);

fid=fopen(infofile,'w');
fprintf(fid,'%s',info);
fclose(fid);

% ====================================================================
% 3. (optional) plot results
% ====================================================================

if ~isempty(fitplots)
    plotstr=sprintf('-af %s -cf %s %s %s -p %s',...
        ampdata_file,fitdata_file,fitplots,savejpgs,plotname); 
    fidelityPlots(plotstr);
end

end

function show_usage
    usage=sprintf('Usage:\tcalcFit(options)\n');
    usage=sprintf('%s\toptions:    \n',usage);
    usage=sprintf('%s\t -u              print usage and return\n',usage);
    usage=sprintf('%s\t -v              print version and return\n',usage);
    usage=sprintf('%s\t -af name        name of raw input data file[ampdata]\n',usage);
    usage=sprintf('%s\t -cf name        name of output file[fitdata]\n',usage);
    usage=sprintf('%s\t -p name         plot name (optional)\n',usage);
    usage=sprintf('%s\t -f              show fit plots\n',usage);
    usage=sprintf('%s\t -j              create jpg images for plots\n',usage);
    usage=sprintf('%s\t -Q              use piecewise polynomial fit model [default]\n',usage);
    usage=sprintf('%s\t -qspans value   number of segments in piecewise polynomial fit [8]\n',usage);
    usage=sprintf('%s\t -qorders value  number of polynomial orders in piecewise polynomial fit [2]\n',usage);
    usage=sprintf('%s\t -P              use single polynomial fit model\n',usage);
    usage=sprintf('%s\t -porders value  number of polynomial orders in single polynomial fit [10]\n',usage);
    usage=sprintf('%s\t -I              use interpolate fit model\n',usage);
    usage=sprintf('%s\t -n value        number of points in output data[128]\n',usage);
    usage=sprintf('%s\t -tfact value    target line scaling factor[0.999]\n',usage);
    disp(usage)
end

