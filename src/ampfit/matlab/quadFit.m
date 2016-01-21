function cv=quadFit(datafile,cpoints,spans,order)

dflt_cpoints=128;
dflt_spans=8;
dflt_order=3;

if(nargin == 0 || nargin>4)
    usage=sprintf('Usage:\quadFit(datafile[,cpoints,spans,order])\n');
    usage=sprintf('%s\tdatafile: exp data file power, ampl, [p]\n',usage);
    usage=sprintf('%s\tcpoints:  number of points in tables [%d]\n',usage,dflt_cpoints);
    usage=sprintf('%s\tspans:    number of spans [%d]\n',usage,dflt_spans);
    usage=sprintf('%s\torder:    poynomial order in spans [%d]\n',usage,dflt_order);
    disp(usage)
    return;
end

if nargin ==1
    cpoints=dflt_cpoints;
    spans=dflt_spans;
    order=dflt_order;
elseif nargin ==2
    spans=dflt_spans;
    order=dflt_order;
elseif nargin ==3
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
%mnpwr=x(1);

cx=0:mxpwr/(cpoints-1):mxpwr; 
cx=cx';

cy=cx;

S=0:mxpwr/(spans-1):mxpwr;

[~,q1] = histc(S,x);  % x index at start of span
[~,q2] = histc(S,cx); % cx index at start of span

%q2'
lastx=0;
lasty=0;
if ~isempty(p)
    lastp=0;
end

for i=2:spans
    index=q1(i-1);
    if(index<1)
        index=1;
    end
    
    % transform x and y values in new span by subtracting off 
    % extrapolated end points of previous span
    
    xp=x(index:q1(i))-lastx;
    yp=y(index:q1(i))-lasty;
    
    % bias first scan to go through 0
    
    if ~isempty(p)
        pp=p(index:q1(i))-lastp;
    end

    % fit the transformed data segment in the span to a low order
    % polynomial constrained to have an intercept of zero
    g=ones(size(xp));
    t=zeros(size(xp));
    %t=g;

    for j = 0:order
        g=g.*xp;
        if j==0
            t=g;
        else
            t=[t g];  % t1*x + t2*x^2 + ...
        end
    end
    ty=t\yp; % ty,tp contain polynomial coefficients for offet x,y
    if ~isempty(p)
      tp=t\pp;
    end
    
    % cx & cy are equally spaced points gereated from fit
    % use low order polynomial to fit cy values in span
    
    xv=cx(q2(i-1):q2(i));
    xv=xv-lastx; % transform cx to offet space
    
    g=ones(size(xv));
    t=zeros(size(xv));
    for j = 0:order
        g=g.*xv;
        if j==0
            t=g;
        else
        t=[t g];
        end
    end

    f=t*ty; % cy fit of transformed Xi
    % generate fit values for Yi in span after conversion from poly space
    % to power space
    
    fit=f+lasty;
    cy(q2(i-1):q2(i))=fit;
    
    % set start values for next span
    
    lastx=cx(q2(i));
    lasty=cy(q2(i));
    
    if ~isempty(p)
        f=t*tp; % phase fit of transformed Xi
        fit=f+lastp;
        cp(q2(i-1):q2(i))=fit;
        lastp=cp(q2(i));
    end
   
end
% force end point contraints

cv=zeros(cpoints,m);
cv(:,1)=cx;
cv(:,2)=cy;
if ~isempty(p)
    cv(:,3)=cp;
end

end