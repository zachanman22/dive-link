% MATLAB Script for a Binary FSK with two frequencies
format long;

% newStartIndex = 178542
% previousnewStartIndex = 1938181;

u = 1.15;
for u = .6:0.05:1.35
p = 0;
for p = 5:20
newStartIndex = startIndex + p*200;
% Clear all variables and close all figures
% The number of bits to send - Frame Length
% Generate a random bit stream
bit_stream = targetSymbols;
% Enter the two frequencies 
% Frequency component for 0 bit
f1 = min_freq; 
% Frequency component for 1 bit
f2 = max_freq;
% Sampling rate - This will define the resoultion
fs = sampling_frequency;
% Time for one bit

N = sampling_frequency/baud;
f = (f1-2000):(baud):(f2+2000);
freq_indices = round(f/fs*N) + 1;
% This time variable is just for plot
FSK_signal = reads(newStartIndex:end);
newt = t(newStartIndex:end);
k = FSK_signal(1:N);
previousMag = abs(goertzel(k,freq_indices));
middle = int16(size(previousMag,2)/2);
previousMag(1:middle) = previousMag(1:middle);
previousMag(middle:end) = previousMag(middle:end);
derivativeGain = 0;
correctBits = 0;

for ii = 1: 1: length(bit_stream)
    
    k = FSK_signal((ii-1)*N+1:(ii)*N);
    dft_data = goertzel(k,freq_indices);
    dft_data_mag = abs(dft_data);
    dft_data_mag(1:middle) = dft_data_mag(1:middle)/u;
    dft_data_mag(middle:end) = dft_data_mag(middle:end);% 0.81508
    derivative = (dft_data_mag - previousMag)*baud;
    
%     dft_data_scaled = 2*dft_data/numel(k);
%     stem(f,abs(dft_data_scaled))
    
%     stem(f(indxs),mag2db(abs(squeeze(dft_data))))

    filterThreshold = 0;
    temp_dft_mag = dft_data_mag;
%     temp_dft_mag(temp_dft_mag<filterThreshold) = 0;
    temp_der = derivative;
    temp_der(temp_dft_mag<filterThreshold) = 0;
%     temp_der(temp_der<0) = 0;
    adj_mag = (temp_dft_mag + temp_der*derivativeGain);
    
    weightedfrequency = 0;
    total_adj_mag = 0;
    for c = 1:size(adj_mag,2)
       weightedfrequency = weightedfrequency + f(c)*adj_mag(c);
       total_adj_mag = total_adj_mag + adj_mag(c);
    end
    weightedfrequency = weightedfrequency/total_adj_mag;
    predictedBit = 0;
    
    if(max(dft_data_mag(middle:end)) > max(dft_data_mag(1:middle)))
%     if(max(adj_mag(middle:end)) > max(adj_mag(1:middle)))
%     if(weightedfrequency > (f1+f2)/2)
        predictedBit = 1;
    end
    if(predictedBit == bit_stream(ii))
        correctBits = correctBits + 1;
    end
%     figure(5);
%     plot(newt((ii-1)*N+1:(ii)*N),k)
%     figure(4);
%     plot(freq_indices,20*log10(dft_data_mag));
%     figure(1);
%     stem(f, derivative)
%     grid()
%     figure(2);
%     stem(f, dft_data_mag)
%     grid()
%     figure(3);
%     stem(f, adj_mag)
    
    previousMag = dft_data_mag;
    ax = gca;
    ax.XTick = f;
    xlabel('Frequency (Hz)')
    ylabel('DFT Magnitude')
    title(bit_stream(ii))
% if (correctBits/(ii) < 0.75 && correctBits > 40)
%     break
% end
%     disp('Press a key !')  % Press a key here.You can see the message 'Paused: Press any key' in        % the lower left corner of MATLAB window.
%     pause;
%     disp("Accuracy: " + (correctBits/(ii)))
end
% disp("Start Index: " + newStartIndex + "  Accuracy: " + (correctBits/length(bit_stream)))
% if((correctBits/length(bit_stream)) > 0.6)
    disp("Divider: " + u  + " newStartIndex: " + newStartIndex + "   Accuracy: " + (correctBits/length(bit_stream)))
% end
% end
% disp("newStartIndex: " + newStartIndex + "   Accuracy: " + (correctBits/length(bit_stream)))
% end
end
end