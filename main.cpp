#include "mbed.h"
#include "HEPTA_CDH.h"
#include "HEPTA_EPS.h"
#include "HEPTA_SENSOR.h"
HEPTA_CDH cdh(p5, p6, p7, p8, "sd");
HEPTA_EPS eps(p16,p26);
HEPTA_SENSOR sensor(p17,
                  p28,p27,0x19,0x69,0x13,
                  p13, p14,p25,p24);
DigitalOut condition(LED1);
RawSerial gs(USBTX,USBRX,9600);
Timer sattime;
int rcmd = 0,cmdflag = 0; //command variable
 
//getting command and flag
void commandget()
{
    rcmd=gs.getc();
    cmdflag = 1;
}
//interrupting 
void receive(int rcmd, int cmdflag)
{
    gs.attach(commandget,Serial::RxIrq);
}
//initialzing
void initialize()
{
    rcmd = 0;
    cmdflag = 0;
    condition = 0;
}

int main() {
    gs.printf("From Sat : Operation Start...\r\n");
    int flag = 0;      // condition
    float batvol,temp; //Voltage, Temerature
    float ax,ay,az;
    float gx,gy,gz;
    sattime.start();
    receive(rcmd,cmdflag); //interupting by ground station command
    eps.turn_on_regulator();//turn on 3.3V conveter
    for(int i = 0; i < 100; i++) {
        //satellite condition led
        condition = !condition;
        
        //senssing HK data(dummy data)
        eps.vol(&batvol);
        sensor.temp_sense(&temp);
        
        //Transmitting HK data to Ground Station(GS)
        gs.printf("HEPTASAT::Condition = %d, Time = %f [s], batVol = %.2f [V],Temp = %.2f [C]\r\n",flag,sattime.read(),batvol,temp);
        wait_ms(1000);
                
        //Power Saving Mode 
        if((batvol <= 3.5)  | (temp > 35.0)){
            eps.shut_down_regulator();
            gs.printf("Power saving mode ON\r\n"); 
            flag = 1;
        } else if((flag == 1) & (batvol > 3.7) & (temp <= 25.0)) {
            eps.turn_on_regulator();
            gs.printf("Power saving mode OFF\r\n");
            flag = 0;
        }
        
        //Contents of command
        if (cmdflag == 1) {
            if(rcmd == 'a'){
                for(int j=0;j<5;j++){
                    gs.printf("Hello World!\r\n");
                    condition = 1;
                    wait_ms(1000);
                }
            }else if(rcmd == 'b') {
                char str[100];
                mkdir("/sd/mydir", 0777);
                FILE *fp = fopen("/sd/mydir/satdata.txt","w");
                if(fp == NULL) {
                    error("Could not open file for write\r\n");
                }
                for(int i = 0; i < 10; i++) {
                    eps.vol(&batvol);
                    fprintf(fp,"%f\r\n",batvol);
                    condition = 1;
                    wait_ms(1000);
                }
                fclose(fp);
                fp = fopen("/sd/mydir/satdata.txt","r");
                for(int i = 0; i < 10; i++) {
                    fgets(str,100,fp);
                    gs.puts(str);
                }
                fclose(fp);
            }else if(rcmd == 'c'){
                gs.printf("Hepta-Sat Lite Uplink Ok\r\n");
                float ax,ay,az;
                for(int i = 0; i<10; i++) {
                    sensor.sen_acc(&ax,&ay,&az);
                    gs.printf("acc : %f,%f,%f\r\n",ax,ay,az);
                    wait_ms(1000);
                }
            }else if(rcmd == 'd'){ 
                gs.printf("Hepta-Sat Lite Uplink Ok\r\n");
                float gx,gy,gz;
                for(int i = 0; i<10; i++) {
                    sensor.sen_gyro(&gx,&gy,&gz);
                    gs.printf("gyro: %f,%f,%f\r\n",gx,gy,gz);
                    wait_ms(1000);
                }
            }
            initialize();
        }
    }
    sattime.stop();
    gs.printf("From Sat : Operation Stop...\r\n");
}
