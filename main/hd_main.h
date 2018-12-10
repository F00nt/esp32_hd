/*
esp32_hd
Copyright (c) 2018 ys1797 (yuri@rus.net)

License (MIT license):
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

extern char I2C_detect[128];		/* ������ ������������ ��������� �� ���� i2c */

// ������ ������
typedef enum {
        MODE_IDLE=0,		// ����� �����������
        MODE_POWEERREG=1,	// ����� ���������� ��������
	MODE_DISTIL=2,		// ����� �����������
	MODE_RECTIFICATION=3,	// ����� ������������
	MODE_TESTKLP=10,	// ����� ������������ ��������
} main_mode;

// ��������� ���������
typedef enum {
	NO_ALARM    =0x0,		// ��� ������
	ALARM_TEMP  =0x2,		// ������ �� ���������� �����������
	ALARM_WATER =0x4,		// ������: ���������� ���� ����������
	ALARM_FREQ  =0x8, 		// ������: ���������� ���������� ����
	ALARM_NOLOAD=0x10,		// ������: ���������� ����������� ��������
} alarm_mode;

#define START_WAIT	0	// �������� ������� ��������
#define PROC_START	1	// ������ ��������
#define PROC_RAZGON	2	// ������ �� ������� �����������
#define PROC_STAB	3	// ������������ �����������
#define PROC_GLV	4	// ����� �������� �������
#define PROC_T_WAIT	5       // �������� ������������ �����������
#define PROC_SR		6	// ����� ��
#define PROC_DISTILL	PROC_SR	// �����������
#define PROC_HV		7	// ����� ��������� �������
#define PROC_WAITEND	8	// ���������� �������, ������ ���� ��� ����������
#define PROC_END	100	// ��������� ������


extern char *Hostname;		// ��� �����
extern char *httpUser;		// ��� ������������ ��� http
extern char *httpPassword;	// ������ ��� http
extern int httpSecure;		// ���������� ������
extern char *smscUser;		// ��� ������������ ��� smsc
extern char *smscHash;		// ��� ������ smsc
extern char *smscPhones;	// ����� �������� ��� Smsc
extern int useSmsc;		// ������������ smsc


extern main_mode MainMode;	// ������� ����� ������
extern alarm_mode AlarmMode;	// ��������� ������
extern int16_t MainStatus;	// ������� ��������� (� ����������� �� ������)
extern int16_t CurPower;	// ������� ���������� ��������
extern int16_t SetPower;	// ������������� ��������
extern int16_t CurVolts;	// ������� ���������� ����������
extern volatile int16_t CurFreq;// ���������� ����� �������� � ������� ��������� ����������
extern int16_t WaterOn;		// ���� ��������� ������� ����������
extern float TempWaterIn;	// ����������� ���� �� ����� � ������
extern float TempWaterOut;	// ����������� ���� �� ������ �� �������
extern int16_t WaterFlow;	// �������� ������� ������ ����.




// PZEM command and responce
struct PZEMCommand {
	uint8_t command;
	uint8_t addr[4];
	uint8_t data;
	uint8_t crc;
};

#define PZEM_VOLTAGE (uint8_t)0xB0
#define RESP_VOLTAGE (uint8_t)0xA0
#define PZEM_CURRENT (uint8_t)0xB1
#define RESP_CURRENT (uint8_t)0xA1
#define PZEM_POWER   (uint8_t)0xB2
#define RESP_POWER   (uint8_t)0xA2
#define PZEM_ENERGY  (uint8_t)0xB3
#define RESP_ENERGY  (uint8_t)0xA3
#define PZEM_SET_ADDRESS (uint8_t)0xB4
#define RESP_SET_ADDRESS (uint8_t)0xA4
#define PZEM_POWER_ALARM (uint8_t)0xB5
#define RESP_POWER_ALARM (uint8_t)0xA5

#define PZEM_RESPONSE_SIZE sizeof(struct PZEMCommand)
#define PZEM_RESPONSE_DATA_SIZE PZEM_RESPONSE_SIZE - 2

#define PZEM_BAUD_RATE 9600
#define PZEM_DEFAULT_READ_TIMEOUT 1000

#define PZEM_ERROR_VALUE -1.0


#define TICK_RATE_HZ 100
#define TICK_PERIOD_MS (1000 / TICK_RATE_HZ)
#define LED_HZ 50
// TRIAC is kept high for TRIAC_GATE_IMPULSE_CYCLES PWM counts before setting low.
#define TRIAC_GATE_IMPULSE_CYCLES 10
// TRIAC is always set low at least TRIAC_GATE_QUIESCE_CYCLES PWM counts 
// before the next zero crossing.
#define TRIAC_GATE_QUIESCE_CYCLES 10
#define TRIAC_GATE_MAX_CYCLES 55

#define HMAX (1<<LEDC_TIMER_10_BIT)-1

// ��� ����������
enum variable_type {
        VARIABLE_CHECKBOX,
        VARIABLE_STRING,
        VARIABLE_INT,
	VARIABLE_FLOAT
};


typedef struct  {
        char *name;
        enum variable_type type;
        int min;
        int max;
} vaiable_list;

// ����������� ��� ������ � ���������
typedef struct  {
	bool is_pwm;			// ����, ����������, ��� ������  � ������ ���.
	float open_time;		// ����� ��� �������� ������� � �������� (� ������ ���)
	float close_time;		// ����� ��� ��������� ��������� ������� � �������� (� ������ ���)
	float timer_sec;		// ������ ������ ��� �������� ��������� �������. (� ������ ���)
	bool is_open;			// ����, ��� ������ � �������� ���������
	int channel;			// ����� ledc
} klp_list;

extern klp_list Klp[MAX_KLP];		// ������ ��������.
#define klp_water 0			// ������ ������ ����.
#define klp_glwhq 1			// ������ ������ ����� � �������.
#define klp_sr    2			// ������ ������ ��������� ������


void myBeep(bool lng);		// �������� �����


float PZEM_setAddress(void);
float PZEM_voltage(void);
float PZEM_current(void);
float PZEM_power(void);
float PZEM_energy(void);


const char *getMainModeStr(void);
const char *getAlarmModeStr(void);
const char *getMainStatusStr(void);
cJSON* getInformation(void);

int param_default(void);	// ����� ���������� � �������� �� ���������
void sendSMS(char *text);	// �������� SMS
void Rectification(void);	// ��������� ��������� � ������ ������������
void setPower(int16_t pw);	// ��������� ������� ��������
void setMainMode(int new_mode);	// ��������� ������ ������ ������
void setStatus(int next);	// ������ ��������� ��������� ��������� ��������
void closeAllKlp(void);		// �������� ���� ��������.
void openKlp(int i);		// �������� ������� ����
void closeKlp(int i);		// �������� ������������� �������
void startKlpPwm(int i, float topen, float tclose); // ������ ���� �������
void startGlvKlp(float topen, float tclose); // ������ ���� ������ �����
void startSrKlp(float topen, float tclose);	// ������ ���� ������ ��������� ��������


int hd_httpd_init(void);	// ������ http �������