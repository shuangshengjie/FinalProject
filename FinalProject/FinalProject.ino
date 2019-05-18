/*
 Name:		FinalProject.ino
 Created:	4/27/2019 9:17:20 PM
 Author:	Gaoxz
*/
 /*Android通信串口:
PA_4:RX;    PA_5:TX;
FPGA通信端口:
PQ_3:signal_High;   PQ_2:signal_Low;
*/


#include "TCS34725.h"
#include "VL6180X.h"

char buffer;	//从串口中读取到的数据
VL6180X distance(1);//设置一个距离对象
TCS34725 rgb(0);	//设置一个颜色对象
int colorJudged[3];	//颜色传感器读取到的颜色数据
int DISTANCE_LOW = 16;	//用户所在范围的最近距离
int DISTANCE_HIGH = 25;	//用户所在范围的最远距离
int userId = 0;
int TIME_LIMITED = 50;
int score;

char userGroup[15] = {
	'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O'
};

//初始化
void setup() {
	//与Android上位机通信的串口初始化
	Serial3.begin(115200);
	Serial3.flush();
	//打开与电脑通信的串口来进行检验
	Serial.begin(115200);
	Serial.flush();
	//颜色传感器和距离传感器的初始化
	rgb.Init();
	distance.Init();
	//红外距离传感器的输入引脚		胜负判断引脚
	pinMode(PE_4, INPUT);
	//与FPGA进行并行串口通信的端口 
	pinMode(PQ_3, OUTPUT);//HIGH
	pinMode(PP_3, OUTPUT);
	pinMode(PQ_1, OUTPUT);//LOW

	//读取用户ID
	pinMode(PL_0,INPUT);//HIGH
	pinMode(PL_1,INPUT);
	pinMode(PL_2, INPUT);
	pinMode(PL_3, INPUT);//LOW

	pinMode(D1_LED, OUTPUT);
	RESETON();
}
// 主循环
void loop() {
	JudgeId();//判断用户ID及维护模式
	stayHere();//判断用户距离
	delay(2000);
}
//读取用户ID
void JudgeId() {
	int button1;
	int button2;
	int button3;
	int button4;
	int userIdGet1;
	int userIdGet2;
	//Get the User ID
	while (1) {
		
		if (Serial3.available() > 0) {
			if (Serial3.read() == 'b') {
				debugMode1();
			}
			else if (Serial3.read() == 'c') {
				debugMode2();
			}
		}
		Serial3.flush();

		button1 = digitalRead(PL_0);
		button2 = digitalRead(PL_1);
		button3 = digitalRead(PL_2);
		button4 = digitalRead(PL_3);

		userIdGet1 = button1 * 8 + button2 * 4 + button3 * 2 + button4 * 1;
		delay(300);

		button1 = digitalRead(PL_0);
		button2 = digitalRead(PL_1);
		button3 = digitalRead(PL_2);
		button4 = digitalRead(PL_3);

		userIdGet2 = button1 * 8 + button2 * 4 + button3 * 2 + button4 * 1;
		if (userIdGet1 == userIdGet2) {
			if (userIdGet1 != 0) {
				break;
			}
		}

		
	}
	userId = userIdGet1;
	//Send the User ID
	while (1) {
		if (Serial3.available() > 0) {
			if (Serial3.read() == 's') {
				break;
			}
		}
		Serial3.print(userGroup[userId-1]);
		Serial3.flush();
		delay(200);
	}
}

//正常的游戏模式
void NormalMode() {
	delay(3000);//根据最后的实际情况进行更改
	colorFinalCheck();
	while (1) {
		if (Serial3.available() > 0) {
			if (Serial3.read() == 't') {
				break;
			}
		}
		delay(10);
		Serial3.flush();
	}
	colorSendtoFPGA();	//颜色判断&&控制舵机活动模式
	sendDataToAndroid();//发送输赢数据给安卓
	//根据上位机的返还数据决定是否结束游戏
	while (1) {
		if (Serial3.available() > 0) {
			if (Serial3.read() == 'e') {
				break;
			}
			else if (Serial3.read() == 'r') {
				NormalMode();
				break;
			}
		}
	}
}

//颜色识别程序			未完		实际数据采样
void colorSendtoFPGA() {
	if (colorJudged[0] && colorJudged[1] && colorJudged[2]) {
		setGreen();
	}
	else if (colorJudged[0] && colorJudged[1] && colorJudged[2]) {
		setRed();
	}
	else if (colorJudged[0] && colorJudged[1] && colorJudged[2]) {
		setBlue();
	}
}

void colorFinalCheck() {
	int* color1 = getColor2();
	int* color2 = getColor2();
	int* color3 = getColor2();
	int* color4 = getColor2();
	int* color5 = getColor2();
	int* coloraverage = average(color1, color2, color3, color4, color5);
	colorJudged[0] = coloraverage[0];
	colorJudged[1] = coloraverage[1];
	colorJudged[2] = coloraverage[2];
}
int* getColor2(){
	int* color1 = getColor1();
	int* color2 = getColor1();
	int* color3 = getColor1();
	int* color4 = getColor1();
	int* color5 = getColor1();
	int* coloraverage = average(color1, color2, color3, color4, color5);
	return coloraverage;
}
int* getColor1() {
	int color[3];
	color[0] = rgb.getGreenData();
	color[1] = rgb.getBlueData();
	color[2] = rgb.getRedData();
	delay(100);
	return color;
}
//对采集到的颜色数据进行平均化处理
int* average(int* color1, int* color2, int* color3, int* color4, int* color5) {
	int color[3];
	for (int i = 0; i < 3; i++) {
		color[i] = color1[i] + color2[i] + color3[i] + color4[i] + color5[i];
		color[i] /= 5;
	}
	return color;
}
//距离识别模块
void stayHere() {
	int distance;
	while (1) {
		Serial3.flush();
		distance = distanceGet();
		if (distance > DISTANCE_HIGH || distance < DISTANCE_LOW) {
			Serial3.print('n');
			Serial3.flush();
		}
		else if (distance < DISTANCE_HIGH && distance > DISTANCE_LOW) {
			Serial3.flush();
			break;
		}
	}
}
int distanceGet() {
	int distanceget1 = distance.readRangeSingle();
	delay(200);
	int distanceget2 = distance.readRangeSingle();
	int distanceget = (distanceget1 + distanceget2) / 2;
	return distanceget;
}
//胜负判断部分
void sendDataToAndroid() {
	int single;
	single = winORlose();
	while (1) {
		Serial3.print(single);
		Serial3.flush();
		buffer = Serial3.read();
		if (buffer == 's') {
			break;
		}
		delay(500);
	}
}
//更改为积分制游戏		根据最后讨论的游戏时间进行调整
int winORlose() {
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 100; j++) {
			if (digitalRead(PE_4) == 0) {
				score = ((1000-i*j) / 1000) * 100;
				return score;
			}
			delay(10);
		}
	}
	score = 0;
	return score;
}
//调试模式1：将信号发送给下位机使其逐个调用电机。
void debugMode1() {
	RESETOFF();
	delay(2000);
	RESETON();
}
//调试模式2:将传感器读取到的数据传输给上位机
void debugMode2() {
	Serial3.print("R:");
	Serial3.print(rgb.getRedData());
	Serial3.print("\tG:");
	Serial3.print(rgb.getGreenData());
	Serial3.print("\tB:");
	Serial3.print(rgb.getBlueData());
	Serial3.print("\tC:");
	Serial3.println(rgb.getClearData());
	Serial3.print("Distance:");
	Serial3.println(distance.readRangeSingle());
}
//传输数据成功后亮灯提示
void blink() {
	digitalWrite(D1_LED, HIGH);
	delay(200);
	digitalWrite(D1_LED, LOW);
	delay(200);
}
//和FPGA进行并行串口通信的四种基础模式		更改
//设置FPGA的输出模式为绿色
void setGreen() {
	digitalWrite(PQ_3, LOW);
	digitalWrite(PP_3, HIGH);
	digitalWrite(PQ_1, LOW);
}
//设置FPGA的输出模式为蓝色
void setBlue() {
	digitalWrite(PQ_3, LOW);
	digitalWrite(PP_3, HIGH);
	digitalWrite(PQ_1, HIGH);
}
//设置FPGA的输出模式为红色
void setRed() {
	digitalWrite(PQ_3, LOW);
	digitalWrite(PP_3, LOW);
	digitalWrite(PQ_1, HIGH);
}
//将FPGA置为初始化模式
void RESETON() {
	digitalWrite(PQ_3, LOW);
	digitalWrite(PP_3, LOW);
	digitalWrite(PQ_1, LOW);
}
//将FPGA置为调试模式中舵机全部抬起的状态
void RESETOFF() {
	digitalWrite(PQ_3, HIGH);
	digitalWrite(PP_3, LOW);
	digitalWrite(PQ_1, LOW);
}