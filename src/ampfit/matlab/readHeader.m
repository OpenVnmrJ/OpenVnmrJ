function [fileHeader,fileHeaderSize] = readHeader(fileName)
% READHEADER        Reads header from .fid file format files.
%                   For more information on elements, see VNMR 
%                   User Programming Guide, Pub # 01-999165-00,
%                   Rev. B0802.  
%   USAGE:          [fileHeader,fileHeaderSize] = readHeader(fileName)
%                   where fileHeader is a structure containing
%                   sub-elements such as nblocks,ntraces,etc per
%                   page 285 of the Guide, and fileName is the
%                   name of the fid file (should be placed in single
%                   quotes). fileHeaderSize is returned as 32/ebytes so
%                   that fseek can be used during data read.

%   REV. HISTORY:
%   DATE            DESCRIPTION                     AUTHOR
%   -------------------------------------------------------
%   12 MAR 03       Created File                    Sarah Vargas
%   

% ERROR CHECKING ON INPUTS

if ~ischar(fileName)
    error('File Name Must be in character format.  See help readHeader')
end

[fileID,errMsg] = fopen(fileName,'r','ieee-be');
if ~isempty(errMsg)
    error(errMsg)
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   Begin header read
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

fileHeader.nblocks = fread(fileID,1,'int32');
fileHeader.ntraces = fread(fileID,1,'int32');
fileHeader.np = fread(fileID,1,'int32');
fileHeader.ebytes = fread(fileID,1,'int32');
fileHeader.tbytes = fread(fileID,1,'int32');
fileHeader.bbytes = fread(fileID,1,'int32');
fileHeader.vers_id = fread(fileID,1,'int16');
fileHeader.status = fread(fileID,1,'int16');
fileHeader.nbheaders = fread(fileID,1,'int32');

% Generate status bits as sub-structure elements

fileHeader.statusBits.S_DATA = bitget(fileHeader.status,1);
fileHeader.statusBits.S_SPEC = bitget(fileHeader.status,2);
fileHeader.statusBits.S_32 = bitget(fileHeader.status,3);
fileHeader.statusBits.S_FLOAT = bitget(fileHeader.status,4);
fileHeader.statusBits.S_COMPLEX = bitget(fileHeader.status,5);
fileHeader.statusBits.S_HYPERCOMPLEX = bitget(fileHeader.status,6);
fileHeader.statusBits.S_ACQPAR = bitget(fileHeader.status,8);
fileHeader.statusBits.S_SECND = bitget(fileHeader.status,9);
fileHeader.statusBits.S_TRANSF = bitget(fileHeader.status,10);
fileHeader.statusBits.S_NP = bitget(fileHeader.status,12);
fileHeader.statusBits.S_NF = bitget(fileHeader.status,13);
fileHeader.statusBits.S_NI = bitget(fileHeader.status,14);
fileHeader.statusBits.S_NI2 = bitget(fileHeader.status,15);

% Calculate header size

fileHeaderSize = 32;
fclose(fileID);