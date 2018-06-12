function [gorecon,expiration] = CheckCoolDown(SAOVR_start, tagid_number, global_cooldown, tag_cooldown, time_in)

%TMG 11/7/15
%This function is called by ReconSim or its derivatives to see if there is
%a global or local tag cool down period in affect for running maneuvers

%Check global cooldown timer
%The global timer has to be set at a given time i in ReconSim as
%SAVOR_start
%Then that time i has to be added to the cooldown time to form an allowed time back in.
%For example, if SAOVR is initiated at 2100, and cooldown is 240, time
%allowed in is 2340

expiration = SAOVR_start + global_cooldown
if SAOVR_start > 0; %SAOVR starts as  [], so the first encounter with any tag initiates a manouver
            
            gorecon = 1;
    
else
            %SAOVR_start is only empty if it has not run before.
            %Since this condition is true if it has run before, a new
            %SAOVR_start and expiration will have been set
            %if expiration > time_in
                gorecon = 0; %0 is no go on maneuver
                
            %else
            %    gorecon = 1;
            %end
end

%load tags in cooldown, for now this is not enabled and global cooldown is
%left
if tagid_number == 0;
    tag_cooldown = 0;
end

