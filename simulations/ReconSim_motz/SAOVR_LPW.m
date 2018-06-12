function [new_x, new_y, new_i, xremus, yremus, ststamp] = SAOVR_LPW(m, remus_speed, counter_in, x_range, y_range, x_remus_in, y_remus_in, detect_radius, refresh_time, change_x, change_y)

%TMG 11/7/2015
%This function is called by ReconSim or its derivative
%It causes RECON sim to execute a manuever upon qualifying tag detections
%as allowed in CheckCoolDown.m and CheckTag.m
%It returns the new x and y coordinates and the new elapsed time after manuever to the ReconSim so
%that it can carry on with its new point

%for now this will be the manuever, later it will pick a maneuver.

%Set maneuver length in meters
%m_length = 900;
%num_m_legs = 3;
%m_leg_length = m_length/num_m_legs;

time_of_manuv =  674; %* 1.8; %m_length/remus_speed;
%need to change i to local variable, but pass it back as i

%If auv is moving north, change_x = 0, change_y = 1.8
%If auv is moving south, change_x = 0, change_y = -1.8;
%if auv is moving east, change_x = 1.8, change_y = 0;
%if auv is moving west, change_x = -1.8, change_y = 0, but it never goes wesst;
%Therefore we need to know only the non-zero condition
[theta, rad] = cart2pol(change_x, change_y);
theta = radtodeg(theta)

%base forward with stem 
 
% t = [theta, mod(theta -30, 360), mod(theta +90, 360), mod(theta+210,360), mod(180+theta, 360), theta]
% change_stop = [0, 75, 200, 325, 450, 525];

% Point forward with stem
% t = [theta, mod(theta -90, 360), mod(theta +30, 360), mod(theta+150,360), mod(270+theta, 360), mod(180+theta, 360), -theta]
% change_stop = [0, 50, 125, 275, 425, 500, 550];
 
%diamond
t = [mod(theta -60, 360), mod(theta +60, 360), mod(theta+120,360), mod(240+theta, 360), theta]
 change_stop = [0, 166, 342, 508, 674];

    
    

%zigzag

% if change_y == 1.8 %Going North
%    t = [135, 30, 135];
% elseif change_y == -1.8 %Going South
%    t = [315, 210, 315];
% elseif change_x == 1.8 %Going East
%    t = [45, 315, 45];
% end

%change_stop represents each timepoint that REMUS reaches and changes
%direction at, with change of angle t
%change_stop = [0, 170, 440];

%% Create manuever

t = t*pi/180; %convert to radians for sin,cos

dx = remus_speed * cos(t(1));
dy = remus_speed * sin(t(1));

p = 0:0.1:2*pi;
plot(x_remus_in,y_remus_in,'*');
%x_range = (detect_radius * cos(p))+ x_remus(1);
%y_range = (detect_radius * sin(p))+ y_remus(1);
radius_handle = plot(x_range, y_range, 'k');

ststamp = zeros(time_of_manuv,6);
savor_i = counter_in;
xremus(1) = x_remus_in;
yremus(1) = y_remus_in;
%Gives the hydrophone a new y & x position
for ii = 2: time_of_manuv;
    ststamp(ii,:) = clock; 
    for j = 2:length(change_stop)
        %if REMUS reaches a value in change_stop then it will change
        %direction based on angle set at t
        if  ii == change_stop(j)
            dx = remus_speed * cos(t(j));
            dy = remus_speed * sin(t(j));
        end
    end

    %Gives the hydrophone a new y & x position
    yremus(ii) = yremus(ii-1) + dy;
    xremus(ii) = xremus(ii-1) + dx;
    saovr_i(ii) = ii;
    delete(radius_handle)
    x_range = (detect_radius * cos(p))+ xremus(ii);
    y_range = (detect_radius * sin(p))+ yremus(ii);
    radius_handle = plot(x_range, y_range, 'k');
    %hold on
    %axis equal
       %plots new points on figure
    plot(xremus(ii),yremus(ii), '*');
    
%     for j = 1:numFish
%         %gives the fish a new y & x position (between -swim or swim)
%         allFishInfoZM(i,2,j) = allFishInfoZM(i-1,2,j) + swim_speed(j)*(rand()-0.48);
%         allFishInfoZM(i,1,j) = allFishInfoZM(i-1,1,j) + swim_speed(j)*(rand()-0.48) ;
%         plot(allFishInfoZM(i,1,j),allFishInfoZM(i,2,j),fishColor(j));
%     end
    
    if  mod(ii,refresh_time) == 0
        
        drawnow
    end
end
ststamp = datenum(ststamp);
new_x = xremus(ii);
new_y = yremus(ii);
new_i = ii+m;