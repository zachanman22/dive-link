clear all
% hundred_baud
% twofifty_baud
% ten_baud

% C:\Users\sctma\Documents\Arduino\libraries\ACOMM\transducer\tests\acoustic_tank\2022-05-25\10\8m\25_32k__3000gain__30__sampleK_125_497692.txt
% transducer\tests\sweetwater_creek\2022-06-01\50\15m\25_32k__3000gain__30__sampleK_200_101139_readme.txt
% transducer\tests\acoustic_tank\2022-06-13\fhfsk\100\10m\fhfsk__3000gain__100__sampleK_200_132213.txt
% transducer\tests\acoustic_tank\2022-06-17\fhfsk\160\10m\OP820\fhfsk__3000gain__100__sampleK_200_479123.txt
folder = "C:/Users/sctma/Documents/Arduino/libraries/ACOMM/transducer/tests/crc_pool/2022-06-17/fhfsk/250/50m/OP820/";
% C:\Users\sctma\Documents\Arduino\libraries\ACOMM\transducer\tests\acoustic_tank\2022-06-06\fhfsk\10\10m\fhfsk__3000gain__100__sampleK_200_547567.txt
% transducer\tests\acoustic_tank\2022-06-13\fhfsk\100\10m\fhfsk__3000gain__100__sampleK_200_133740.txt
dataFileName = "fhfsk__3000gain__100__sampleK_200_486399";
% fhfsk__3000gain__100__sampleK_200_486399
% transducer\tests\crc_pool\2022-06-17\fhfsk\250\50m\OP820\fhfsk__3000gain__100__sampleK_200_486399.txt
% transducer\tests\kraken_springs\2022-06-23\fhfsk\125\1m\OP820\fhfsk__3000gain__100__sampleK_200_003677_readme.txt
% transducer\tests\kraken_springs\2022-06-23\fhfsk\125\1m\OP820\fhfsk__30000gain__100__sampleK_200_998243.txt
% transducer\tests\kraken_springs\2022-06-23\fhfsk\125\1m\OP820\fhfsk__10000gain__100__sampleK_200_998060.txt
% transducer\tests\kraken_springs\2022-06-23\fhfsk\125\1m\OP820\fhfsk__10000gain__100__sampleK_200_997860_readme.txt
% fhfsk__10000gain__100__sampleK_200_995161
% transducer\tests\crc_pool\2022-06-17\fhfsk\320\50m\OP820\fhfsk__3000gain__100__sampleK_200_486709.txt
% transducer\tests\crc_pool\2022-06-17\fhfsk\250\50m\OP820\fhfsk__3000gain__100__sampleK_200_486399.txt

readmeFileName = dataFileName + "_readme.txt";
dataFileName = dataFileName + ".txt";


ADC_OFFSET_CENTER = 2048;
ADCCENTER = 2048;
sampling_frequency = 200000;
min_freq = 22000;
max_freq = 45000;

%read the descriptor info into memory
readmeFilePath = folder + readmeFileName;
readmeID = fopen(readmeFilePath, 'r');
B = importdata(readmeFilePath);
baud = B.data(1);
volume = B.data(2);
try
   gain = B.data(3);
catch ME
   gain = 3000;
end 


targetString = cell2mat(B.textdata(2));
targetString = targetString(2:end - 2);
targetString = strcat('1', targetString);
targetSymbols = targetString - '0';

fhfsk_string = erase(cell2mat(B.textdata(3))," ");
fhfsk_string = split(fhfsk_string(4:end - 4),'},{');
fhfsk_freq = zeros(size(fhfsk_string,1), 2);
for c = 1:size(fhfsk_string,1)
    fhfsk_string{c} = split(fhfsk_string{c}, ",");
    fhfsk_freq(c,1) = str2double(fhfsk_string{c}{1});
    fhfsk_freq(c,2) = str2double(fhfsk_string{c}{3});
    
end

% read the data file into memory
dataFilePath = folder + dataFileName;
dataID = fopen(dataFilePath, 'r');
formatSpec = '%f %i';
sizeA = [2 Inf];
A = fscanf(dataID, formatSpec, sizeA);

t = (A(1, :) - A(1:1)) / 1000000;
reads = (A(2, :) - ADC_OFFSET_CENTER) / (ADCCENTER);

disp("Load complete of " + dataFileName)
t2 = (1:size(reads, 2));
% figure(1);
% plot(t, reads, (t2 - 1) / sampling_frequency, reads);
% title("Time Stamped Data and Raw Data Comparison")

figure(2);
plot(t2, reads)
title("Raw Signal from PreAmp")
