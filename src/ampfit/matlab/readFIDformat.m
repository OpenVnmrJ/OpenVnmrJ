function [fidData,fileHeader] = readFIDformat(fileName,nskip,dcCorrect,ntotal)
%   READFIDFORMAT   Reads files in *.fid data format.
%   USAGE:  [fidData,fileHeader] = readFIDformat(fileName,nskip,dcCorrect,ntotal)
%       fileName is the name of the fid file, typically 'fid'. Right now, there is
%       no support for directories supplied on the input so you have to be
%       in the directory where the 'fid' file is located.  I have directory
%       support in another menu driven format if you need it.
%       
%       nskip is number of skipped TRACES.  I added support for reading
%       partial FID blocks into this file because I was having some memory
%       allocation problems for very large FID files.  You can remove this
%       support if you don't need it.  This feature has been tested and appears to function. 
%       
%       dcCorrect is a binary flag to turn on or off DC Correction based on "lvl" and
%       "tilt" in the header.  My understanding is that the lvl and tilt
%       may represent some sort of average DC offset and our
%       reconstructions may use a more complex method for estimating the DC
%       offset.
%
%       ntotal is the total number of TRACES to be read.  Again, this can be
%       used to support reading partial blocks of data in for memory
%       support in matlab.
%
%       REVISION HISTORY:
%       AUTHOR          DATE            DESCRIPTION
%       S. Vargas       3/12/03         Created file
%       S. Vargas       4/2/03          Added support for partial FID files
%       S. Vargas       1/27/04         Added additional comments

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   ERROR CHECKING and HEADER READ
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if ~ischar(fileName)
    error('File Name Must be in character format.  See help readHeader')
end

% Get Header
[fileHeader,fileHeaderSize] = readHeader(fileName);

if (nargin == 2)
	ntotal = fileHeader.nblocks * fileHeader.ntraces - nskip;  
    disp(['Skipping ' num2str(nskip) '   Total FIDS = ' num2str(ntotal-nskip)]);
    dcCorrect = 1;
elseif (nargin == 3)
    ntotal = fileHeader.nblocks * fileHeader.ntraces - nskip;  
    disp(['Skipping ' num2str(nskip) '   Total FIDS = ' num2str(ntotal-nskip)]);
elseif (nargin == 4)
    if (fileHeader.nblocks*fileHeader.ntraces < ntotal)
        error('NTOTAL value exceeds number of traces in file')
    end
end

fileID = fopen(fileName,'r','ieee-be');

% SEEK PAST FILE HEADER
seekStatus = fseek(fileID,fileHeaderSize,'bof');

if (seekStatus == -1)
    error('Problem operating on file')
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% CALCULATE SKIPPED BYTES INCLUDING BLOCK HEADERS
%  AND SEEK PAST THESE BYTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% we skip first block header even if only one trace is skipped.
nBlocksSkipped = ceil(nskip/fileHeader.ntraces);
floorNumSkipped = floor(nskip/fileHeader.ntraces);

if (floorNumSkipped ~= nBlocksSkipped)
    disp('Skipped only part of Block')
    nTracesPartialBlock = nskip - floorNumSkipped*fileHeader.ntraces; 
    partialBlockFlag = 1;
else
    partialBlockFlag = 0;
end

blockHeaderSize = 28;
nSkippedBytes = nBlocksSkipped*blockHeaderSize + fileHeader.np*fileHeader.ebytes*nskip;
seekStatus = fseek(fileID,nSkippedBytes,'cof');

if (seekStatus == -1)
    error('Problem operating on file')
end

%%%%%%%%%%%%%%%%%%%%%%%%%%
%   BEGIN READING DATA
%%%%%%%%%%%%%%%%%%%%%%%%%%

if (fileHeader.ebytes == 4)
    if (fileHeader.statusBits.S_FLOAT == 1)
        precision = 'float32';
    else
        precision = 'int32';
    end
elseif (fileHeader.ebytes == 2)
    precision = 'int16';
end

nBlocksRead = ceil(ntotal/fileHeader.ntraces);    % Number of blocks to be read 

% COMPLETE PARTIAL BLOCK
if (partialBlockFlag == 1)
    fidDataVec = fread(fileID,nTracesPartialBlock*fileHeader.np,precision);
    nBlocksRead = nBlocksRead - 1;
end

% read block header and data--for now, write over block header data
%  at each pass.  We should be on a block boundary at the start of this 
%   loop!!
fidDataVec = zeros(fileHeader.ntraces*fileHeader.np,nBlocksRead);
for nBlockCount = 1:nBlocksRead
    blockHead = readBlockHeader(fileID,fileHeader);
    blktilt(nBlockCount) = blockHead.tlt;
    blklevel(nBlockCount) = blockHead.lvl;
    fidDataVec(:,nBlockCount) = fread(fileID,fileHeader.ntraces*fileHeader.np,precision);
end
fidDataVec = fidDataVec(:);

%%%%%%%%%%%%%%%%%%%%%%%%%
%   FORMAT FID DATA
%%%%%%%%%%%%%%%%%%%%%%%%%

fidData = fidDataVec(1:2:end) + j*fidDataVec(2:2:end);
if (dcCorrect)
    disp(['DC Correction ON, lvl = ' num2str(blockHead.lvl) '   Tilt = ' num2str(blockHead.tlt)]);
    fidData = fidData - (blockHead.lvl+j*blockHead.tlt);
else
    disp('DC Correction OFF');
end
fidData = reshape(fidData,fileHeader.np/2,nBlocksRead*fileHeader.ntraces);
fclose(fileID);


