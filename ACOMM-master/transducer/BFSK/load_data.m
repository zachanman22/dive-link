clear all
% hundred_baud
% twofifty_baud
% ten_baud

% C:\Users\sctma\Documents\Arduino\libraries\ACOMM\transducer\tests\acoustic_tank\2022-05-25\10\8m\25_32k__3000gain__30__sampleK_125_497692.txt
% transducer\tests\sweetwater_creek\2022-06-01\50\15m\25_32k__3000gain__30__sampleK_200_101139_readme.txt
% transducer\tests\acoustic_tank\2022-06-06\BFSK\40\10m\.txt
% transducer\tests\acoustic_tank\2022-06-06\BFSK\100\10m\25_32k__3000gain__40__sampleK_200_548285.txt
folder = "C:/Users/sctma/Documents/Arduino/libraries/ACOMM/transducer/tests/acoustic_tank/2022-06-06/BFSK/100/10m/";
% transducer\tests\acoustic_tank\2022-06-06\BFSK\40\10m\25_32k__3000gain__40__sampleK_200_548090.txt
% transducer\tests\acoustic_tank\2022-06-06\BFSK\10\10m\25_32k__3000gain__40__sampleK_200_547969_readme.txt
% transducer\tests\acoustic_tank\2022-05-25\40\8m\25_32k__3000gain__30__sampleK_200_499293.txt
dataFileName = "25_32k__3000gain__40__sampleK_200_548285";
readmeFileName = dataFileName + "_readme.txt";
dataFileName = dataFileName + ".txt";
% transducer\tests\acoustic_tank\2022-05-25\40\8m\25_32k__3000gain__30__sampleK_200_500273.txt
% transducer\tests\acoustic_tank\2022-05-25\40\8m\25_32k__3000gain__30__sampleK_200_500428.txt
% transducer\tests\acoustic_tank\2022-05-25\20\8m\25_32k__3000gain__30__sampleK_200_498882.txt
% transducer\tests\acoustic_tank\2022-05-25\40\8m\25_32k__3000gain__30__sampleK_200_499293.txt
% transducer\tests\acoustic_tank\2022-05-25\50\2m\25_32k__3000gain__10__sampleK_200_505936.txt

ADC_OFFSET_CENTER = 2048;
ADCCENTER = 2048;
sampling_frequency = 200000;
min_freq = 25000;
max_freq = 32000;

%read the descriptor info into memory
readmeFilePath = folder+readmeFileName;
readmeID = fopen(readmeFilePath,'r');
B = importdata(readmeFilePath);
baud = B.data(1);
volume = B.data(2);
targetString = cell2mat(B.textdata(2));
targetString = targetString(2:end-2);
targetString = strcat('1',targetString);
targetSymbols = targetString-'0';

% read the data file into memory
dataFilePath = folder+dataFileName;
dataID = fopen(dataFilePath,'r');
formatSpec = '%f %i';
sizeA = [2 Inf];
A  = fscanf(dataID,formatSpec,sizeA);

t = (A(1,:)-A(1:1))/1000000;
reads = (A(2,:)-ADC_OFFSET_CENTER)/(ADCCENTER);


disp("Load complete of " + dataFileName)
t2 = (1:size(reads,2));
figure(1);
plot(t,reads,(t2-1)/sampling_frequency,reads);
title("Time Stamped Data and Raw Data Comparison")

figure(2);
plot(t2,reads)
title("Raw Signal from PreAmp")
