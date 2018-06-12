function newSturgeonWalk

%Markov_walk_sturgeon
%TMG 1/12/2010
%This calculates a random walk for sturgeon in the upper Hudson River
%in which swim speed is constrained by river conditions.
%Favorable conditions in a stretch decrease swim speed to make it more "sticky"
%Conditions generated independently and are passed to this script as called
%by function get_enviro on (or around) line 224!!

% Edited by Dana Christensen November 2012
% Edited by Walker Davis March 2013 
%     -Added variable speed for simulated sturgeon walk
%     -More accurately models sturgeon's tendency to remain in favorable
%     environments
% Edited by Zoe Mayo June 2015
%     -Not for  Hudson River anymore so boundaries of where fish can go
%     have been removed
%     -Environmental conditions removed - not needed, no longer use
%     function get_enviro
%     -Certain parts of the code have been hard-coded for testing 

% Statements below ensure no previous variables and used which may be left
% in the workspace.
close all
clear all

%% Environment Definition
%This section sets up values for the various variables that define the environment
%the model needs to run properly. The number of fish whose swim to model, the length of time
%to run the program for, the boundaries of the physical environment being 
%modeled (fish are not limited by this boundary, i.e. they cam go outside the boundary),
%and, finally, the number of hydrophones (recording deviced to model as well as
%their placement within the boundaries mentioned above.

%Number of fish to model (usually an input but has been hardcoded to test)
numFish = 2;
%numFish = input ('Enter number of fish to model:\n');

%Length of time to run (usually an input but has been hardcoded to test)
maxTime = 600;
%maxTime = input ('Enter the length of time (in minutes) for which the model should run (600 = 10h):\n');

%% Boundary Definition
%This sub-section sets points that define the boundary
bounds = [586999 4628999;
        586999 4628000;
        586000 4628000;
        586000 4628999;
        586999 4628999];

% Categorization of max/min bounds
%min(arr), max(arr) are functions that treat the input argument as a list
%and returns the minimum value of the list ... or the maximum, depending on
%which you use.
xMin = min(bounds(:,1));
xMax = max(bounds(:,1));
yMin = min(bounds(:,2));
yMax = max(bounds(:,2));

%%% Hydrophone Information - THIS WILL BE CHANGED - ZOE 
%This prompts the user for the maximum radius, outside of which fish cannot
%be detected (usually an input but has been hardcoded to test)
radius = 500;
%radius = input ('Enter maximum distance from the REMUS\nthat fish are recorded.\n[1,30]:\n');

%% Creation of Data Structure
%This model simulates a number of fish (numFish) swimming in a 2-D space as
%a function of time. In order to catalog the results of the model timestep
%by timestep, it is necessary to create a data structure that holds (in one
%dimension) time, (in another dimension) a location, and (in the last dimension)
%the number of fish in the model.
%Therefore, below is a maxTime x 2( for an x-y point) x numFish matrix (3D Vector or 3D Array)
%
%Even though the desired result in an output file with the pertinent
%information, it is time consuming for the computer to print to a file
%after each fish. So, a second (sort of dummy) 2D matrix will hold the
%information for each fish. After fish 'n' is done with its swim, its 2D
%matrix will be placed into the 3D matrix, sort of as a slice of the 3D
%matrix.
%
%At the end, the entire file can be iterated through and saved.
%Potential improvement: save user input with timestamp in future

%Creation of 2D Matrix
oneFishInfo = zeros(maxTime,2);
%Creation of 3D Matrix
allFishInfo = zeros(maxTime,2,numFish);

%% Plot of Environment
%This section of code creates a plot of the environment (the grid being analyzed)
%and labels it in red on a simple cartesian plot. The first fish's initial 
%location is then plotted. The hydrophones is then placed onto that grid.

%Establishing separate figures becomes important when attempting to plot
%different sorts of information on separate plots.
fig = figure('name','Physical Environment','numbertitle','off');
plot(bounds(:,1),bounds(:,2),'r');
hold on;
grid on;

%Axes set to equal for aestheic purposes.
axis equal;

%This sets the labels for the axes and title.
xlabel('Easting (UTM)');
ylabel('Northing (UTM)');
title('Physical Environment');

%These three lines plot the first position of the first fish.
x(1) = randi([xMin xMax],1);
y(1) = randi([yMin yMax],1);
plot(x(1),y(1),'g*');

%%% Plot Hydrophones
%This sub-section cycles through the 3D matrix hydroPoints and plots the
%hydrophones as well as a filled-in area around them, representing their
%radius of detection.

%This plots the hydrophones as above specified.
numHydrophones = 1;
%hydroPoints are the position (x & y) of hydrophone in grid
hydroPoints = [586000 4628400];
for n = 1:numHydrophones
    %Make sure the plotting is done to the same figure as above.
    figure(fig);
    plot (hydroPoints(n,1),hydroPoints(n,2),'-o');
    
    %Fill an area circumscribed by a circle of previously specifed radius.
    t = 0:0.01:2*pi;
    xrange = (radius * cos(t))+ hydroPoints(n,1);
    yrange = (radius * sin(t))+ hydroPoints(n,2);
    fill (xrange, yrange, 'm');
    
    %Label each hydrophone. (It may look kind of messy if the radius is
    %small).
    text(hydroPoints(n,1),hydroPoints(n,2),num2str(n));
end

%% Didn't Touch

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Line 136 is for pseudorandom magnitude component
% Comment in line 136 for pseudorandom magnitude component
%forx=(100.13+124.96*randn(129600,40)).*sign(randn(129600,40));
% Line 136 indicates that the, mean is 100 and standard dev. is 124
% The randn command shifts the 129600x40 table
% The second component is a centered 129600x40 table (1/2 values will
% be neg because of multiplication by sign)
% !(Each component: speed and direction?? Why is it possible to have
% negative magnitude??)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%% Iterate through fish and time
%The following loop iterates through each fish
%and looks at its entire swim. (Nested with time loop)

%Loop to run through number of fish, treating them as 'particles'.
%A display informs user which fish is being coded and info on lag time while
%the program is running.
%After each fish, a 2D matrix (oneFishInfo) is inserted into a larger 3D matrix (allFishInfo) as
%mentioned above.

fprintf('Time steps set to %d to avoid problem with lag in environmental model.\n', maxTime);
for j = 1:numFish
    fprintf('Running model for fish #%d. \n' , j);
    
    % Nested loop cycling through every swim of the fish
    for i = 2:maxTime
        
        %if above a certain value will you be out of bounds??
        %find where that fish is
        
        %Set base swim speed again for each individual
        swim = 30;
        %swim = input ('Enter swim speed in m/s': )

        %Gives the fish a new y & x position (between -swim or swim)
        y(i) = y(i-1) + randi([-swim*0.9 swim],1);
        x(i) = x(i-1) + randi([-swim*0.9 swim],1);
        
    end
    
    %Fish j is done, before proceeding to fish j+1, we store the jth fish's
    %information in the 2D matrix.
    oneFishInfo(:,1) = x;
    oneFishInfo(:,2) = y;
    
    %And then, we store that 2D matrix into the larger one.
    allFishInfo(:,:,j) = oneFishInfo;
end %End of numFish Loop

%Plot sample fish swim (for last fish only)
finalDisplay = input ('If you would like to view WITHIN BOUNDARY on plot \nENTER 1 if you would like to view ENTIRE FISH SWIM ENTER 0: ');

%if user wants to display just the bounded swim a plot is generated
if finalDisplay == 1
    %plots the boundary plus 10 on either side
    figure(fig);
    axis([xMin-10 xMax+10 yMin-10 yMax+10]);
    %plotting fish information's trail (swim)in blue within grid
    plot (x,y,'b');%cmap(i,:))
    hold on
    
    %or the user may choose to plot the entire swim of the fish
elseif finalDisplay == 0
    %plotting fish's trail(swim)in blue within grid
    figure(fig);
    plot (x,y,'b');
    hold on
end

%Found this section of code online and modified it a bit to help me
%time stamp the output files. Source: http://www.mathworks.com/matlabcentral/newsreader/view_thread/125781
time = clock; %Gets the current time as a 6 element vector

eval(['save \\DELLSERVER\t\MATLAB_scripts_and_data\AUV\ReconSim\sturgeonWalk' ...
    num2str(time(3)) '-' ...
    num2str(time(2)) '-' ...
    num2str(time(1)) '--' ...
    num2str(time(4)) '-' ...
    num2str(time(5)) ...
    '.mat' ' allFishInfo']);

%This save may prove to be unuseful over time ... nonetheless...
eval(['save \\DELLSERVER\t\MATLAB_scripts_and_data\AUV\ReconSim\everythingSturgeonWalk' ...
    num2str(time(3)) '-' ...
    num2str(time(2)) '-' ...
    num2str(time(1)) '--' ...
    num2str(time(4)) '-' ...
    num2str(time(5)) ...
    '.mat']);

%% Hydrophone Detection Section
%Optimally, we want to know which fish was recorded in which hydrophone and
%when. The following section does that computation and outputs the result
%for us.

%This segment iterates through the data to determine which (if any) fish were detected by the hydrophones
%It iterates through in much of the same way the larger for-loop did above.

%Set up a file array that is as long as the number of hydrophones.
file = zeros(numHydrophones);
for n = 1:numHydrophones
    fileName = ['hydrophone_' num2str(n) '.txt'];
    file(n) = fopen(fileName,'w');
    %Throws the hydrophone location at the top of each of them.
    fprintf(file(n), 'Location: ( %d , %d ) UTM\n',hydroPoints(n,1),hydroPoints(n,2)); 

end

%Hydrophone 'detective work'
for phone = 1:numHydrophones
    for j = 1:numFish
        for i = 1:maxTime
            
            %Pythagorean Theorem
            %See image on Pythagorean Theorem for explanation
            displacementX = hydroPoints(phone,1) - allFishInfo(i,1,j);
            displacementY = hydroPoints(phone,2) - allFishInfo(i,2,j);
            
            displacement = sqrt(displacementX^2 + displacementY^2);
            
            %Check to see if in range of hydrophone
            if(displacement <= radius) %Yes! Recorded a fish
                %Output to appropriate file
                fprintf(file(phone), 'Tracked fish #%d at time %d\n', j,i);
            end
        end
    end
    
end
end






