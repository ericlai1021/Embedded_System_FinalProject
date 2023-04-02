#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define CLOCK_DELAY 5
#define DEFAULT_BRIGHTNESS {1,0,0,0,1,0,0,1} //0x88+1
#define AUTO_MODE {0,1,0,0,0,0,0,0} //0x40  (Command: auto increment mode)
#define FIXED_MODE {0,1,0,0,0,1,0,0} //0x44 (Command: fixed position mode)
#define START_ADDR {1,1,0,0,0,0,0,0}  //0xC0 (Command to set the starting position at first digit)

int Digits[16][8] = {
	//H,G,F,E,D,C,B,A
	{0,0,1,1,1,1,1,1}, //0
	{0,0,0,0,0,1,1,0}, //1
	{0,1,0,1,1,0,1,1}, //2
	{0,1,0,0,1,1,1,1}, //3
	{0,1,1,0,0,1,1,0}, //4
	{0,1,1,0,1,1,0,1}, //5
	{0,1,1,1,1,1,0,1}, //6
	{0,0,0,0,0,1,1,1}, //7
	{0,1,1,1,1,1,1,1}, //8
	{0,1,1,0,1,1,1,1}, //9
	{0,1,1,1,0,1,1,1}, //A
	{0,1,1,1,1,1,0,0}, //b
	{0,0,1,1,1,0,0,1}, //C
	{0,1,0,1,1,1,1,0}, //d
	{0,1,1,1,1,0,0,1}, //E
	{0,1,1,1,0,0,0,1}  //F
};
int clearbit[8] = {0,0,0,0,0,0,0,0};

FILE* clk = NULL;
FILE* dio = NULL;

void writeDIO(int v) {
	/*Write the GPIO file*/
	dio = fopen("/sys/class/gpio/gpio24/value", "w");
	fprintf(dio, "%d", v);
	fclose(dio);
}
void writeCLK(int v) {
	/*Write the GPIO file*/
	clk = fopen("/sys/class/gpio/gpio25/value", "w");
	fprintf(clk, "%d", v);
	fclose(clk);
}

void initialize(){  
	/*Setup GPIO files*/
	clk = fopen("/sys/class/gpio/export", "w");
	fprintf(clk, "%d", 25);
	fclose(clk);

	dio = fopen("/sys/class/gpio/export", "w");
	fprintf(dio, "%d", 24);
	fclose(dio);

	clk = fopen("/sys/class/gpio/gpio25/direction", "w");
	fprintf(clk, "out");
	fclose(clk);

	dio = fopen("/sys/class/gpio/gpio24/direction", "w");
	fprintf(dio, "out");
	fclose(dio);

	writeCLK(0);    //CLK low
	writeDIO(0);    //DIO low
}

void end() {
	/*Close GPIO files*/
	clk = fopen("/sys/class/gpio/unexport", "w");
	fprintf(clk, "%d", 25);
	fclose(clk);

	dio = fopen("/sys/class/gpio/unexport", "w");
	fprintf(dio, "%d", 24);
	fclose(dio);
}

void startWrite(){
	//send start signal to TM1637
	writeCLK(1);    //CLK high
	writeDIO(1);    //DIO high
	usleep(CLOCK_DELAY);
	writeDIO(0);    //DIO low
}

void stopWrite() {
	//send stop signal to TM1637
	writeCLK(0);    //CLK low
	usleep(CLOCK_DELAY);
	writeDIO(0);    //DIO low
	usleep(CLOCK_DELAY);
	writeCLK(1);    //CLK high
	usleep(CLOCK_DELAY);
	writeDIO(1);    //DIO high
}

void writeByte(int Byte[8]){
	int i;
	for(i = 7; i >= 0; i--){        
		writeCLK(0);    //CLK low
		//Set data bit                                 //H,G,F,E,D,C,B,A                    
		if (Byte[i] == 1)       //Example: 數字3,則Byte={0,1,0,0,1,1,1,1} 從A開始寫
			writeDIO(1);
		else writeDIO(0);
		usleep(CLOCK_DELAY);
		writeCLK(1);    //CLK high
		usleep(CLOCK_DELAY);
}
/*Waiting for ACK 
  每次輸入完8個bits DIO會回傳一個ACK，我們下面不讀取ACK，單純等待ACK完成*/
writeCLK(0);    //CLK low
usleep(CLOCK_DELAY);
writeCLK(1);    //CLK high
usleep(CLOCK_DELAY);
writeCLK(0);    //CLK low
usleep(CLOCK_DELAY);
}

void showNum(int num, int flag) {
	if (num >= 0 && num <= 9999) {
		int mode[8] = AUTO_MODE;
		int addr[8] = START_ADDR;
		int bright[8] = DEFAULT_BRIGHTNESS;

		startWrite(); // 輸入前要先給開始訊號
		writeByte(mode); //mode = {0,1,0,0,0,0,0,0} (0x40 Command: auto increment mode 連續輸入模式)
		stopWrite();

		startWrite();
		writeByte(addr); //addr = {1,1,0,0,0,0,0,0} (0xC0 這道命令會將起始位置 設在顯示器左邊第一位)
		writeByte(Digits[num / 1000]);      // 從最大位數(最左邊一位)開始連續輸入
		if(flag){
			int *time;
			time = Digits[num/100 %10];
			*time = 1;
			writeByte(time);
		}
		else{
			writeByte(Digits[num / 100 % 10]);
		}
		writeByte(Digits[num / 10 % 10]);
		writeByte(Digits[num % 10]);
		stopWrite();

		startWrite();
		writeByte(bright); //bright = {1,0,0,0,1,0,0,1} (0x88+1 Set brightness)   
		stopWrite();
	}
	else {
		printf("Error: Exceed Limit\n");
	}
}

void clear(){
	/*清空七段顯示器(不亮任何東西)*/
	int mode[8] = AUTO_MODE;
	int addr[8] = START_ADDR;
	int bright[8] = DEFAULT_BRIGHTNESS;

	startWrite(); //輸入前要先給開始訊號
	writeByte(mode); //mode = {0,1,0,0,0,0,0,0} (0x40 Command: auto increment mode 連續輸入模式)
	stopWrite();

	startWrite();
	writeByte(addr); //addr = {1,1,0,0,0,0,0,0} (0xC0 這道命令會將起始位置 設在顯示器左邊第一位)
	writeByte(clearbit);      // 從最大位數(最左邊一位)開始連續輸入
	writeByte(clearbit);
	writeByte(clearbit);
	writeByte(clearbit);
	stopWrite();

	startWrite();
	writeByte(bright); //bright = {1,0,0,0,1,0,0,1} (0x88+1 Set brightness)   
	stopWrite();
}

void show_signal(int s){
	FILE* p;

	p = fopen("/sys/class/gpio/gpio4/value", "w");
	fprintf(p, "%d", s);
	fclose(p);
}

void activate_signal(){
	FILE *p;

	p = fopen("/sys/class/gpio/export", "w");
	fprintf(p, "%d", 4);
	fclose(p);

	p = fopen("/sys/class/gpio/gpio4/direction","w");
	fprintf(p, "out");
	fclose(p);
}

void deactivate_signal(){
	FILE* p;

	p = fopen("/sys/class/gpio/unexport","w");
	fprintf(p, "%d", 4);
	fclose(p);
}

int main(void)
{
	time_t timep;
	struct tm *p;
	int i, mode, num, cnt;
	int ctrl = 1;
	initialize();
	activate_signal();

	while(ctrl){
		printf("Please type a mode:\n(1) show the time info. in hour:minute\n(2) show the time info. in minute:second\n(3) set the alarm\n(4) set the countdown timmer.\n");
		scanf("%d", &mode);

		time(&timep);
		p = localtime(&timep);
		if(mode == 1){
			printf("Current time:\t%d:%d\n", p->tm_hour, p->tm_min);
			num = p->tm_hour*100 + p->tm_min;
			showNum(num, 1);
		}
		else if(mode == 2){
			printf("Current time:\t%d:%d\n", p->tm_min, p->tm_sec);
			num = p->tm_min*100 + p->tm_sec;
			showNum(num, 1);
		}
		else if(mode == 3){
			int hour, min, tmp;
			printf("Please set the time. (hour:minute)\n");
			scanf("%d:%d", &hour, &min);
			printf("set the alarm at: %d:%d\n", hour, min);
			showNum(hour*100 + min, 1);

			show_signal(0);
			while(hour!=p->tm_hour || min!=p->tm_min){
				time(&timep);
				p = localtime(&timep);
			}
			show_signal(1);
			printf("Type anything to close the alarm.\n");
			tmp = getchar();
			tmp = getchar();
			show_signal(0);
		}
		else if(mode == 4){
			printf("Type the time to countdown(in sec.)\n");
			scanf("%d", &cnt);
			for(i=cnt; i>0; i--){
				if(i < 5){
					showNum(i, 0);
					show_signal(1);
					usleep(300000);
					show_signal(0);
					usleep(200000);
					show_signal(1);
					usleep(300000);
					show_signal(0);
					usleep(200000);
				}
				else{
					show_signal(1);
					showNum(i, 0);
					usleep(800000);
					show_signal(0);
					usleep(200000);
				}
			}
			showNum(0, 0);
			show_signal(1);

		}
		printf("Type 0 to close the program.\nType 1 to continue.\n");
		scanf("%d", &ctrl);
		if(ctrl == 0){
			break;
		}
		else{
			show_signal(0);
			clear();
		}
	}
	show_signal(0);
	clear();
	end();
	deactivate_signal();

	exit(0);
}
