function[sal,temp,tide,nut,other] = get_enviro(y,i,f0,f1,f2,f3,f4);
if i~=1;
    i=i-1;
end
%This function is called by Markov_walk_sturgeon.m
%It loads 5 environmental variable vectors created and stored by stur_env.m
%After the calling function looks up the river latitude y at which a
%particel resides in the current time step i, it finds the corresponding
%values of the variables, at time i but modifies them by rivr step to
%simulate the tidal wave progress up the river.

%river sections are min northing -max northing
%Define Section boundary for tidal progression
%section1_south_boundary=4628999+3000; %North to south
%section3_north_boundary=4628999-4000;

sect1 = 4628999;
sect2 = 4628000;

lag = 0;
if y(i) >= sect1;
    lag = 0;
elseif y(i) <= sect1;
    lag =0;
else
    disp('no lag')
end
    
lagged_time=i+lag;

sal = f1(lagged_time);
temp = f2(lagged_time);
tide = f0(lagged_time);
nut = f4(lagged_time);
other = f3(lagged_time);

    
%track these by graphing with hold on

%plot(i,sal,'r.',i,temp,'k.',i,tide,'b.',i,nut,'g.',i,other,'c.')
%hold on