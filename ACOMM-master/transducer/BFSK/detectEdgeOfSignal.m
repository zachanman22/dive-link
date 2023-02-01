%try sliding window goertzel with dft magnitude threshold

maxPosOfStartMessage = size(reads,2)-50;
N = 20;
% f = (min_freq+3000):(sampling_frequency/N):(max_freq)
f = [max_freq];
freq_indices = round(f/sampling_frequency*N) + 1;
x=zeros(1,maxPosOfStartMessage);
startIndex = 0;
for ii = 1:1:maxPosOfStartMessage
    samples = reads(ii:N+ii);
    x(ii) = abs(goertzel(samples,freq_indices));
    if(x(ii) > 0.2 && startIndex == 0)
        startIndex = ii
        break
    end
end
plot(t(1:maxPosOfStartMessage), x)