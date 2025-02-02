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

#include <cJSON.h>
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

void foo(int i);

extern char I2C_detect[128];		/* Список обнаруженных устройств на шине i2c */

// Режимы работы
typedef enum {
        MODE_IDLE=0,		// Режим мониторинга
        MODE_POWEERREG=1,	// Режим регулятора мощности
	MODE_DISTIL=2,		// Режим дистилляции
	MODE_RECTIFICATION=3,	// Режим ректификации
	MODE_TESTKLP=10,	// Режим тестирования клапанов
} main_mode;

// Аварийные состояния
typedef enum {
	NO_ALARM    			=0,			// Нет аварии
	ALARM_SENSOR_ERR=(1<<0),// Авария: сбой датчиков температуры
	ALARM_TEMP  			=(1<<1),	// Авария по превышению температуры
	ALARM_WATER 		=(1<<2),	// Авария: отсутствие воды охлаждения
	ALARM_EXT   			=(1<<3), 	// Авария от внешнего источника

	ALARM_FREQ  			=(1<<4), 	// Авария: отсутствие напряжения сети
	ALARM_NOLOAD		=(1<<5),	// Авария: отсутствие подключения нагрузки
	ALARM_OVER_POWER=(1<<6),// Авария: мощность превышает заданную
	ALARM_PZEM_ERR		= (1<<7) // Авария: отказ контроля мощности
} alarm_mode;

typedef enum {
	cmd_open,
	cmd_close
} valve_cmd_t;

typedef struct  {
	uint8_t valve_num;
	valve_cmd_t  cmd;
 } valveCMDmessage_t;


#define START_WAIT		0	// Ожидание запуска процесса
#define PROC_START		1	// Начало процесса
#define PROC_RAZGON	2	// Разгон до рабочей температуры
#define PROC_STAB			3	// Стабилизация температуры
#define PROC_GLV			4	// Отбор головных фракций
#define PROC_T_WAIT		5       // Ожидание стабилизации температуры
#define PROC_SR				6	// Отбор СР
#define PROC_HV			7	// Отбор хвостовых фракций
#define PROC_WAITEND	8	// Отключение нагрева, подача воды для охлаждения
#define PROC_END_			9	// Окончание работы
#define PROC_END			100	// Окончание работы

#define PROC_DISTILL		PROC_SR	// Дистилляция


 typedef struct {
 	uint16_t prevState;
 	uint16_t nextState;
 } state_vector_t;

extern char *Hostname;		// Имя хоста
extern char *httpUser;		// Имя пользователя для http
extern char *httpPassword;	// Пароль для http
extern int httpSecure;		// Спрашивать пароль
extern int wsPeriod;		// Период обновления данных через websocket

extern main_mode MainMode;	// Текущий режим работы
extern alarm_mode AlarmMode;	// Состояние аварии
extern int16_t MainStatus;	// Текущее состояние (в зависимости от режима)
extern int16_t CurPower;	// Текущая измерянная мощность
extern int16_t SetPower;	// Установленная мощность
extern int16_t CurVolts;	// Текущее измеренное напряжение
extern volatile int16_t CurFreq;// Измерянное число периодов в секунду питающего напряжения
extern int16_t WaterOn;		// Флаг включения контура охлаждения
extern float TempWaterIn;	// Температура воды на входе в контур
extern float TempWaterOut;	// Температура воды на выходе из контура
extern int16_t WaterFlow;	// Значения датчика потока воды.
extern int16_t fAlarmSoundOff; //флаг выключения звука аварии

//#define NO_BEEP

#ifndef NO_BEEP
#define GPIO_ON(A)		do {GPIO.out_w1ts = (1 << (A));} while(0)
#define GPIO_OFF(A)  	do {GPIO.out_w1tc = (1 << (A));} while(0)
#else
#define GPIO_ON(A)
#define GPIO_OFF(A)
#endif

#define SEC_TO_TICKS(A) ((uint32_t)((A)*1000 + portTICK_PERIOD_MS/2)/portTICK_PERIOD_MS)

#define TRIAC_CONTROL_LED_FREQ_HZ 50
// TRIAC is kept high for TRIAC_GATE_IMPULSE_CYCLES PWM counts before setting low.
#define TRIAC_GATE_IMPULSE_CYCLES 10

// TRIAC is always set low at least TRIAC_GATE_QUIESCE_CYCLES PWM counts 
// before the next zero crossing.

#define TRIAC_GATE_MAX_CYCLES 55
#define HMAX (1<<LEDC_TIMER_10_BIT)-1
#define MAX_HPOINT_VALUE (HMAX - TRIAC_GATE_MAX_CYCLES)
#define MIN_HPOINT_VALUE 		TRIAC_GATE_MAX_CYCLES

// частота ШИМ клапанов, Гц
#define VALVES_CONTROL_LED_FREQ_HZ 20000

//задержка выставления тревоги "пробитие триака" от момента начала превышения мощности, в секундах
#define TRIAK_ALARM_DELAY_SEC	10
//превышение текущей мощности от установленной, в % от максимальной, при которой триак считается пробитым
#define DELTA_TRIAK_ALARM_PRC  5

#define VALVE_DUTY 1023
#define VALVE_ON_FADE_TIME_MS 100
//#define KEEP_KLP_PWM 30  // процент ШИМ удержания
#define KEEP_KLP_DELAY_MS 100  // задержка начала снижения ШИМ клапана после включения, мс

// Определение для работы с клапанами
typedef struct  {
	bool is_pwm;			// Флаг, означающий, что клапан  в режиме шим.
	float open_time;		// Время для открытия таймера в секундах (в режиме шим)
	float close_time;		// Время для закрытого состояния клапана в секундах (в режиме шим)
	float timer_sec;		// Отсчет секунд для текущего состояния таймера. (в режиме шим)
	bool is_open;			// Флаг, что клапан в открытом состоянии
	int channel;			// Канал ledc
	TaskHandle_t pwm_task_Handle; // задача программного ШИМ
} klp_list;

extern klp_list Klp[MAX_KLP];		// Список клапанов.
#define klp_water 0			// Клапан подачи воды.
#define klp_glwhq 1			// Клапан отбора голов и хвостов.
#define klp_sr    2			// Клапан отбора товарного спирта
#define klp_diff   3			// выключение дифф-автомата

extern volatile int zero_imp_shift;

#define EXISTS_ALARM(A)  (AlarmMode & (A))
#define CLEAR_ALARM(A)  do {AlarmMode &= ~(A);} while(0)
#define SET_ALARM(A)  do {AlarmMode |= (A);} while(0)

void myBeep(bool lng);		// Включаем бипер

void PZEM_init(void);
bool PZEMv30_updateValues(void);
float PZEM_voltage(void);
float PZEM_current(void);
float PZEM_power(void);
float PZEM_energy(void);

const char *getMainModeStr(void);
const char *getAlarmModeStr(void);
const char *getMainStatusStr(void);
const char *getResetReasonStr(void); // Получение строки о причине перезагрузки

cJSON* getInformation(void);
cJSON* json_klp(int i);

void write2log(const char* s);

void Rectification(void);	// Обработка состояний в режиме ректификации
void setPower(int16_t pw);	// Установка рабочей мощности
void setMainMode(int new_mode);	// Установка нового режима работы
void moveStatus(int next);	// изменение состояния конечного автомата, вперед или назад
void set_status(int16_t newStatus);

void setTimezone(int gmt_offset);
void setTempStabSR(double newValue);
void setNewProcChimSR(int16_t newValue);

void closeAllKlp(void);		// Закрытие всех клапанов.
void openKlp(int i);		// Открытие клапана воды
void closeKlp(int i);		// Закрытие определенного клапана
void startKlpPwm(int i, float topen, float tclose); // Запуск шима клапана
int setWaterPWM(int pwm_percent);	//задать в процентах быстрый ШИМ ключа управления насосом воды. Вернет текущий % ШИМ

void start_valve_PWMpercent(int valve_num, float period_sec, float percent_open);

int hd_httpd_init(void);	// Запуск http сервера
int hd_display_init(void);	// Запуск обработчика дисплея

void 		setWaitStr(const char* s);
const 	char *getWaitStr(void);

#ifdef __cplusplus
}
#endif
