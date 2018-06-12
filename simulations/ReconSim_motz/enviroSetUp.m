%The name of this file appears on (or around) line #170 in newSturgeonWalk

function enviroSetUp

steps = 43200; %time steps number of minutes in 30 days

N = 30; % half tidal periods, gets doubled +/- start time 
t1 = -N*pi;
t2 = N*pi;
t = linspace(t1, t2, steps);
tideArr = cos(t); %creates tides for 30 day interval minute by minute

t2 = linspace(0, 5, steps); %??? why step from 0 to 5? Appears to be used for creating info about salinity, temp,and nutrients

%Changed this around a little. Functions store values directly into an
%array. Named accordingly.
salinityArr = exp(-abs(t2));
tempArr = atan(t2);
tideWithLagArr = sin(t2);
nutrientArr = t2+(t2./steps);

    % Figure is providing the tide information used during run
    fig1 = figure('name','Tide','numbertitle','off');
    plot(t,tideArr)
    xlim([0 100])
    xlabel('Time');
    ylim([-3 3]);
    ylabel('Tide');
    %Label axes!!!
    grid on;
    hold on;

    fig2 = figure('name','Condition Information','numbertitle','off');
    plot(t2,salinityArr,'b');
    hold on;
    grid on;
    plot(t2,tempArr,'g')
    plot(t2,tideWithLagArr,'k')
    plot(t2,nutrientArr, 'r');
    
    legend('Salinity (Neg-Exponential)', ' Temperature(Tangent-Inverse)', ' Tide (Sine)', 'Nutrient (Linear)');
    xlabel('Tide');
    ylabel('Env. Condition Value');
    title('Environment Conditions');

t_step = (1:1:steps);

    titleFile = ['sturgeonEnvironmentVars' '.txt'];
    fileID = fopen(titleFile,'w');
    fprintf(fileID,'%6s %12s %12s %12s %12s\n','tide', 'sal','temp','tideW/Lag','nutri');
    fprintf(fileID,'%6.2f %12.8f %12.8f %12.8f %12.8f\n',tideArr,salinityArr,tempArr,tideWithLagArr,nutrientArr);
    fclose(fileID);


save sturgeonEnvironmentConditions t_step tideArr salinityArr tempArr tideWithLagArr nutrientArr;
end

 
