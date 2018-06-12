function tagheard = CheckTag(current_x, current_y, radius, iter, fishpath)

%TMG 11/7/2015 
%This function is called by ReconSim or one of its drivatives. 
%It checks the possibility of tag detection at every remus
% position during the mission simulation. It does this by checking if a tag
%exists within range of the current position using the same script that is 
%embedded in teh default no-response run AFTER the mission completion.
%If a tag is in range, it checks the range, and then assigns a probability
%of detection to the tag based on the tag's actual distance at time (t).
%Then it makes a final decision on whether the tag is heard or not. If the
%decision is positive, it returns only a tag ID to the SIM.

%load the Fish record
piter = strcat(['load ',fishpath,'Fish_',num2str(iter)]);
eval(piter)

Dx = current_x - Fish.fish_east;
Dy = current_y - Fish.fish_north;
        
displacement = sqrt(Dx.*Dx + Dy.*Dy);

%Check to see if fish in range of hydrophone, if so assign probability
if(displacement <= radius)
   % disp('True, less than radius')
    %add a weighting method here
     
     %a = randi([0 1]);
     %if a == 1;
         tagheard = Fish.fish;
 else
         tagheard = 0;
     %end     
     
 %else
 %    tagheard = 0;
 end