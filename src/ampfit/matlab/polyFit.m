function cv=polyFit(datafile,cpoints,order)

dflt_cpoints=64;
dflt_order=10;

if(nargin == 0 || nargin>3)
    usage=sprintf('Usage:\polyFit(datafile[,cpoints,order])\n');
    usage=sprintf('%s\tdatafile: exp data file power, ampl, [p]\n',usage);
    usage=sprintf('%s\tcpoints:  number of points in tables [%d]\n',usage,dflt_cpoints);
    usage=sprintf('%s\torder:    poynomial order [%d]\n',usage,dflt_order);
    disp(usage)
    return;
end

if nargin ==1
    cpoints=dflt_cpoints;
    order=dflt_order;
elseif nargin ==2
    order=dflt_order;
end

expdata=load(datafile);
expdata=sortrows(expdata);

if isempty(expdata)
    err=strcat('could not load data file: ',datafile);
    disp(err)
    return;
end 

[n,m] = size(expdata);

x=expdata(:,1);
y=expdata(:,2); 
p=[];
if m>2
   p=expdata(:,3);
end

mxpwr=x(n);

cx=0:mxpwr/(cpoints-1):mxpwr; 
cx=cx';

n=order;

V(:,1) = ones(length(x),1,class(x));
V(:,1) = x.*V(:,1); % constrains intercept to 0
for j = 2:1:n
   V(:,j) = x.*V(:,j-1);
end
[Q,R] = qr(V,0);
ws = warning('off','all'); 
f = R\(Q'*y);    % Same as p = V\y;

% build output vector
g=ones(size(cx));
t=zeros(size(cx));
for j = 0:order-1
    g=g.*cx;
    if j==0
        t=g;
    else
    t=[t g];
    end
end

cy=t*f; % cy fit of transformed Xi

%f=polyfit(x,y,order);
%cy=polyval(f,cx);
if m>2  
    %f=polyfit(x,p,order);
    %cp=polyval(f,cx);
    f = R\(Q'*p);    % Same as p = V\y;
    cp=t*f; % cy fit of transformed Xi
end

% force end point contraints

cv=zeros(cpoints,m);
cv(:,1)=cx;
cv(:,2)=cy;
if ~isempty(p)
    cv(:,3)=cp;
end

end