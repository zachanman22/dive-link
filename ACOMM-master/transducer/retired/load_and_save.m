clear all
% hundred_baud
% twofifty_baud
% ten_baud

folder = "./tests/250/";
readmeFileName = "readme";
dataFileName = "baud_250__25_33k___1";
ADCCENTER = 128;
sampling_frequency = 100000;
min_freq = 25000;
max_freq = 33000;

% read the data file into memory
dataFilePath = folder+dataFileName;
dataID = fopen(dataFilePath,'r');
formatSpec = '%f %i';
sizeA = [2 Inf];
A  = fscanf(dataID,formatSpec,sizeA);
t = (A(1,:)-A(1:1))/1000000;
% reads = (A(2,:)-ADCCENTER)/(2*ADCCENTER);
reads = A(2,:);