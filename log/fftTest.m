data_mat_filename = 'data.mat';
data_mat = load(data_mat_filename);
data = data_mat.data;

figure(1)
% Plot wrt time
plot(data.Times, data.Channel1V)
% Plot wrt index
plot(data.Channel1V)

% Determine bit time based on length of time each bit is sent for
% Noticed a pattern of bit times of 2 ms instead of 2.5 ms
bit_time = 2e-3;
% Sampling rate based on oscilloscope ADC rate
fs = 2e6;

% Window len is number of samples per bit period
window_len = fs * bit_time;
% Frequency width of each bin in Hz
bin_width = fs / window_len;
% Search adjacent bins in case the target frequencies are shifted
search_bin_radius = 6;


% Set target frequencies of FSK
target_f_1 = 45000;
target_f_2 = 60000;

% % Zero crossing (experimental)
% target_f_1_zc = 2 * target_f_1 / fs * window_len;
% target_f_2_zc = 2 * target_f_2 / fs * window_len;

% Data to provide to spectrogram if slicing needed
% data_to_spec = data.Channel1V(2000:32000);
data_to_spec = data.Channel1V;

% Determine which frequency bins the target frequencies are in
target_bin_1 = int32(target_f_1 / bin_width);
target_bin_2 = int32(target_f_2 / bin_width);

% Spectrogram is FFT over time, and we want FFTs of each frame through time
% Spectrogram with window length as determined previously and 0 overlap
% Length of FFT/DFT is same as windows length
% Sampling frequency provided
% Place freqs on yaxis
% Output is FFT values, frequencies, and times
[s, w, t] = spectrogram(data_to_spec, window_len, 0, window_len, fs, 'yaxis');
figure(2)
spectrogram(data_to_spec, window_len, 0, window_len, fs, 'yaxis')

% Get magnitude of FFT values
s = abs(s);
% Number of frequency bins
num_w_bins = size(w, 1);
% Filters to just 0-100kHz for a fs of 2 MHz
s = s(1:int32(num_w_bins/10), :);
w = w(1:int32(num_w_bins/10));

figure(3)
waterfall(t,w,s)
xlabel('Time (s)')
ylabel('Freq (Hz)')
colorbar

% surf(t,w,s)

% Determine bits based on maximum between the target frequency bins
bits = zeros(size(t));
for i = 1:size(bits,2)
    s(target_bin_1 - search_bin_radius: ...
    target_bin_1 + search_bin_radius, i)
    % Max within search region of target frequency 1
    target_1_max = max(s(target_bin_1 - search_bin_radius: ...
    target_bin_1 + search_bin_radius, i));
    % Max within search region of target frequency 2
    target_2_max = max(s(target_bin_2 - search_bin_radius: ...
    target_bin_2 + search_bin_radius, i));
    % Target freq 1 is 0 and target freq 2 is 1
    bits(i) = target_1_max < target_2_max;
end

% % For zero crossing (experimental)
% dc = 4;

% Define offset
offset = 9900;
figure(4)
% Plot 1 frame starting from a particular offset
plot(data.Channel1V(1+offset:window_len+offset));
% FFT amplitudes
amps = fft(data.Channel1V(1+offset:window_len+offset));

% % Zero cross (experimental)
% zero_cross = zc_count(data.Channel1V(1+offset:window_len+offset), dc);

N = size(amps, 1);

% 1 sided FFT (positive freqs only)
amps = amps(1:N/2 + 1);
amps = amps(1:int32(N/20));
freqs = (0:N/2) * fs/N;
freqs = freqs(1:int32(N/20));


figure(5)
% Plot FFT magnitude in db
plot(freqs,mag2db(abs(amps)))

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