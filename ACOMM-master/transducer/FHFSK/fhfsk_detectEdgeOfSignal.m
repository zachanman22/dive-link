%try sliding window goertzel with dft magnitude threshold

% gain = 10000;
threshold = 0.3;
if(gain == 10000)
    threshold = 0.65;
elseif(gain == 30000)
    threshold = 1.1 ;
end
maxPosOfStartMessage = size(reads,2)-50;
N = 20;
% f = (min_freq+3000):(sampling_frequency/N):(max_freq)
f = [fhfsk_freq(1,2)];
freq_indices = round(f/sampling_frequency*N) + 1;
x=zeros(1,maxPosOfStartMessage);
startIndex = 0;
beginningIndex = startIndex;
for ii = max(1,startIndex):1:maxPosOfStartMessage
    samples = reads(ii:N+ii);
    x(ii) = abs(goertzel(samples,freq_indices));
    if(x(ii) > threshold && startIndex == beginningIndex)
        startIndex = ii
        startIndex2 = ii;
        break
    end
end
% plot(t(1:maxPosOfStartMessage), x)