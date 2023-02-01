N = sampling_frequency/baud
endIndex = startIndex + size(targetSymbols,2)*N;
f = (min_freq-2000):(baud):(max_freq+2000);
goertz(1,1:size(f,2)) = Goertz(30000,sampling_frequency);
for c = 1: size(f,2)
    goertz(1,c) = Goertz(f(c),sampling_frequency);
end
FSK_signal = reads(startIndex:end);
middle = int16(size(f,2)/2);
dft_data_mag = zeros(1,size(goertz,2));
dft_data_imag = zeros(1,size(goertz,2));
newt = t(startIndex:end);
figure(1)
plot(t,reads)

meanSaved = [];

for ii = size(targetSymbols,2)-100:1:size(targetSymbols,2)+30
    if((ii) < size(targetSymbols,2))
        display(targetSymbols(ii))
    else
        display("Message Over")
    end
    k = FSK_signal((ii-1)*N+1:ii*N);
    for c = 1:size(k,2)
        for h = 1:size(f,2)
            goertz(1,h) = goertz(1,h).processSample(k(c));
        end
    end

    for b = 1:size(f,2)
        dft_data_mag(1,b) = goertz(1,b).calcPurity(N);
        dft_data_imag(1,b) = goertz(1,b).calcImag();
        goertz(1,b) = goertz(1,b).reset();
    end

    dft_data_mag(1:middle) = dft_data_mag(1:middle)./10;
    dft_data_mag(middle:end) = dft_data_mag(middle:end);

    meanSaved(end+1) = mean(dft_data_mag);
    if(size(meanSaved,2) > 10 && meanSaved(end) < 0.05 && meanSaved(end-1) < 0.05 && meanSaved(end-2) < 0.05)
        display("message over at " + ii)
        pause;
    end

%     figure(3);
%     stem(f, dft_data_mag)
%     figure(5);
%     plot(newt((ii-1)*N+1:(ii)*N),k)
%     pause;
end

figure(7);
plot(meanSaved)
meanSaved