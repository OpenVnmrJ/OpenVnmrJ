% ==================================================================
% generate table data from a map file
% - called by fidelityPlots and addTables
% TODO:
%  1. change tbldata format from uint32 to real,real array
%  2. modify fidelityPlots to use real data format
% ==================================================================
function [mapinfo,mapdata,tbldata]=readTables(mapfile)
    mapinfo=[];
    mapdata=[];
    tbldata=[];
    fid=fopen(mapfile,'r');
    if fid <0
        err=strcat('could not load tables map file: ',mapfile);
        disp(err)
        return;
    end
    binary_tables=false;
    S=textscan(fid,'%s %s\n', 1);
    tmp=S{:,2};
    tablesfile=char(tmp(1));
    V = textscan(fid,'%s %n\n', 6);
    
    if ~strcmp(tablesfile,'inline')
        k=strfind(tablesfile,'bin');
        if ~isempty(k)
           binary_tables=true;
        end
    end
    
    mapinfo=V{:,2};

    tsize=mapinfo(4);
    ntbls=mapinfo(5);
    nmaps=mapinfo(6);
    mapdata=textscan(fid,'%n %n %n %n\n', nmaps);
    tp1=mapdata{:,1};
    ids=mapdata{:,2};
    maxdac=65535;  % 2^16-1

    pscale=360.0/maxdac;
    % data format in saved binary file is:
    % tpwr[0] [a0p0] [a1p1] ... a[anpn]
    % tpwr[1] [a0p0] [a1p1] ... a[anpn]
    % ...
    
    if binary_tables
        fclose(fid);
        fid=fopen(tablesfile,'r','b');
        tbldata=fread(fid,[tsize,ntbls],'uint32'); % complexfloat
    else
        tbldata=zeros(tsize,ntbls); 
        if ~strcmp(tablesfile,'inline')
            fclose(fid);
            fid=fopen(tablesfile,'r');
        end
        for i=1:ntbls
            tbl=fscanf(fid,'%g %g\n',[2,tsize])';
            a=tbl(:,1).*maxdac;            
            ta=uint32(a);
            ta=bitshift(ta,16);
            ta=bitand(ta,hex2dec('ffff0000'));
            p=(tbl(:,2)+180.0)./pscale;
            tp=uint32(p);
            tp=bitand(tp,hex2dec('0000ffff'));
            s=ta+tp;
            tbldata(:,i)=uint32(s);
        end
    end
    if fid <0
        err=strcat('could not load tables data file: ',tablesfile);
        disp(err)
        return;
    end
    fclose(fid);
    tbldata=tbldata';

end