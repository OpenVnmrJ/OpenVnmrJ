function [value,error] = Vread_procpar(procpar_path,procpar_name,search_string)
%   READ_PROCPAR   searches for values in the procpar file
%   USAGE:  [value,error] = read_procpar(procpar_path,procpar_name,search_string)
%
%       procpar_path is the path / directory in which the file is located
%
%       procapr_name is the name of the procpar file, typically 'procpar'.
%
%       search_string is the string for which teh value should be extracted
%
%       value  - value of the found parameter
%
%       errror -  0: no error  , 1: errro during operation/unsuccessful

%
%       REVISION HISTORY:
%       AUTHOR          DATE            DESCRIPTION
%       M. Meiler       3/25/04         Initial creation
%
value = '';
error = 0;
value_location=12;
strng = '2';
flag = '4';
fileID = fopen(strcat(procpar_path,procpar_name),'r');
%Error opening file
if fileID < 0
   fprintf('ERROR: - File not found!\n');
   error=1;
   return;
end   
%search_string = 'sw';
while (~feof(fileID))
   text = fscanf(fileID, '%s',1);
      if strmatch(text,search_string,'exact')
         type= fscanf(fileID, '%s',1);
         for i =1:value_location -1
            text= fscanf(fileID, '%s',1);
         end
         fclose(fileID);
         if (type ~=strng(1)) && (type~=flag(1)) %check if string or flag
            value = str2double(text);
         else
             value = text; %don't convert if string or flag
         end
         return;
      end   
end   
fprintf('\nSorry the value could not be located! \n\n');
error=1;
fclose(fileID);
return;