function args=getArgs(s)
    remain = s;
    i=1;
    args={};
    while true
        [str, remain] = strtok(remain);
        if isempty(str),
            break;
        end
        args = [args;str];
        i=i+1;
    end
end
