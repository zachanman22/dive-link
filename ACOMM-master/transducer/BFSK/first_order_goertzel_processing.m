% MATLAB Script for a Binary FSK with two frequencies
format long;
bit_stream = targetSymbols;
numberOfSamples = 20;
for u = 1:9
for p = -10:10
    startIndex = 131168+p*25;
    purityDivider = u;
    N = sampling_frequency/baud;
    f = (min_freq-2000):(baud):(max_freq+2000);
    goertz(1,1:size(f,2)) = Goertz(30000,sampling_frequency);
    for c = 1: size(f,2)
        goertz(1,c) = Goertz(f(c),sampling_frequency);
    end


    % This time variable is just for plot
    FSK_signal = reads(startIndex:end);
    newt = t(startIndex:end);
    correctBits = 0;
    middle = int16(size(f,2)/2);
    dft_data_mag = zeros(1,size(goertz,2));
    dft_data_imag = zeros(1,size(goertz,2));
    accuracyTracker = zeros(1,size(bit_stream,2));
    meanSaved = zeros(1,size(bit_stream,2));
    for ii = 1: 1: numberOfSamples %length(bit_stream)
        k = FSK_signal((ii-1)*N+1:(ii)*N);
        for c = 1:size(k,2)
            for h = 1:size(f,2)
                goertz(1,h) = goertz(1,h).processSample(k(c));
            end
        end

        for b = 1:size(f,2)
            dft_data_mag(1,b) = goertz(1,b).calcPurity(N);
            dft_data_imag(1,b) = goertz(1,b).calcImag();
        end

        dft_data_mag(1:middle) = dft_data_mag(1:middle)./purityDivider;
        dft_data_mag(middle:end) = dft_data_mag(middle:end);

    %     filterThreshold = 1;
    %     temp_dft_mag = dft_data_mag;
    %     temp_dft_mag(temp_dft_mag<filterThreshold) = 0;
    %     temp_der = derivative;
    %     temp_der(temp_dft_mag<filterThreshold) = 0;
    %     temp_der(temp_der<0) = 0;
        adj_mag = dft_data_mag;

        weightedfrequency = 0;
        total_adj_mag = 0;
        for c = 1:size(adj_mag,2)
           weightedfrequency = weightedfrequency + f(c)*adj_mag(c);
           total_adj_mag = total_adj_mag + adj_mag(c);
        end
        weightedfrequency = weightedfrequency/total_adj_mag;
        predictedBit = 0;
        if(weightedfrequency > (min_freq+max_freq)/2)
            predictedBit = 1;
        end
        if(predictedBit == bit_stream(ii))
            correctBits = correctBits + 1;
            accuracyTracker(ii) = 1;
        end
        meanSaved(ii) = mean(dft_data_mag);

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
%         figure(3);
%         stem(f, adj_mag)

        for h = 1:size(f,2)
            goertz(1,h) = goertz(1,h).reset();
        end

        previousMag = dft_data_mag;
        ax = gca;
        ax.XTick = f;
        xlabel('Frequency (Hz)')
        ylabel('DFT Magnitude')
        title(bit_stream(ii))
    %     disp('Press a key !')  % Press a key here.You can see the message 'Paused: Press any key' in        % the lower left corner of MATLAB window.
    %     pause;
%         disp("Accuracy: " + (correctBits/(ii)))
%         disp(ii)

    end
%     disp("StartIndex: " + startIndex + "   Accuracy: " + (correctBits/length(bit_stream)))
    disp("PurityDivider: " + purityDivider + "   StartIndex: " + startIndex + "   Accuracy: " + (correctBits/numberOfSamples))
%     figure(8);
%     plot(meanSaved)
end
end