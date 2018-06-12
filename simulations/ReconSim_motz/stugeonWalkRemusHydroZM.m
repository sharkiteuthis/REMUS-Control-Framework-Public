clear all
close all

maxTime = 3810; %just a test number at the moment, to be amended

%% setup figure
fig = figure('name','Physical Environment','numbertitle','off');
%plot(bounds(:,1),bounds(:,2));
hold on;
grid on;

axis equal; %Axes set to equal for aestheic purposes.

%This sets the labels for the axes and title - these may not be relevant
xlabel('Easting (UTM)');
ylabel('Northing (UTM)');
title('Physical Environment');

%% Fish paramters
% numFish = 3; %Number of fish to model
% fishColor = ['r','b','m'];
% assert(numFish == length(fishColor));
% 
% %Set base swim speed again for each individual - currently these must be
% %integer values
% swim_speed = [2.4,2.4,2.4]; 
% assert(numFish == length(fishColor));
% 
% fish_offset = [-100, 0, 100];
% assert(numFish == length(fish_offset));
% 
% allFishInfoZM = zeros(maxTime,2,numFish); %Creation of 3D Matrix
% 
% % plot the first position of the first fish.
% for j = 1:numFish
%     allFishInfoZM(1,1,j) = 585800;
%     allFishInfoZM(1,2,j) = 4628300 + fish_offset(j);
%     plot(allFishInfoZM(1,1,j),allFishInfoZM(1,2,j),[fishColor(j),'*']);
% end

%% Remus parameters
numHydro = 1; %one hydrophone on REMUS
r = 1.8; %movement of REMUS at 1.8 m / s

refresh_time = 31;  %redraw every refresh_time timesteps
radius = 200; %maximum radius, outside of which fish cannot be detected

x_remus = zeros(maxTime,1);
y_remus = zeros(maxTime,1);

%x & y coordinates of first position of the hydrophone
x_remus(1) = 585600;
y_remus(1) = 4628100;
plot(x_remus(1),y_remus(1),'g*'); %plotting first points on figure

%change_stop represents each timepoint that REMUS reaches and changes
%direction at, with change of angle t
change_stop = [0, 601, 802, 1403, 1604, 2205, 2406, 3007, 3208, 3809];
t = [90, 0, 270, 0, 90, 0, 270, 0, 90, 0];

t = t*pi/180; %convert to radians for sin,cos

dx = r * cos(t(1));
dy = r * sin(t(1));

p = 0:0.1:2*pi;
xrange = (radius * cos(p))+ x_remus(1);
yrange = (radius * sin(p))+ y_remus(1);
radius_handle = plot(xrange, yrange, 'k');

%% main loop - step the remus

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

%% run the fish
sturgeonWalkFishZM

%% check fish-remus interactions
%load ReconFindFishZM_med20-7-2015--9-53

%this loads a file of 3 columns, x, y, fish id

for i = 1:maxTime
    for j = 1:numFish
        Dx = x_remus(i) - allFishInfoZM();
        Dy = y_remus(i) - allFishInfoZM();

        displacement = sqrt(Dx*Dx + Dy*Dy);

        %Check to see if fish in range of hydrophone
        if(displacement <= radius)
            display(['Tracked fish ', num2str(j), ' at time ', num2str(i), '\n']);
            
        end
    end
end

 %run the response
 %fish = menu('Choose a fish ','One','Two','Three','Four','Five','Six')
 %fish_file  = ReconFindFishZM_med20-7-2015--9-53(find(ReconFindFishZM_med20-7-2015--9-53(:,3) == numfish),:);
 %[newstartx newstarty] = deltaResponseRemus(dx, dy);
