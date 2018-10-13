function matlabPlots(argstr)

if nargin==0
   show_usage();
   return;
end
savejpgs=false;

pl={'-bs' '-ro' '-kd' '-g+' '-cs' '-mo' '-bd' '-y+'};

plots=[];
plots(1).files=[];
plots(1).legends=[];
plots(1).xaxis='';
plots(1).yaxis='';
plots(1).title='';
plots(1).xlim=[];
plots(1).ylim=[];
version='1.0';

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
    elseif (strfind(arg,'-f'))
        j=sscanf(arg,'-f%u');
        k=1;
        i=i+1;
        while(i<=length(args))
            f=args{i};
            v=strfind(f,'-');
            if (isempty(v))
                plots(j).files{k}=f;
                k=k+1;
                i=i+1;
            else
                i=i-1;
                break;
            end
        end
    elseif (strfind(arg,'-L'))
        j=sscanf(arg,'-L%u');
        k=1;
        i=i+1;
        while(i<=length(args))
            f=args{i};
            v=strfind(f,'-');
            if (isempty(v))
                name= strrep(f, '<sp>', ' ');
                plots(j).legends{k}=name;
                k=k+1;
                i=i+1;
            else
                i=i-1;
                break;
            end
        end
    elseif(strcmp(arg,'-j'))
        savejpgs=true;
    elseif (strfind(arg,'-l'))
        j=sscanf(arg,'-l%u');
        i=i+1;
        pl{j}=args{i};
    elseif(strfind(arg,'-t'))
        i=i+1;
        j=sscanf(arg,'-t%u');
        name= strrep(args{i}, '<sp>', ' ');
        if(isempty(j))
            j=length(plots);
        end
        plots(j).title=name;
    elseif(strfind(arg,'-xlim'))
        i=i+1;
        j=sscanf(arg,'-xlim%u');
        if(isempty(j))
            j=length(plots);
        end
        x1=str2double(args{i});
        i=i+1;
        x2=str2double(args{i});
        plots(j).xlim=[x1 x2];
    elseif(strfind(arg,'-x'))
        i=i+1;
        name = strrep(args{i}, '<sp>', ' ');
        j=sscanf(arg,'-x%u');
        if(isempty(j))
            j=length(plots);
        end
        plots(j).xaxis=name;
    elseif(strfind(arg,'-ylim'))
        i=i+1;
        j=sscanf(arg,'-ylim%u');
        if(isempty(j))
            j=length(plots);
        end
        y1=str2double(args{i});
        i=i+1;
        y2=str2double(args{i});
        plots(j).ylim=[y1 y2];
    elseif(strfind(arg,'-y'))
        i=i+1;
        name = strrep(args{i}, '<sp>', ' ');
        j=sscanf(arg,'-y%u');
        if(isempty(j))
            j=length(plots);
        end
        plots(j).yaxis=name;
    end
    i=i+1;
end

% ====================================================================
% usr plots - general purpose plot utility
%   draws an arbitrary number of user specified plots
%   syntax:   
%      -f{i} file1 file2 ..  list of files plotted in figure{i}
%      -x{i} label           x-axis label in figure{i}
%      -y{i} label           y-axis label in figure{i}
%      -t{i} label           plot title in figure{i}
%      -l{j} pattern         line/color pattern for file{j}
% ====================================================================
for i=1:length(plots)
    if(~isempty(plots(i).files))
        figure(i);
        hold on;
        if(~isempty(plots(i).title))
            title(plots(i).title);
        end
        n=length(plots(i).files);
        plothandle=[];
        for j=1:n
            data=load(plots(i).files{j});
            x=data(:,1);
            y=data(:,2);
            if j<=length(pl)
                l=pl{j};
            else
                l='-+';
            end 
            
            if ~isempty(plots(i).xlim)
                xlim(plots(i).xlim);
            else
                xlim('auto');
            end
            if ~isempty(plots(i).ylim)
                ylim(plots(i).ylim);
            else
                ylim('auto');
            end
            
            if j==1 && savejpgs
                plothandle=plot(x,y,l);
            else
                plot(x,y,l);
            end
            hold all
        end
        if(~isempty(plots(i).xaxis))
            xlabel(plots(i).xaxis);
        end
        if(~isempty(plots(i).yaxis))
            ylabel(plots(i).yaxis);
        end
        if(~isempty(plots(i).legends))
            legend(plots(i).legends);
        end

        hold off;
        if ~isempty(plothandle)
            name=strcat('figure',int2str(i));
            saveas(plothandle,name,'jpg');
        end
     end
end

function show_usage()
    usage=sprintf('Usage:matlabPlots(options)\n');
    usage=sprintf('%s\toptions:    \n',usage);
    usage=sprintf('%s\t -u         print usage and return\n',usage);
    usage=sprintf('%s\t -v         print version and return\n',usage);
    usage=sprintf('%s\t -f{i} file1 file2 ..   create plot{i} from file list\n',usage);
    usage=sprintf('%s\t -L{i} legnd1 legnd1 .. add legend strings\n',usage);
    usage=sprintf('%s\t -t{i} title            plot{i} title \n',usage);
    usage=sprintf('%s\t -x{i} label            plot{i} x-axis label \n',usage);
    usage=sprintf('%s\t -y{i} label            plot{i} y-axis label  \n',usage);
    usage=sprintf('%s\t -xlim{i} min max       plot{i} x-axis limits \n',usage);
    usage=sprintf('%s\t -ylim{i} min max       plot{i} y-axis limits \n',usage);
    usage=sprintf('%s\t -l{i} pattern          trace{j} line/color pattern \n',usage);
    usage=sprintf('%s\t -j                     save jpg files from figures\n',usage);
    disp(usage)
end

end

