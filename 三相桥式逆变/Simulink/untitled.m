f=50; %Hz
T = 1/f; %s   
V_input = 550; %V
A= 1;
Pulse = 50; %每个桥臂导通角度180
%上桥臂IGBT延迟时间(每个IGBT延时120导通)
pwm1_delay = 0;          
pwm3_delay = T*(120/360);%120
pwm5_delay = T*(-120/360);%-120，240导致第一个周期不导通
%下桥臂IGBT延迟时间
pwm2_delay = T*(60/360);  %60
pwm4_delay = T*(180/360); %180
pwm6_delay = T*(-60/360); %-60，300导致第一个周期不导通

L_val = 480e-3;
C_val = 48e-3;
R_val = 0.22;