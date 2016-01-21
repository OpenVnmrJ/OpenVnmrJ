function fidelityPlots(argstr)

rawplots=false;
fitplots=false;
tblplots=false;
shapeplots=false;
savejpgs=false;
usrplots=false;

cdir=pwd;
pstep=1;
version='v1.0';
qspans=0;
porders=8;

if nargin==0
   show_usage();
   return;
end

maxdac=65535;  % 2^16-1

c1=log(10.0)/20.0;  % constant -> 2.303/20.0
ampdata_file = 'ampdata';
fitdata_file ='fitdata';
fitinfo_file ='fitinfo';
mapfile =  'tables.map';
usrfile1 = '';
usrfile2 = '';
usrfile3 = '';
usrfile4 = '';

plotname='';
xaxis='';
yaxis='';
l1='-bs';
l2='-ro';
l3='-gd';
l4='-g+';

shape1 =  'expPuls.RF';
shape2 =  'expPulsTest.RF';
dir='';

fidData = 'DUTdata.fid';
refData = 'REFdata.fid';

i=1;
args=getArgs(argstr);
while i<= length(args)
    arg=args{i};
    if(strcmp(arg,'-u'))
        show_usage();
        return;
    elseif(strcmp(arg,'-v'))
        disp(version);
        return;
    elseif(strcmp(arg,'-f'))
        fitplots=true;
    elseif (strcmp(arg,'-r'))
        rawplots=true;
    elseif (strcmp(arg,'-t'))
        tblplots=true;
    elseif (strcmp(arg,'-g'))
        usrplots=true;
    elseif(strcmp(arg,'-j'))
        savejpgs=true;
    elseif(strcmp(arg,'-l1'))
        i=i+1;
        l1 = args{i};
    elseif(strcmp(arg,'-l2'))
        i=i+1;
        l2 = args{i};
    elseif(strcmp(arg,'-l3'))
        i=i+1;
        l3 = args{i};
    elseif(strcmp(arg,'-p'))
        i=i+1;
        plotname = strrep(args{i}, '<sp>', ' ');
    elseif(strcmp(arg,'-x'))
        i=i+1;
        xaxis = strrep(args{i}, '<sp>', ' ');
    elseif(strcmp(arg,'-y'))
        i=i+1;
        yaxis = strrep(args{i}, '<sp>', ' ');
    elseif(strcmp(arg,'-pstep'))
        i=i+1;
        pstep=str2double(args{i});
    elseif(strcmp(arg,'-d'))
        i=i+1;
        dir=args{i};
    elseif(strcmp(arg,'-cf'))
        i=i+1;
        fitdata_file=args{i};
    elseif(strcmp(arg,'-map'))
        i=i+1;
        mapfile=args{i};
    elseif(strcmp(arg,'-f1'))
        i=i+1;
        usrfile1=args{i};
    elseif(strcmp(arg,'-f2'))
        i=i+1;
        usrfile2=args{i};
    elseif(strcmp(arg,'-f3'))
        i=i+1;
        usrfile3=args{i};
    elseif(strcmp(arg,'-f4'))
        i=i+1;
        usrfile4=args{i};
    elseif(strcmp(arg,'-af'))
        i=i+1;
        ampdata_file=args{i};
    elseif (strcmp(arg,'-s'))
        shapeplots=true;
    elseif(strcmp(arg,'-s1'))
        i=i+1;
        shape1=args{i};
     elseif(strcmp(arg,'-s2'))
        i=i+1;
        shape2=args{i};
    elseif (strcmp(arg,'-R'))
        i=i+1;
        refData=args{i};
    elseif (strcmp(arg,'-D'))
        i=i+1;
        fidData=args{i};
   end
    i=i+1;
end

if(~isempty(dir))
    cd(dir);
    if(isempty(plotname))
        plotname=dir;
    end
end

% ====================================================================
% usr plots - general purpose plot utility
% ====================================================================
if usrplots
    if(~isempty(usrfile1))
       data1=load(usrfile1);
       x=data1(:,1);
       y=data1(:,2);
       figure;
       hold on; 
       plot(x,y,l1);
       if(~isempty(usrfile2))
           data=load(usrfile2);
           y=data(:,2);
           plot(x,y,l2);
       end
       if(~isempty(usrfile3))
           data=load(usrfile3);
           y=data(:,2);
           plot(x,y,l3);
       end
       if(~isempty(usrfile4))
           data=load(usrfile4);
           y=data(:,2);
           plot(x,y,l4);
       end
       if(~isempty(plotname))
           title(plotname);
       end
       if(~isempty(xaxis))
           xlabel(xaxis);
       end
       if(~isempty(yaxis))
           ylabel(yaxis);
       end
    else
        err=strcat('could not load plot file: ',usrfile1);
        disp(err)   
    end
    return;
end

% ====================================================================
% raw plots - REF and DUT
% ====================================================================

if rawplots
    fidpath = strcat(fidData,'/');
    refpath = strcat(refData,'/');

    fiddata=readFIDformat(strcat(fidpath,'fid'),0);

    if isempty(fiddata)
        disp('could not load DUT data');
        return;
    end 
    refdata=readFIDformat(strcat(refpath,'fid'),0);

    if isempty(refdata)
        disp('could not load REF data');
        return;
    end

    fd1=sqrt(refdata.*conj(refdata));
    ls=2; % best guess bad start points (rcvr delay)

    %fd1=fd1(ls:length(fd1));
    mx=max(fd1(ls:length(fd1)));
    fs=mx/4095;
    fd1=fd1./fs;

    first=0;
    last=0;
    mid=0;
    minval=30;
    
    n=length(fd1);
    % find window in ref data that exactly contains shape
    for i=ls:n
        if(first==0 && fd1(i) >minval)
            first=i+2;
        end;
        if(mid==0 && fd1(i)>200)
            mid=i;
        end
        if mid>0 && fd1(i) <minval && last==0
            last=i-2;
        end
    end
    
    refdata=refdata(first:last);
    fiddata=fiddata(first:last);

    ratio1=fiddata./refdata;
    ratio3=20*log10(sqrt(ratio1.*conj(ratio1)));
    
    figure;
    hold off; 
    rd=abs(refdata);
    plothandle = plot(rd,'-b'); 
    hold on; 
    title(strcat(plotname,' Amp & Ref magnitude')); 
    xlim([1 length(refdata)]);
    dd=abs(fiddata);
    pscale=max(rd)/max(dd);
    plot(dd.*pscale,'-r'); 
    xlabel('points'); 
    ylabel('amplitude'); 
    legend('Ref','Amp'); 
    if savejpgs
        saveas(plothandle,'amp_ref_mag','jpg')
    end
    
    
    figure; 
    hold off; 
    plothandle1=plot(ratio3(:,1),'-b'); 
    hold on; 
    title(strcat(plotname,' ratio magnitude')); 
    xlabel('points');
    xlim([1 length(refdata)]);
    hold off; 
    if savejpgs
        saveas(plothandle1,'ratio_mag','jpg'); 
    end
 
    hold off; 
    figure 
    Xscale = sqrt(refdata.*conj(refdata))/max(abs(refdata)); 
    Dlen = size(Xscale);
    hDlen = fix(Dlen(1)/2);
    hold off; 
    plothandle=plot(Xscale(1:hDlen,1),abs(ratio1(1:hDlen,1))-1,'color','b');
    hold on; 
    plot(Xscale(hDlen:Dlen(1),1),abs(ratio1(hDlen:Dlen(1),1))-1,'color','r'); 
    title(strcat(plotname,' linear sweep amplitude')); 
    ylabel('amplitude linear'); 
    legend('Up','Down'); 
    hold off; 
    filename='lin_sweep_amp'; 
    if savejpgs
        saveas(plothandle, filename, 'jpg');
    end
    
    figure; 
    hold off; 
    plothandle=plot(Xscale(1:hDlen,1),unwrap(angle(ratio1(1:hDlen,1)))*(180.0/pi),'color','b'); 

    hold on; 
    plot(Xscale(hDlen:Dlen(1),1),unwrap(angle(ratio1(hDlen:Dlen(1),1)))*(180.0/pi),'color','r');
    title(strcat(plotname,' linear sweep phase')); 
    ylabel('degrees'); 
    legend('Up','Down'); 

    hold off; 
    if savejpgs
        saveas(plothandle, 'lin_sweep_phase', 'jpg');
    end
end

% ====================================================================
% The remaining plots need results from the data analysis
% ====================================================================

% from text file "fitinfo" generated by calcFit
% fitmodel  Q            curve generation model used
% points    128          the number of points in fit curve
% xmax      1412.54      maximum input power in fit curve
% ymax      1255.68      maximum output power in fit curve
% tfact     0.999        scaling factor for target line
% qspans    16           numper of spans in piecewise poly method
% qorders   2            numper of orders in piecewise poly method
% porders   10           numper of orders in full data poly method

fid=fopen(fitinfo_file,'r');
if fid>=0
    C = textscan(fid,'%s %s', 1);
    fitmodel=C{:,2};
    C = textscan(fid,'%s %n', 6);
    fitvars=C{:,2};
    cpoints=fitvars(1);
    xmax=fitvars(2);
    ymax=fitvars(3);
    tfact=fitvars(4);
    qspans=fitvars(5);
    porders=fitvars(6);
    %cf=tfact;%*ymax/xmax; % target line slope
    cf=ymax/xmax; % target line slope
    fclose(fid);
    if strcmp(fitmodel,'Q')==0
        qspans=0;
    end
else
    err=strcat('could not load calibration file: ',fitinfo_file);
    disp(err)
    return;
end

% ====================================================================
% fit plots - results from "calcFit"
% ====================================================================
if fitplots
 
    expdata=load(ampdata_file);
    if isempty(expdata)
        err=strcat('could not load data file: ',ampdata_file);
        disp(err)
        return;
    end 
    [~,m] = size(expdata);
    
    x=expdata(:,1);
    y=expdata(:,2); 
    if m>2
       p=expdata(:,3);
    end

    dpoints=length(x);

    mxpwr=x(dpoints);

    fitdata=load(fitdata_file);
     
    cx=fitdata(:,1);
    cy=fitdata(:,2);
    
    if(qspans >0)
        S=0:mxpwr/(qspans-1):mxpwr;
        [~,k] = histc(S,cx); % cx index at start of span
        if k(1)==0
           k(1)=1;
        end
        qx=cx(k);
        qy=cy(k);
    end
    
    % plot 1: Amplitude transfer function

    figure;
    hold on;
    xrange=mxpwr;
    xlim([0.0 xrange]);
    plothandle=plot(x,y,'ob');
    
    hold on;
    ymax=max(cy);
    xmax=max(cx);
    linx=cx.*(ymax/xmax);
    amperr=abs(cy-linx)./(cx+1e-5);
    
    amperr=100*max(amperr);
    str=sprintf('Amplitude (Max Non-linearity=%-3.1f %%)',amperr);

    
    title(strcat(plotname,str)); 
    xlabel('normalized ref power'); 
    ylabel('normalized amp power'); 
    
    tx=0:mxpwr/10.0:mxpwr;
    %plot(tx,tx,'-+c');
    plot(tx,tx.*tfact,'-+c');

    plot(cx,cy,'r');
   
    legend('Data','Target','Fit','Location','NorthWest'); 
    if savejpgs
        saveas(plothandle,'ampl-fit','jpg')
    end
    % plot 2: Amplitude Error
    figure;
    hold off; 
    dy1=y-x;

    xlim([0.0 xrange]);
    hold on;
    plothandle=plot(x,dy1,'ob');
    
    amperr=abs(cy-cx)./xrange;
    amperr=100*max(amperr);
    str=sprintf('Amplitude Error (Max=%-3.1f %%)',amperr);
   
    title(strcat(plotname,str)); 
    
    xlabel('normalized ref power'); 
    ylabel('normalized amp power error'); 
    
    my2=interp1(cx,cy,x);
    
    dy2=my2-x;
    
    plot(cx,cy-cx,'-r');
    dy3=dy2-dy1;
    plot(x,dy3.*10,'g');
    legend('Data','Fit','Fit Error(x10)','Location','SouthWest'); 
    
    if(qspans >0)
        plot(qx,qy-qx,'*c');
    end
    if savejpgs
        saveas(plothandle,'ampl-err','jpg')
    end
    % plot 3 plot phase response
    if m > 2   
        cp=fitdata(:,3);
        
        figure;
        hold off; 

        plothandle=plot(x,p,'b'); 
        hold on;     
        plot(cx,cp,'r');

        pf=interp1(cx,cp,x); 
        
        d=p-pf;

        plot(x,d.*10,'g'); 
        xlabel('normalized ref power'); 
        ylabel('degrees'); 
        legend('Data','Fit','Fit Error(x10)','Location','NorthWest'); 
        if(qspans >0)
            qp=cp(k);
            plot(qx,qp,'*c');    
        end
        str=sprintf('Phase Error (Max=%-3.1f degrees)',max(abs(cp)));
        title(strcat(plotname,str));
        if savejpgs
            saveas(plothandle,'phase-err','jpg');
        end
    end  
end

% ====================================================================
% shape plots - results from "shapeFit"
% ====================================================================
if shapeplots && ~isempty(shape2)
    [p a c]=textread(shape1,'%f %f %f','commentstyle','shell');
    if isempty(c)
        disp(strcat('could not load shape file: ',shape1));
        return;
    end 

    if c(1)==0
        p=p(2:length(p));
        a=a(2:length(a));
    end   
    shapephs1=p; 
    shapeamp1=sqrt(a.*conj(a));    
    l1=length(shapeamp1);

    [p a c]=textread(shape2,'%f %f %f','commentstyle','shell');
    if isempty(c)
        disp(strcat('could not load shape file: ',shape2));
        return;
    end 

    if c(1)==0
        p=p(2:length(p));
        a=a(2:length(a));
    end    
    shapephs2=p; 
    shapeamp2=sqrt(a.*conj(a));
    l2=length(shapeamp1);
    if l1 ~=l2
        disp('shape files must be equal length');
        return;
    end
    
    plottitle=strcat(plotname,'-',shape1);
    
    sp=length(shapeamp1)+1;
    maxshape=max(shapeamp1);
    
    deg2dac=360.0/maxshape;

    figure;
    hold off; 
    xlim([0.0 sp]);

    hold on;   
    plothandle=plot(shapeamp1,'b');
    str=strcat(' Normalized Pulse Shape Correction:',plottitle);
    title(str); 
    plot(shapeamp2,'r');
    
    df=shapeamp2-shapeamp1;
    plot(df,'g');
    
    pe=shapephs1-shapephs2;
    plot(pe./deg2dac,'c');
    
    lin=shapeamp2./shapeamp1;
    %lin=1.0/tfact-lin./cf;
    lin=1.0-lin./cf;
    plot(lin.*maxshape,'m');
    
    legend('Original','Corrected','Amp-Cor','Phase-Cor','Non-Linearity'); 

    xlabel('point index'); 
    ylabel('amplitude'); 
    if savejpgs
        saveas(plothandle,'shape-fit','jpg');
    end;

end

% ====================================================================
% table plots - results from "powerTables"
% ====================================================================
if tblplots 
    [mapinfo,mapdata,amptbls]=readTables(mapfile);
    if isempty(amptbls)
        err=strcat('error reading table map file: ',mapfile);
        disp(err)
        return;
    end
    tmin=mapinfo(1);
    tmax=mapinfo(2);
    tstep=mapinfo(3);
    tsize=mapinfo(4);
    ntbls=mapinfo(5);
    nmaps=mapinfo(6);

    tp1=mapdata{:,1};
    ids=mapdata{:,2};
    j=-1;
    str='';    
    n=ntbls+mod(pstep,ntbls);   
    for i=1:nmaps
        if ids(i) ~= j
            j=ids(i);
            if mod(j,pstep)==0 || i==nmaps
                lstr=sprintf('%4.1f',tp1(i));
                str=[str;lstr];
            end
        end
    end
    pscale=360.0/maxdac;

    tblstep=maxdac/tsize;
    dac=tblstep:tblstep:maxdac;
    dac=dac';
    
    % show amplitude tables

    figure;
    hold on
    for i=1:pstep:n
        j=i;
        if i>ntbls
            j=ntbls;
        end
        table=bitshift(amptbls(j,:),-16);
        if(j==1)
            plothandle=plot(dac,table,'-+');
        else
            plot(dac,table,'-+');        
        end
        hold all;
    end

    xlabel('index'); 
    ylabel('ampl out (dac units)'); 
    legend(str,'Location','NorthWest'); 
    title(strcat(plotname,' dac correction tables (tpwr)')); 
    if savejpgs
        saveas(plothandle,'ampl-tables','jpg');
    end

    % show amplitude error plots
    figure;
    hold on
    for i=1:pstep:n
        j=i;
        if j>ntbls
            j=ntbls;
        end
        table=bitshift(amptbls(j,:),-16);
        table=table-dac';
        table=table.*(100.0/maxdac);
        if(j==1)
            plothandle=plot(dac,table,'-+');
        else
            plot(dac,table,'-+');        
        end
        hold all;
    end

    xlabel('index'); 
    ylabel('error ampl out (%)'); 
    legend(str,'Location','NorthWest'); 
    title(strcat(plotname,' dac error tables (tpwr)')); 
    if savejpgs
        saveas(plothandle,'ampl-error','jpg');
    end

    % show phase tables

    figure;
    hold off;
    for i=1:pstep:n
        j=i;
        if j>ntbls
            j=ntbls;
        end
        table=amptbls(j,:);
        % convert 16 bit encoded phase error back to degrees
        table=bitand(table,hex2dec('0000ffff'));
        table =double(table).*pscale-180;
        if(i==1)
            plothandle=plot(dac,table,'-+');
        else
            plot(dac,table,'-+');        
        end
        hold all
    end
    legend(str,'Location','NorthWest');
    xlabel('index');
    ylabel('phase error (degrees)');
    title(strcat(plotname,' phase correction tables (tpwr)'));
    legend(str,'Location','NorthWest');
    if savejpgs
        saveas(plothandle,'phase-tables','jpg');
    end

end

if(~isempty(dir))
   cd(cdir);
end
end

function show_usage()
    usage=sprintf('Usage:fidelityPlots(options)\n');
    usage=sprintf('%s\toptions:    \n',usage);
    usage=sprintf('%s\t -u         print usage and return\n',usage);
    usage=sprintf('%s\t -v         print version and return\n',usage);
    usage=sprintf('%s\t -p name    plot name\n',usage);
    usage=sprintf('%s\t -r         show raw plots\n',usage);
    usage=sprintf('%s\t -f         show fit plots\n',usage);
    usage=sprintf('%s\t -t         show table plots\n',usage);
    usage=sprintf('%s\t -s         show shape plots\n',usage);
    usage=sprintf('%s\t -j         save jpg files from figures\n',usage);
    usage=sprintf('%s\t -s1 path   shape1 path\n',usage);
    usage=sprintf('%s\t -s2 path   shape2 path\n',usage);
    usage=sprintf('%s\t -pstep     tpwr plot step\n',usage);
    usage=sprintf('%s\t -af name   name of data input data file[ampdata]\n',usage);
    usage=sprintf('%s\t -cf name   name of calcFit output file[fitdata]\n',usage);
   disp(usage)
end

