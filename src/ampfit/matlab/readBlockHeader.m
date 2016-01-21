function blockHeader = readBlockHeader(fileID,header)
% READBLOCKHEADER   Reads block header from .fid file format files.
%                   For more information on elements, see VNMR 
%                   User Programming Guide, Pub # 01-999165-00,
%                   Rev. B0802.  
%   USAGE:          blockHeader = readBlockHeader(fileID)
%                   where blockHeader is a structure containing
%                   sub-elements such as scale,status,etc per
%                   page 286 of the Guide, fileID is the
%                   name of the fid file opened with FOPEN, and
%                   header is the file header structure returned
%                   by readHeader.

%   REV. HISTORY:
%   DATE            DESCRIPTION                     AUTHOR
%   -------------------------------------------------------
%   12 MAR 03       Created File                    Sarah Vargas
%   

blockHeader.scale = fread(fileID,1,'int16');
blockHeader.status = fread(fileID,1,'int16');
blockHeader.index = fread(fileID,1,'int16');
blockHeader.mode = fread(fileID,1,'int16');
blockHeader.ctcount = fread(fileID,1,'int32');
blockHeader.lpval = fread(fileID,1,'float32');
blockHeader.rpval = fread(fileID,1,'float32');
blockHeader.lvl = fread(fileID,1,'float32');
blockHeader.tlt = fread(fileID,1,'float32');

if ((header.nbheaders == 2) & (header.statusBits.S_HYPERCOMPEX==1))
    blockheader.s_spare1 = fread(fileID,1,'int16');
    blockheader.status = fread(fileID,1,'int16');
    blockheader.s_spare2 = fread(fileID,1,'int16');
    blockheader.s_spare3 = fread(fileID,1,'int16');
    blockheader.l_spare1 = fread(fileID,1,'int32');
    blockheader.lpval1 = fread(fileID,1,'float32');
    blockheader.rpval1 = fread(fileID,1,'float32');
    blockheader.f_spare1 = fread(fileID,1,'float32');
    blockheader.f_spare2 = fread(fileID,1,'float32');
end
