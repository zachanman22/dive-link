% For csv data
csv_filename = '32byteFrameFull';
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


% Determine bit time based on length of time each bit is sent for
% Noticed a pattern of bit times of 2 ms instead of 2.5 ms
bit_time = 1e-3;

% Sampling rate based on oscilloscope ADC rate
fs = 515464;

% Window len is number of samples per bit period
window_len = int32(fs * bit_time);
% Frequency width of each bin in Hz
bin_width = fs / window_len;
% Search adjacent bins in case the target frequencies are shifted
search_bin_radius = 3;


% Set target frequencies of FSK
target_f_1 = 70000;
target_f_2 = 120000;

% % Zero crossing (experimental)
% target_f_1_zc = 2 * target_f_1 / fs * window_len;
% target_f_2_zc = 2 * target_f_2 / fs * window_len;

data_to_spec = data.Channel1V;


% Determine which frequency bins the target frequencies are in
target_bin_1 = int32(target_f_1 / bin_width);
target_bin_2 = int32(target_f_2 / bin_width);


bit_points = (1:window_len:size(data_to_spec,1)).';

% BPF data to eliminate noise
% data_to_spec = bandpass(data_to_spec,[min([start_f, target_f_1, target_f_2]) - search_bin_radius * bin_width, ...
%                                       max([start_f, target_f_1, target_f_2]) + search_bin_radius * bin_width],fs);

% Can count zero crossings at a dc level to estimate the frequency
% Zero cross count ~= 2 * freq * bit_time
offset = 0 + 0*window_len;
dc = 0.10;
zero_count = zc_count(data_to_spec(offset + 1:window_len + offset), dc);

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
    start_index = find_start_index(data_to_spec, window_len, 4);

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

            plot(freqs(1:N/4), amps(1:N/4))
        
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
offset = 22903;
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

% Function of zero crossing count (experimental)
function count = zc_count(arr, dc)
    count = 0;
    for i = 2:size(arr, 1)
        if (arr(i) > dc && arr(i - 1) < dc) || (arr(i) < dc && arr(i - 1) > dc)
            if abs(arr(i) - arr(i - 1)) > 0
                arr(i)
                arr(i - 1)
                count = count + 1;
            end
        end
    end
end

function shift_count = find_start_index(data, window_len, window_factor)
    signal_square_avg = mean(data.^2);
    window_square_avg = signal_square_avg;
    next_window_square_avg = signal_square_avg - 1;
    shift_count = 1;
    search_window_len = int32(window_len / window_factor);
    while ~(window_square_avg <= signal_square_avg && next_window_square_avg >= signal_square_avg)  && shift_count < size(data,1) - search_window_len - 1
        window_square_avg = mean(data(shift_count:shift_count + search_window_len).^2);
        next_window_square_avg = mean(data(shift_count+1:shift_count + 1 + search_window_len).^2);
        shift_count = shift_count + 1;
    end
end