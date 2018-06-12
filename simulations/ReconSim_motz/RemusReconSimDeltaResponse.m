clear all
close all
fishpath = input('Enter the save to path in single quotes with final backslash: ');

for iter = 1:30
clearvars -except iter fishpath
close all
%recon = 1; % = menu('Run with ', 'Override', 'No override');
global_cooldown = 60*60;
local_cooldown = 60*60;
SAOVR_start = 0;

%% Remus parameters
numHydro = 1; %one hydrophone on REMUS
r = 1.8; %movement of REMUS at 1.8 m / s

refresh_time = 31;  %redraw every refresh_time timesteps
radius = 300; %maximum radius, outside of which fish cannot be detected
disp('Radius & Speed')
disp(radius)
disp(r)
%change_stop represents each timepoint that REMUS reaches and changes
%direction at, with change of angle t
leg_length = 600;
join_length = 400;
num_legs = 1;

%change_stop = [0, 601, 802, 1403, 1604, 2205, 2406, 3007, 3208, 3809];
change_stop = zeros(num_legs*2,1);
for n = 1:(2*num_legs) - 1;
    if mod(n,2)
        %disp('odd')
        change_stop(n+1)  = change_stop(n) + leg_length;
    else 
        %disp('even')
        change_stop(n+1)  = change_stop(n) + join_length;
    end
end
change_stop; %This is the cumulative distance
change_stop_time = change_stop / r; %This is the cumulative time at each stop
maxTime = ceil(max(change_stop_time)); %just a test number at the moment, to be amended

%t = [90, 0, 270, 0, 90, 0, 270, 0, 90, 0];
t = zeros(num_legs*2,1);
t(1) = 90;
for n = 2:(2*num_legs) - 1;
    
    if mod(n,2)
        %disp('odd')
        t(n)  = mod(361,t(n-2)+91);    
    else 
        %disp('even')
        t(n)  = 0; %if odd, then the next (even) stop is angle 0
    end
end
ind = find(t == 180);
t(ind) = 270;

t = t*pi/180; %convert to radians for sin,cos

dx = r * cos(t(1));
dy = r * sin(t(1));

p = 0:0.1:2*pi;

%% set initial position
x_remus = zeros(maxTime,1);
y_remus = zeros(maxTime,1);
tstamp = zeros(maxTime,6);
%x & y coordinates of first position of the hydrophone
x_remus(1) = 0; %585600;
y_remus(1) = 0; %4628100;

xrange = (radius * cos(p))+ x_remus(1);
yrange = (radius * sin(p))+ y_remus(1);

%% setup figure
%define the domain based on the number of legs, provide a 50 m buffer
domain_end_x = (join_length * (num_legs -1)) + radius;
domain_end_y = leg_length + radius

bounds = [-(radius), -(radius); domain_end_x, domain_end_y];

fig = figure('name','Domain','numbertitle','off');
plot(bounds(:,1),bounds(:,2),'.');
hold on;
grid on;

axis equal; %Axes set to equal for aestheic purposes.

%This sets the labels for the axes and title - these may not be relevant
xlabel('Meters');
ylabel('Meters');
title('Model Domain');

plot(x_remus(1),y_remus(1),'g*'); %plotting first points on figure
hold on
radius_handle = plot(xrange, yrange, 'k');

%% Fish paramters
tag_burst_rate = 5; 

numFish = 1; %Number of fish to model; 
%Create an x,y matrix of random fish positions

%for f = 1%:numFish
    FishID = iter;
    FishX = randi([bounds(1,1), bounds(2,1)],1,1);
    FishY = randi([bounds(1,2), bounds(2,2)],1,1);
    %range_record_all = [];
    %tag_start = rand(1);
    %tag_signal = (tag_start:tag_burst_rate:maxTime);
    %tag_signal = tag_signal';
    Fish = struct('fish',FishID, 'fish_east',FishX, 'fish_north',FishY,'burst_rate',tag_burst_rate,'detect_r', radius,'Record', []);
    pv = strcat(['save ',fishpath,'Fish_',num2str(iter),'.mat Fish']) 
    eval(pv)

%end
%clear Fish

%for f = 1:numFish
    %eval(['load E:\ReconSim\Fish_', num2str(iter)])
    plot(Fish.fish_east,Fish.fish_north,'or')
%end


%clear Fish*

%% main loop - step the remus

%Nested loop cycling through every movement of hydrophone
figure(1)
counter = 1;
for i = 2:maxTime;
    tstamp(i,:) = clock;
    for j = 2:length(change_stop)
        %if REMUS reaches a value in change_stop then it will change
        %direction based on angle set at t
        if  i >= change_stop_time(j)
            dx = r * cos(t(j));
            dy = r * sin(t(j));
   
        end
    end

    %Gives the hydrophone a new y & x position
    y_remus(i) = y_remus(i-1) + dy;
    x_remus(i) = x_remus(i-1) + dx;
    counter(i) = i;
    delete(radius_handle)
    
    xrange = (radius * cos(p))+ x_remus(i);
    yrange = (radius * sin(p))+ y_remus(i);
    radius_handle = plot(xrange, yrange, 'k');
    
       %plots new points on figure
    plot(x_remus(i),y_remus(i), 'g*');
    
    tagid = CheckTag(x_remus(i), y_remus(i), radius, iter,fishpath);
    
    if tagid > 0; % A default tag id of zero is returned by CheckTag if no tag is heard
        %[gorecon,expires] = CheckCoolDown(SAOVR_start, tagid, global_cooldown, local_cooldown, i)
        if SAOVR_start == 0;
            SAOVR_start = i; %Set new criteria so it won't execute twice 
            gorecon = 1;
        else
            gorecon = 0;
        end
        
        %SAOVR_start = expires
        if gorecon == 1;
           
            [x_remus(i), y_remus(i), new_i, xremus, yremus, ststamp] = SAOVR_LPW(i, r, counter(i), xrange, yrange, x_remus(i), y_remus(i), radius, refresh_time, dx, dy);
            %SAOVR has its own runtime clock starting at 1 and going to manuver time and needs to be rectified
            
        end  
    end
        
    %legend(num2str(i))
    
%     for j = 1:numFish
%         %gives the fish a new y & x position (between -swim or swim)
%         allFishInfoZM(i,2,j) = allFishInfoZM(i-1,2,j) + swim_speed(j)*(rand()-0.48);
%         allFishInfoZM(i,1,j) = allFishInfoZM(i-1,1,j) + swim_speed(j)*(rand()-0.48) ;
%         plot(allFishInfoZM(i,1,j),allFishInfoZM(i,2,j),fishColor(j));
%     end
    
    if  mod(i,refresh_time) == 0
        
        drawnow
    end
end


disp('End of REMUS Run')

%% Combine and sort regular and override time records
tstamp = datenum(tstamp);
ststamp = datenum(ststamp);
combined_record_all = [x_remus, y_remus, tstamp; xremus', yremus', ststamp];

combined_record_all = sortrows(combined_record_all,3);

x_remus_all = combined_record_all(:,1);
y_remus_all = combined_record_all(:,2);
mission_time = combined_record_all(:,3);

%% check fish-remus interactions
% Need to interpolate remus position at tag time
   
for f = 1:numFish
    range_record_all = [];
    pv = strcat(['load ',fishpath,'Fish_', num2str(iter)])
    eval(pv)
    disp(Fish)
 
    
    for i = 1:maxTime
        
        Dx = x_remus_all(i) - Fish.fish_east;
        Dy = y_remus_all(i) - Fish.fish_north;
        
        displacement = sqrt(Dx.*Dx + Dy.*Dy);

        %Check to see if fish in range of hydrophone
        if(displacement <= radius)
            %display(['In range of fish ', num2str(j), ' at time ', num2str(i)]);  
            range_record = [i displacement];
            range_record_all = [range_record_all; range_record];
                 
        end %end check loop
     %range_record_all
    end %end time loop
    range_record_all(1,:)=[]; %First row is always 2,399 and change, not sure why
    Fish.Record = range_record_all;
    pv = strcat(['save ',fishpath,'Fish_',num2str(iter),'.mat Fish'])
    eval(pv)
     
 end %end fish loop
        
clear Fish* 

end
 
