
g = Goertz(33000.0,sampling_frequency)
N = sampling_frequency/baud;

f = (min_freq-2000):(baud):(max_freq+2000);
freq_indices = round(f/sampling_frequency*N) + 1;
goertz(1,1:size(f,2)) = Goertz(30000,sampling_frequency);

for c = 1: size(f,2)
    goertz(1,c) = Goertz(f(c),sampling_frequency);
end

for c = 1:N
    for k = 1:size(f,2)
        goertz(1,k) = goertz(1,k).processSample(reads(startIndex+c));
        disp(goertz(1,k))
    end
end

mags = zeros(1,size(goertz,2));
for k = 1:size(f,2)
    mags(1,k) = goertz(1,k).calcMagnitude();
end
mags
stem(f,mags)