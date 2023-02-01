%try sliding window goertzel with dft magnitude threshold

maxPosOfStartMessage = size(reads,2)-50;
N = 50;
% f = (min_freq+3000):(sampling_frequency/N):(max_freq)
numberOfCorrectMatches = 5;
matchCorrectCounter = 0;
f = [max_freq];
freq_indices = round(f/sampling_frequency*N) + 1;
x=zeros(1,maxPosOfStartMessage);
startIndex = 0;
g = Goertz(max_freq,sampling_frequency);
for ii = 1:1:maxPosOfStartMessage
    samples = reads(ii:N+ii);
    for c = 1:size(samples)
        g = g.processSample(samples(c));
    end
    pure = g.calcPurity(N);
    x(1,ii) = pure;
    g = g.reset();
    if(x(ii) > 0.0001 && startIndex == 0)
        matchCorrectCounter = matchCorrectCounter + 1;
    else
        matchCorrectCounter = 0;
    end
    if(matchCorrectCounter >= numberOfCorrectMatches && startIndex == 0)
        startIndex = ii
        break
    end
end
plot(t(1:maxPosOfStartMessage), x)
title("Purity of signal")