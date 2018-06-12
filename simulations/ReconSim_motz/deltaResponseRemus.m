function [lastx, lasty] = deltaReponseRemus(dx, dy)
%long and lat are dx and dy from stugeonWalkRemusHydroZM

maxTime = 603; %just a test number at the moment - maxTime remus will run
%during response

%% setup figure
figure('name','Physical Environment','numbertitle','off');
%plot(bounds(:,1),bounds(:,2));
hold on;
grid on;

axis equal; %Axes set to equal for aestheic purposes.

%This sets the labels for the axes and title - these may not be relevant
xlabel('Easting (UTM)');
ylabel('Northing (UTM)');
title('Physical Environment');

%% Remus parameters during response (delta)
numHydro = 1; %one hydrophone on REMUS
r = 1.8; %movement of REMUS at 1.8 m / s

refresh_time = 31;  %redraw every refresh_time timesteps
radius = 500; %maximum radius, outside of which fish cannot be detected

x_remus = zeros(maxTime,1);
y_remus = zeros(maxTime,1);

%x & y coordinates of first position of the hydrophone
x_remus(1) = 585605;
y_remus(1) = 4628100;
plot(x_remus(1),y_remus(1),'g*'); %plotting first points on figure

%change_stop represents each timepoint that REMUS reaches and changes
%direction at, with change of angle t
change_stop = [0, 200, 400, 600];
t = [60, 180, 300, 60];

t = t*pi/180; %convert to radians for sin,cos

dx = r * cos(t(1));
dy = r * sin(t(1));

p = 0:0.1:2*pi;
xrange = (radius * cos(p))+ x_remus(1);
yrange = (radius * sin(p))+ y_remus(1);
radius_handle = plot(xrange, yrange, 'k');

%% main loop - step the remus and each of the fish

%Nested loop cycling through every movement of hydrophone
for i = 2:maxTime;
    for j = 2:length(change_stop)
        %if REMUS reaches a value in change_stop then it will change
        %direction based on angle set at t
        if  i == change_stop(j)
            dx = r * cos(t(j));
            dy = r * sin(t(j));
        end
    end

    %Gives the hydrophone a new y & x position
    y_remus(i) = y_remus(i-1) + dy;
    x_remus(i) = x_remus(i-1) + dx;
    delete(radius_handle)
    xrange = (radius * cos(p))+ x_remus(i);
    yrange = (radius * sin(p))+ y_remus(i);
    radius_handle = plot(xrange, yrange, 'k');
    %hold on
    %axis equal
       %plots new points on figure
    plot(x_remus(i),y_remus(i), 'g*');
    
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
lastx = x_remus(i)
lasty = y_remus(i)
