%function [num_records, kurt, skew, time_in_range, x_inflex, y_inflex] = AssessRecon(numFish)
figure
%close all
all_rec = [];
all_kurt = [];
all_skew = [];
all_trange = [];
allxflex = [];
allyflex = []; 
fishpath = input('Enter the save to path in single quotes with final backslash: ');

for f = 1:30
    name = num2str(f)
    pv = strcat(['load ',fishpath,'Fish_', num2str(f)]);
    eval(pv)
    %Fish
    if ~isempty(Fish.Record)
        radius = Fish.detect_r;
        tag_record = downsample(Fish.Record,5);

        figure
        %plot(range_record_all(:,1),range_record_all(:,2),'.')
        hold on
        plot(tag_record(:,1),tag_record(:,2),'ro')
        ylabel('Distance to tag')
        xlabel('Mission time (s)')

        if size(tag_record) < 2;
            continue
        end
        K = numel(tag_record(:,2));
        
        bias = tag_record(:,2) / radius; % the bias is calculated first as the the distance detected divided by the radius of detection
                                    % which was set in the sim. This
                                    % reduces the bias to fraction, with
                                    % the farthest diistance being near 1.
                                    % This has to be flipped, so we
                                    % multiply by -1, then raise the
                                    % intercept by 1                  
        bias = (bias*-1)+1;
        
        tags_logged = randsample(tag_record(:,2),K,true,bias)

        [C, Ilogged, Irec] = intersect(tags_logged,tag_record(:,2),'stable');

        log_record = [tag_record(Irec,1),tag_record(Irec,2)]
        plot(log_record(:,1),log_record(:,2),'g^')
    
    end
    [num_records,c] = size(log_record);
    num_records
    kurt = kurtosis(log_record(:,2))
    skew = skewness(log_record(:,2))
    time_in_range = max(log_record(:,1)) - min(log_record(:,2))
    
    xs = log_record(:,1); ys =log_record(:,2);
    I = 1:numel(xs)-2;
    sgnA = sign( ...
    xs(I+0).*ys(I+1) ...
  + xs(I+1).*ys(I+2) ...
  + xs(I+2).*ys(I+0) ...
  - xs(I+0).*ys(I+2) ...
  - xs(I+1).*ys(I+0) ...
  - xs(I+2).*ys(I+1) ...
);
k_inflex = find(2 == abs(diff(sgnA)));
x_inflex = xs(k_inflex);
[rx cx] = size(x_inflex);
y_inflex = ys(k_inflex)
[ry cy] = size(x_inflex);   
%add the variables to arrays in order to create histograms
all_rec = [all_rec; num_records];
all_kurt = [all_kurt; kurt];
all_skew = [all_skew; skew];
all_trange = [all_trange; time_in_range];
allxflex = [allxflex; rx];
allyflex = [allyflex; ry];   
end %end fish loop
figure
subplot(3,2,1)
hist(all_rec)
xlabel('No. of detecions')

subplot(3,2,2)
hist(all_kurt);
xlabel('Kurtosis')
subplot(3,2,3)
hist(all_skew);
xlabel('Skewness')
subplot(3,2,4)
hist(all_trange);
xlabel('Time in range (s)')
subplot(3,2,5)
hist(allxflex); 
xlabel('No. of inflexions')
gcf
pv = strcat(['print ',fishpath,'ResultsPtFwd.png -dpng -r600']);
eval(pv)

pv = strcat(['save ',fishpath,'ResultsPtFwd_stats all_rec all_kut all_skew all_trange allxflex allyflex']);
eval(pv)