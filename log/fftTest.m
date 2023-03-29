% For csv data
csv_filename = 'elapsedTestFull.csv';
data = readtable(csv_filename, 'NumHeaderLines', 11);
data.Properties.VariableNames{1} = 'Times';
data.Properties.VariableNames{2} = 'Channel1V';

% % For mat data
% data_mat_filename = 'data.mat';
% data_mat = load(data_mat_filename);
% data = data_mat.data;

figure(1)
% Plot wrt time
% plot(data.Times, data.Channel1V)
% Plot wrt index
plot(data.Channel1V)
title("Raw Signal of Letter G")
xlabel("Sample Number")
ylabel("Volts (V)")



% Determine bit time based on length of time each bit is sent for
% Noticed a pattern of bit times of 2 ms instead of 2.5 ms
bit_time = 1e-3;

% Sampling rate based on oscilloscope ADC rate
fs = 519751;

% Window len is number of samples per bit period
window_len = int32(fs * bit_time);
% Frequency width of each bin in Hz
bin_width = fs / window_len;
% Search adjacent bins in case the target frequencies are shifted
search_bin_radius = 1;

peak_threshold = 50;


% Set target frequencies of FSK
target_f_1 = 70000;
target_f_2 = 120000;

data_to_spec = data.Channel1V;

% Determine which frequency bins the target frequencies are in
target_bin_1 = int32(target_f_1 / bin_width);
target_bin_2 = int32(target_f_2 / bin_width);


bit_points = (1:window_len:size(data_to_spec,1)).';

figure(4)
hold on
plot(data_to_spec)
stem(bit_points, zeros(size(bit_points)))
hold off


byte_size = 256;

% Determine bits based on maximum between the target frequency bins
start_index = 1;

bits = double.empty;
while size(data_to_spec,1) > window_len * byte_size
    start_index = find_start_index(data_to_spec, window_len, 4, fs, target_f_1, search_bin_radius, peak_threshold);
    start_index

    if size(data_to_spec,1) - start_index > window_len * byte_size
        for i = 1:byte_size
            plot(data_to_spec(start_index+(i-1)*window_len:start_index + i*window_len))
            amps = fft(data_to_spec(start_index+(i-1)*window_len:start_index + i*window_len));
            amps = abs(amps);
            
            N = size(amps, 1);
            
            % 1 sided FFT (positive freqs only)
            amps = amps(1:int32(N/2) + 1);
            amps = amps(1:int32(N/2));
            freqs = (0:N/2) * fs/N;
            freqs = freqs(1:int32(N/2));

%             plot(freqs(1:N/4), amps(1:N/4))
        
            % Max within search region of target frequency 1
            target_1_max = max(amps(target_bin_1 - search_bin_radius: ...
            target_bin_1 + search_bin_radius));
            % Max within search region of target frequency 2
            target_2_max = max(amps(target_bin_2 - search_bin_radius: ...
            target_bin_2 + search_bin_radius));
            % Target freq 1 is 0 and target freq 2 is 1
            bits = [bits, target_1_max < target_2_max];
        end
        data_to_spec = data_to_spec(start_index + byte_size * window_len:end);
    else
        break
    end
end

bits = reshape(bits, byte_size, []).';

csvwrite('output_test.csv', bits)


data_to_spec = data.Channel1V;



% Define offset
offset = 1;
% offset = 0 + window_len;
figure(5)
% Plot 1 frame starting from a particular offset
plot(data_to_spec(1+offset:window_len+offset));
% FFT amplitudes
% amps = fft(one_filtered);
amps = fft(data_to_spec(1+offset:window_len+offset));
% amps = fft(data_to_spec);

N = size(amps, 1);

% 1 sided FFT (positive freqs only)
amps = amps(1:int32(N/2) + 1);
amps = amps(1:int32(N/2));
freqs = (0:N/2) * fs/N;
freqs = freqs(1:int32(N/2));


figure(6)
% Plot FFT magnitude in db
plot(freqs,abs(amps))
title("FFT of Zero Bit")
xlabel("Frequency (Hz)")
ylabel("Amplitude (linear)")

function shift_count = find_start_index(data, window_len, window_factor, fs, search_freq, search_bin_radius, peak_threshold)
    next_window_0_peak = 0;
    shift_count = 0;
    search_window_len = int32(window_len / window_factor);
    while ~(next_window_0_peak >= peak_threshold) && shift_count < size(data,1) - search_window_len - 1
        amps = fft(data(shift_count+1:shift_count + 1 + search_window_len));
        amps = abs(amps);

        bin_width = fs / search_window_len;
        zero_freq_bin = int32(search_freq / bin_width);
%         zero_freq_bin = int32(zero_freq_bin * window_factor);
%         zero_freq_bin
        
        N = size(amps, 1);
        % 1 sided FFT (positive freqs only)
        amps = amps(1:int32(N/2) + 1);
        amps = amps(1:int32(N/2));

        next_window_0_peak = max(amps(zero_freq_bin - search_bin_radius: ...
            zero_freq_bin + search_bin_radius));
%         next_window_square_avg = mean(data(shift_count+1:shift_count + 1 + search_window_len).^2);
        shift_count = shift_count + 1;
    end
end