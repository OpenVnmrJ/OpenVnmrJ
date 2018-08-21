function pwr=tpower(tpwr,tpwrf)
c1=log(10.0)/20.0;  % constant = 2.303/20.0
max_tpwrf=4095.0;
max_tpwr=63;
if(tpwr>max_tpwr)
    tpwr=max_tpwr;
end
if(tpwrf>max_tpwrf)
    tpwrf=max_tpwrf;
end
pwr=tpwrf*exp(c1*tpwr)/max_tpwrf;
end
