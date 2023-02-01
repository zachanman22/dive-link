folder = "./tests/ten_baud/";
readmeFileName = "readme";
dataFileName = "baud_10__25_33k";
ADCCENTER = 128;
sampling_frequency = 100000;
min_freq = 25000;
max_freq = 33000;

%read the descriptor info into memory
readmeFilePath = folder+readmeFileName;
readmeID = fopen(readmeFilePath,'r');
formatSpec = '%s\n%s\n%i\n%f';
B = importdata(readmeFilePath);
baud = B.data(1);
volume = B.data(2);
targetString = cell2mat(B.textdata(2));
targetString = targetString(2:end-2);
targetSymbols = targetString-'0';

% read the data file into memory
dataFilePath = folder+dataFileName;
dataID = fopen(dataFilePath,'r');
formatSpec = '%f %i';
sizeA = [2 Inf];
A  = fscanf(dataID,formatSpec,sizeA);
t = A(1,:);
reads = A(2,:);
disp("Load complete")
