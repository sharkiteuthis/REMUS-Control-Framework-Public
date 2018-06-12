%clear all
%close all

numFish = 6; %Number of fish to model
fishColor = ['r','b','m','y','c','g'];
assert(numFish == length(fishColor))
maxTime = 600; %Length of time to run 

x_fish = zeros(maxTime,1);
y_fish = zeros(maxTime,1);
oneFishInfoZM = []; %Creation of 2D Matrix
allFishInfoZM = []; %Creation of 3D Matrix

%fig = figure('name','Physical Environment','numbertitle','off');
%hold on;
%grid on;

axis equal; %Axes set to equal for aestheic purposes.
%This sets the labels for the axes and title.
xlabel('Easting (UTM)');
ylabel('Northing (UTM)');
%title('Physical Environment');

% plot the first position of the first fish.
x_fish(1) = 585800;
y_fish(1) = 4628300;
%plot(x_fish(1),y_fish(1),'b*');

for j = 1:numFish
    fprintf('Running model for fish #%d. \n' , j);
    
    % Nested loop cycling through every swim of the fish
    for i = 2:maxTime

        swim = 50; %Set base swim speed again for each individual

        %Gives the fish a new y & x position (between -swim or swim)
        y_fish(i) = y_fish(i-1) + randi([-swim*0.9 swim],1);
        x_fish(i) = x_fish(i-1) + randi([-swim*0.9 swim],1);
        
    end
    
    %Fish j is done, before proceeding to fish j+1, we store the jth fish's
    %information in the 2D matrix.
    oneFishInfoZM(:,1) = x_fish;
    oneFishInfoZM(:,2) = y_fish;
    oneFishInfoZM(:,3) = repmat(j, length(y_fish),1);
    
    %And then, we store that 2D matrix into the larger one.
    allFishInfoZM = [allFishInfoZM; oneFishInfoZM];
end %End of numFish Loop

%plotting fish's trail(swim)in blue within grid
%figure(fig);
%for j = 1:numFish
%    plot(allFishInfoZM(:,1,j),allFishInfoZM(:,2,j),fishColor(j));
%end
%hold on

%Found this section of code online and modified it a bit to help me
%time stamp the output files. Source: http://www.mathworks.com/matlabcentral/newsreader/view_thread/125781
time = clock; %Gets the current time as a 6 element vector

eval(['save \\DELLSERVER\t\MATLAB_scripts_and_data\AUV\ReconSim\ReconFindFishZM_fast' ...
    num2str(time(3)) '-' ...
    num2str(time(2)) '-' ...
    num2str(time(1)) '--' ...
    num2str(time(4)) '-' ...
    num2str(time(5)) ...
    '.mat' ' allFishInfoZM']);

%This save may prove to be unuseful over time ... nonetheless...
%eval(['save \\DELLSERVER\t\MATLAB_scripts_and_data\AUV\ReconSim\ReconFindFishZM' ...
 %   num2str(time(3)) '-' ...
 %   num2str(time(2)) '-' ...
 %   num2str(time(1)) '--' ...
 %   num2str(time(4)) '-' ...
 %   num2str(time(5)) ...
 %   '.mat']);