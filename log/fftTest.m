figure(1)
plot(data.Times,data.Channel1V)

f = fft(data.Channel1V);
N = size(f);
N = N(1);
fs = 2e6;
freqs = (0:N-1) * fs/N;
figure(2)
plot(freqs,abs(f))
