%-----硬件设置-------------------------------------
timerCLK = 1000000;%timer时钟，12MHz/12分频
ARR      = 49;  %重装载值
baseRate = timerCLK / (ARR + 1);%载波频率
%-----正弦波设置------------------------------------
F          = 50;%频率
ampScale   = 10.2;%调幅
N          = round(baseRate / F);%一周波分多少段
stepTime   = 1/F/N;%积分步进值
%-----计算------------------------------------------
CCR  = (1:1:N)';

for i=1:1:N
    area  = double(quad(@(x)sin(2*pi*F*x),(i-1)*stepTime,i*stepTime));
    CCR(i) = round((ampScale*area*ARR / stepTime + ARR) / 2)-25;
end
