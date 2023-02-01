% 50 m acoustic tank FHFSK test

data = [[100 50 50],
        [100 100 50],
           [100 125 50],
         [100 160 50],
         [100 200 50],
         [99 250 50],
         [84.5 320 50]]
     
 data(1:end, 1)
figure

x = data(1:end,2)
y = 100 - data(1:end,1)
p = plot(x,y, '-o', 'MarkerIndices',1:length(y))
title('FHFSK BER vs Baud Rate at 50m in CRC Pool')
xlabel('Baud Rate')
ylabel('Bit Error Ratio (%)')
p(1).LineWidth = 1.5;

% 10 m acoustic tank FHFSK test
data = [[100 10],
        [100 20],
        [100 40],
         [100 50],
         [99 100],
         [100 125],
         [98 200],
         [96.5 250],
         [81 400]]
data2 = [[100 10],
        [99.5 20],
        [100 40],
         [100 50],
         [90.5 80],
         [89 100]]
figure


x = data(1:end,2)
y = 100 - data(1:end,1)
x2 = data2(1:end,2)
y2 = 100 - data2(1:end,1)
p = plot(x,y, '-o', x2, y2, '-o', 'MarkerIndices',1:length(y))
title('BER vs Baud Rate at 10m in Acoustic Tank')
xlabel('Baud Rate')
ylabel('Bit Error Ratio (%)')
legend('FHFSK', 'BFSK')
p(1).LineWidth = 1.5;
p(2).LineWidth = 1.5;

% % 10 m acoustic tank BFSK test
% data = [[100 10],
%         [99.5 20],
%         [100 40],
%          [100 50],
%          [90.5 80],
%          [89 100]]
%      
%  data(1:end, 1)
% figure
% 
% x2 = data(1:end,2)
% y2 = 100 - data(1:end,1)
% p = plot(x,y, '-o', 'MarkerIndices',1:length(y))
% title('BFSK BER vs Baud Rate at 10m in Acoustic Tank')
% xlabel('Baud Rate')
% ylabel('Bit Error Rate (%)')
% p(1).LineWidth = 1.5;