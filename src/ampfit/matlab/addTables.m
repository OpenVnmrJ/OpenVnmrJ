% ==================================================================
% combine two correction tables
% ==================================================================
function addTables(argstr)
tstep=0.5;
tblsize=64; % number of table points per tpwr
pstep=4; % for plots

map1_file='tables.map';
map2_file='VALIDATE/tables.map';
out_file='addtables.map';

tableplots=false;
savejpgs='';
pstep=1;
plotname='Combined';

if nargin==0
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
    elseif(strcmp(arg,'-m1'))
        i=i+1;
        map1_file = args{i};
    elseif(strcmp(arg,'-m2'))
        i=i+1;
        map2_file = args{i};
    elseif(strcmp(arg,'-tout'))
        i=i+1;
        out_file = args{i};
    elseif(strcmp(arg,'-t'))
        tableplots=true;
    elseif(strcmp(arg,'-j'))
        savejpgs=arg;
    elseif(strcmp(arg,'-pstep'))
        i=i+1;
        pstep=str2double(args{i});
    end
    i=i+1;
end
    [map1info,map1data,map1tbls]=readTables(map1_file);
    if isempty(map1tbls)
        err=strcat('error reading map file: ',map1_file);
        disp(err)
        return;
    end
    [map2info,map2data,map2tbls]=readTables(map2_file);
    if isempty(map1tbls)
        err=strcat('error reading map file: ',map2_file);
        disp(err)
        return;
    end
    tmin=map1info(1);
    tmax=map1info(2);
    tstep=map1info(3);
    tsize=map1info(4);
    ntbls=map1info(5);
    nmaps=map1info(6);
    
    % insure that the tables were obtained using the same conditions
    if (min(map1info == map2info)~=1)
        err=strcat('Error: map files must be generated with the same conditions');
        disp(err)
        return;
    end
    
    maxdac=65535;  % 2^16-1
    pscale=360.0/maxdac;

    tblstep=maxdac/tsize;
    dac=tblstep:tblstep:maxdac;
    %dac=dac';
    fid=fopen(out_file,'w');
    fprintf(fid,'file   %s\n','inline');
    fprintf(fid,'tmin   %g\n',tmin);
    fprintf(fid,'tmax   %g\n',tmax);
    fprintf(fid,'tstep  %g\n',tstep);
    fprintf(fid,'tsize  %d\n',tblsize);
    fprintf(fid,'ntbls  %d\n',ntbls);
    fprintf(fid,'nmaps  %d\n',nmaps);
    
    tpwr1=map1data{:,1};
    terr1=map1data{:,3}-map1data{:,1};
    terr2=map2data{:,3}-map2data{:,1};
    terr=terr1+terr2;
    tpwr2=tpwr1+terr;
    ids=map1data{:,2};
        
    for i=1:1:nmaps    
        fprintf(fid,'%-3.1f %-2.0f %-3.1f 1.0\n',tpwr1(i),ids(i),tpwr2(i));
    end
    s=zeros(tsize,2);
    for i=1:1:ntbls
        ta1=bitshift(map1tbls(i,:),-16);
        ta1=ta1-dac;
        ta2=bitshift(map2tbls(i,:),-16);
        ta2=ta2-dac;
        err=ta1+ta2;
        aout=err+dac;
        amp=min(aout,maxdac);
        s(:,1)=amp./maxdac;
        tp1=bitand(map1tbls(i,:),hex2dec('0000ffff'));
        tp1 =double(tp1).*pscale-180;
        tp2=bitand(map2tbls(i,:),hex2dec('0000ffff'));
        tp2 =double(tp2).*pscale-180;
        s(:,2)=tp1+tp2;
        fprintf(fid,'%g %g\n',s');
    end
    fclose(fid);
    if tableplots
        plotargs=sprintf('%s -t -p %s -map %s -pstep %d',...
            savejpgs, plotname,out_file,pstep);
        fidelityPlots(plotargs);
    end

end
function show_usage()
    usage=sprintf('\tUsage: addTables([options])\n');
    usage=sprintf('%s\toptions:    \n',usage);
    usage=sprintf('%s\t -u         print usage and return\n',usage);
    usage=sprintf('%s\t -j         save jpg files from figures\n',usage);
    usage=sprintf('%s\t -t         generate plots\n',usage);
    usage=sprintf('%s\t -m1 file   table1 map file name\n',usage);
    usage=sprintf('%s\t -m2 file   table2 map file name\n',usage);
    usage=sprintf('%s\t -tout file output table map file name\n',usage);
    usage=sprintf('%s\t -pstep     tpwr plot step\n',usage);
    usage=sprintf('%s\t -p name    plot name\n',usage);
    disp(usage)
end


