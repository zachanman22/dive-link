% MATLAB Script for a Binary FSK with two frequencies
format long;

% startIndex = 178542
% previousStartIndex = 1938181;
p = 0;
% for p = -30:30
% startIndex = startIndex2 + p*100;
startIndex = 198842 + p*100;
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
f = (f1):(baud):(f2);
freq_indices = round(f/fs*N) + 1;
% This time variable is just for plot
FSK_signal = reads(startIndex:end);
newt = t(startIndex:end);
correctBits = 0;

for ii = 1: 1: length(bit_stream)
    fhfsk_index = mod(ii-1, size(fhfsk_freq,1))+1;
    k = FSK_signal((ii-1)*N+1:(ii)*N);
    dft_data = goertzel(k,freq_indices);
    dft_data_mag = abs(dft_data);
    zero_bit_index = round((fhfsk_freq(fhfsk_index,1)-f1)/baud);
    one_bit_index = round((fhfsk_freq(fhfsk_index,2)-f1)/baud);
%     size(f,1)
    dft_data_0 = sum(dft_data_mag(zero_bit_index:zero_bit_index+2));
    dft_data_1 = sum(dft_data_mag(one_bit_index:one_bit_index+2));
    
    
    
    predictedBit = round((~(dft_data_0 > dft_data_1)).*1);
    
%     if(max(dft_data_mag(middle:end)) > max(dft_data_mag(1:middle)))
%         predictedBit = 1;
%     end
    if(predictedBit == bit_stream(ii))
        correctBits = correctBits + 1;
    end
%     figure(5);
%     plot(newt((ii-1)*N+1:(ii)*N),k)
%     figure(4);
%     plot(freq_indices,20*log10(dft_data_mag));
    figure(2);
    stem(f, dft_data_mag)
    
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
    pause;
%     disp("Accuracy: " + (correctBits/(ii)))
end
% disp("Start Index: " + startIndex + "  Accuracy: " + (correctBits/length(bit_stream)))
% if((correctBits/length(bit_stream)) > 0.6)
    disp("StartIndex: " + startIndex + "   Accuracy: " + (correctBits/length(bit_stream)))

% end