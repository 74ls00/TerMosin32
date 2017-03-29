/*
17.02.2010
Дмитрий Мосин. 
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h> /*F_CPU = 4000000; _delay_ms max = 65,535ms; _delay_us max = 192us */

#include "TerMosin32.h"
/*
4000000 meg внутр
*/


void avr_init(void)
{
	TCCR1B=(1<<CS10)|(1<<WGM12);// ctc mode
	OCR1A=4000*5;//время 5 милисекунд
	
	DDRDRIVE|=DRIVE_TEN_1|DRIVE_COOL;
	PORTDRIVE=SW_3_DIG|SW_COM_ANOD|SW_COOLING|SW_WORK_ON_ERR;
	DDRZN|=BITZN;
	DDRLED=0xFF;
}
//-------------------EEPROM-------------------------
static inline unsigned char EEPROM_read(unsigned int uiAddress)
{
	while(EECR & (1<<EEWE));
	EEAR = uiAddress;
	EECR |= (1<<EERE);
	return EEDR;
}

static void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
	if ( EEPROM_read(uiAddress)!=ucData){
		while(EECR & (1<<EEWE));
		asm("wdr");//сброс сторожевого таймера
		EEAR = uiAddress;
		EEDR = ucData;
		EECR |= (1<<EEMWE);
		EECR |= (1<<EEWE);
		}
}

static void save_struct_eep(void)
{
	uint8_t * a= (uint8_t *)&Termostat;
	Termostat.crc=0;
		for(uint16_t j=0, i=EEP_ADR; j<sizeof(Termostat); i++, j++, a++)
			{
			if (j<sizeof(Termostat)-2) Termostat.crc+=*a;
			EEPROM_write( i, *a);
			}
		eeprom_eer=0;
}

static void read_struct_eep(void)
{
	uint16_t crc=0;
	uint8_t *a= (uint8_t *)&Termostat;
	 	for(uint16_t j=0, i=EEP_ADR; j<sizeof(Termostat); i++, j++, a++)
			{*a=EEPROM_read(i);
			if (j<sizeof(Termostat)-2) crc+=*a;
			}
		if ( crc!= Termostat.crc ) {
			eeprom_eer=1;
			Termostat.temperature=350;
			Termostat.gisterezis=10;
			Termostat.ohlagdenie=0;
			Termostat.GistCooling=10;
			}
}
//*********************************************
//-------------END------EEPROM----------------------

//**********1wire************

void pullup_off(void){
	DDR_1wire&=~pwire0;
	DDR_1wire&=~pwire1;
	PORT_1wire&=~pwire0;
	PORT_1wire&=~pwire1;
}

static unsigned char readbit(void){
unsigned char i;

	DDR_1wire|=out_1wire;//line down
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");
	DDR_1wire&=~out_1wire;//line up 
	_delay_us(10);
	i=PIN_1wire;

	_delay_us(47);
	return i;
}

void readbyte(unsigned char *pw0, unsigned char *pw1) {
unsigned char i,r,p, t1,t2;
pullup_off();
t1=0;
t2=0;
for(i=0, r=1; i<8; i++){
	p=readbit();
	if ( p & pwire0 ) t1|=r;
	if ( p & pwire1 ) t2|=r;
	r<<=1;	
	}
PORT_1wire|=out_1wire;
DDR_1wire|=out_1wire;
*pw0=t1;
*pw1=t2;
}

static void writebit0(void){
	DDR_1wire|=out_1wire;
	_delay_us(60);
	//DDR_1wire&=~out_1wire;
	PORT_1wire|=out_1wire;
	_delay_us(3);
	//DDR_1wire&=~out_1wire;//line up 
	pullup_off();
}

void writebit1(void){
	DDR_1wire|=out_1wire;
	_delay_us(3);
	//DDR_1wire&=~out_1wire;
	PORT_1wire|=out_1wire;
	_delay_us(3);
	pullup_off();
	_delay_us(55);
}

void writebyte(unsigned char byte){
unsigned char i;
	pullup_off();
	for(i=0; i<8; i++){
		if (byte&BIT(0))
			writebit1();
			else
			writebit0();
		byte>>=1;
		}
	PORT_1wire|=out_1wire;
	DDR_1wire |=out_1wire;
}

void detectPresence(void){
unsigned char i;
	pullup_off();
	out_1wire=pwire0|pwire1;
	
	DDR_1wire|=pwire0;
	DDR_1wire|=pwire1;
	_delay_ms(0.48);
	DDR_1wire&=~pwire0;
	DDR_1wire&=~pwire1;
	
	_delay_us(60);
	i=PIN_1wire;
	
	if ( i & pwire0 ) {out_1wire&=~pwire0; PORTDRIVE &= ~DRIVE_TEN_1; PORTDRIVE &= ~DRIVE_COOL;}
	if ( i & pwire1 ) out_1wire&=~pwire1;
	
	if ( out_1wire ){
		_delay_ms(0.42);
		i=PIN_1wire;
		if (!( i & pwire0 )) {out_1wire&=~pwire0; PORTDRIVE &= ~DRIVE_TEN_1; PORTDRIVE &= ~DRIVE_COOL;}
		if (!( i & pwire1 )) out_1wire&=~pwire1;
		}
	if (out_1wire==0) state_wire=0;
	else{
	PORT_1wire|=out_1wire;
	DDR_1wire|=out_1wire;
	}
}

//***************************
static void ds18B20_init_12bit(void){
	detectPresence();
	if (out_1wire){
		writebyte(0xCC);//SKIP ROM [CCh]
		writebyte(0x4E);//WRITE SCRATCHPAD [4Eh]
		writebyte(0);//	TH
		writebyte(0);//	TL
		writebyte(0x7F);// 0x7F - 12 bit 750mSek; 0x3F 10-bit 187.5 ms; 1F-9bit 94msek
		}
}
//*******end***1wire************


static void receive_t(void){

	if ( state_wire==0 )
	{
	state_wire=1;
	detectPresence();
	return;
	}
	
	if ( state_wire==1 )
	{
	writebyte(0xCC);//SKIP ROM [CCh]
	writebyte(0x44);//CONVERT T [44h]
	time_wire=150;
	state_wire=2;
	return;
	}
	
	if ( state_wire==2 )
	{
	if (!time_wire) state_wire=3;
	return;
	}

	if ( state_wire==3 )
	{
	state_wire=4;
	detectPresence();
	return;
	}
	if ( state_wire==4 )
	{
	writebyte(0xCC);//SKIP ROM [CCh]
	writebyte(0xBE);//READ SCRATCHPAD [BEh]
	state_wire=5;
	return;
	}	
	if ( state_wire==5 )
	{
	readbyte(&temp_lsb[0],&temp_lsb[1]);
	readbyte(&temp_msb[0],&temp_msb[1]);
/*
--Охлаждение--
охл до 20 выкл 			среда нагревает
смотрим когда 25 вкл
охл до 20 выкл
--------------
охл до -20 выкл			среда нагревает
смотрим когда -15 вкл
охл до -20 выкл
----Нагрев----------
наг до 20 выкл			среда охлаждает
смотрим когда 15 вкл
наг до 20 выкл
--------------
наг до -20 выкл			среда охлаждает
смотрим когда -25 вкл
наг до -20 выкл
--------------
*/	
	if (eeprom_eer) 
		if ( OFF_SW_WORK_ON_ERR ){
			PORTDRIVE &= ~DRIVE_TEN_1;
			PORTDRIVE &= ~DRIVE_COOL;
			return;
			}
	
	if ((out_1wire & pwire0)==0) return;
	
	int T;
	T=((unsigned int)(temp_msb[0]<<8))|temp_lsb[0];
	T=(T*5)/8;
	Temperatura=T;
	if ( Termostat.ohlagdenie )
		{//охлаждение
			if (T >= (Termostat.temperature+Termostat.gisterezis) )	PORTDRIVE |= DRIVE_TEN_1;//охлаждение
			else
			if (T <= Termostat.temperature)	PORTDRIVE &= ~DRIVE_TEN_1;
			
			if (ON_COOLING)
				if (T <= Termostat.temperature-Termostat.GistCooling)	PORTDRIVE |= DRIVE_COOL;
			if (T >= Termostat.temperature)	PORTDRIVE &= ~DRIVE_COOL;
		}
		else
		{//нагрев
			if (T <= (Termostat.temperature-Termostat.gisterezis) )	PORTDRIVE |= DRIVE_TEN_1;//нагрев
			else
			if (T >= Termostat.temperature)	PORTDRIVE &= ~DRIVE_TEN_1;
			
			if (ON_COOLING)
				if (T >= Termostat.temperature+Termostat.GistCooling)	PORTDRIVE |= DRIVE_COOL;
			if (T <= Termostat.temperature)	PORTDRIVE &= ~DRIVE_COOL;
		}	

	state_wire=0;
	}
}


//-------------------------------------
static void skan_key(void)
{
unsigned char pin;
	key=0;
	//PORTZN &= ~BITZN;
	if (ON_COM_ANOD)
		PORTZN &= ~BITZN;
		else
		PORTZN |= BITZN;	

	DDRLED=BIT(7);
	PORTLED=(unsigned char)~BIT(7);
	_delay_us(25);
	pin=PINLED|BIT(7);
	if ( pin!= 0xFF ){
		TimeActiveEdit=2000;//20sek
		if (++anti_dr==8)
			{
			key= ~pin;
			}
		if (anti_dr==btn_speed){
			anti_dr=0;
			if (n_btn_sp<=50){
				if (++n_btn_sp==2) btn_speed=40;
				if (n_btn_sp==4) 	btn_speed=35;
				if (n_btn_sp==8) 	btn_speed=17;
				if (n_btn_sp>=30) 	btn_speed=8;
				if (n_btn_sp>=50) 	inc=5;
				}
			}
		}
		else{
		btn_speed=200;
		inc=1;
		n_btn_sp=0;
		anti_dr=0;
		}
	DDRLED=0xFF;
}

//-------------------------------------
static void display(void)
{
if (ON_COM_ANOD)
	{
	PORTZN &= ~BITZN;

	if ( znmesto==0 )	PORTZN |= ZN0;
	else
	if ( znmesto==1 )	PORTZN |= ZN1;
	else
	if ( znmesto==2 )	PORTZN |= ZN2;
	else
	if ( znmesto==3 )	PORTZN |= ZN3;
	PORTLED=~lcd_buffer[znmesto];
	}
	else
	{
	PORTZN |= BITZN;
	
	if ( znmesto==0 )	PORTZN &= ~ZN0;
	else
	if ( znmesto==1 )	PORTZN &= ~ZN1;
	else
	if ( znmesto==2 )	PORTZN &= ~ZN2;
	else
	if ( znmesto==3 )	PORTZN &= ~ZN3;
	PORTLED=lcd_buffer[znmesto];
	}
}
//-------------------------------------
static void key_action(void)
{
int16_t Param=0, LimitPlus=200, LimitM=0;
 
if (key){
	if (ViewName && (key&(btn_plus|btn_minus)) )
		{
		if ( time_flag & BIT(_viewp) ) {time_flag &= ~BIT(_viewp); return;}
		
		if (ViewName==1){
			Param=Termostat.temperature;
			if (ON_3_DIGIT){
				LimitPlus=999;
				LimitM=-99;
				}else{
				LimitPlus=1200;
				LimitM=-550;
				}
			}
			else
			if (ViewName==2){
			Param=Termostat.gisterezis;
			//LimitPlus=200;
			//LimitM=0;
			}
			else
			if (ViewName==3){
			Param=Termostat.ohlagdenie;
			LimitPlus=1;
			//LimitM=0;
			}
			else
			if (ViewName==4){
			Param=Termostat.GistCooling;
			//LimitPlus=200;
			//LimitM=0;
			}
		
		if ( key & btn_plus ){
			if ((Param+inc-1)<LimitPlus) 
				Param+=inc;
				else
				Param=LimitM;
			}
		if ( key & btn_minus ){
			if ((Param-inc+1)>LimitM) 
				Param-=inc;
				else
				Param=LimitPlus;
			}
		if (ViewName==1){
			Termostat.temperature=Param;
			}
			else
			if (ViewName==2){
			Termostat.gisterezis=Param;
			}
			else
			if (ViewName==3){
			Termostat.ohlagdenie=Param;
			}
			else
			if (ViewName==4){
			Termostat.GistCooling=Param;
			}
		}
	if ( key & btn_set ){
		uint8_t N=4;
		if (ON_COOLING) N=5;
		if (++ViewName==N) ViewName=0;
		save_struct_eep();
		time_flag |= BIT(_viewp);
		TimeViewp=150;
		}
	}
}

static void itoa2(int16_t Val, uint8_t * Buf)
{
const uint16_t step[]={1000,100,10,1};
int16_t Subtract, Value;
uint8_t i, Dig;

	Value=Val;
	for (i=0; i<4; i++)
	{
	Subtract=step[i];
	Dig=0;
	while(Value >= Subtract)
		{
		Dig++;
		Value-=Subtract;
		}
	Buf[i]=Dig;
	}
}

//**************************************
void tempir_in_bcd(int16_t t){
uint8_t znak=0, Dig[4];

	if ( t<0 ){
		t*=-1;
		znak=1;
		}

	itoa2(t, Dig);
		
	if (Dig[1]>0){ 
		lcd_buffer[1]=font[Dig[1]];
		if (znak) 	lcd_buffer[0]=sG;
		}
		else
		if (znak) 	lcd_buffer[1]=sG;
	if ( Dig[0]>0 ){
		lcd_buffer[0]=font[Dig[0]];
		lcd_buffer[1]=font[Dig[1]];
		}
	lcd_buffer[2]=font[Dig[2]]|sH;
	lcd_buffer[3]=font[Dig[3]];

}

static void led_nag_ohl(void){
	if ( Termostat.ohlagdenie ){
		lcd_buffer[1]=font[0];
		lcd_buffer[2]=f_h;
		lcd_buffer[3]=f_L|sH;
		}
		else{
		lcd_buffer[1]=f_H;
		lcd_buffer[2]=f_A;
		lcd_buffer[3]=f_G|sH;
		}
}

//-------------------------------------
static void data_led(void)
{

for(uint8_t i=0;i<4;i++)
	lcd_buffer[i]=0;
	
if (ViewName)
	{
	if ( time_flag & BIT(_viewp) ){
		if (ViewName==1){
			//toC
			lcd_buffer[1]=f_t;
			lcd_buffer[2]=f_gr;
			lcd_buffer[3]=f_C;
			}
			else
			if (ViewName==2){
				//ГUC
			lcd_buffer[1]=f_G;
			lcd_buffer[2]=f_U;
			lcd_buffer[3]=f_C|sH;
			}
			else
			if (ViewName==3){
			lcd_buffer[1]=f_L;
			lcd_buffer[3]=f_H;
			if ( Termostat.ohlagdenie )
				lcd_buffer[1]|=sH;
				else
				lcd_buffer[3]|=sH;
			}
			else
			if (ViewName==4){
			
			if ( Termostat.ohlagdenie ){
				lcd_buffer[1]=sE|sD|sC;
				lcd_buffer[3]=f_L|sH;
				}
				else{
				lcd_buffer[1]=sF|sA|sB;
				lcd_buffer[3]=f_H|sH;
				
				}
			}
		}
		else{
		if (ViewName==1){
			tempir_in_bcd(Termostat.temperature);
			}
			else
			if (ViewName==2){
			tempir_in_bcd(Termostat.gisterezis);
			}
			else
			if (ViewName==3){
			led_nag_ohl();
			}
			if (ViewName==4){
			tempir_in_bcd(Termostat.GistCooling);
			}
		}
	}
	else
	{
	if (eeprom_eer){
		lcd_buffer[1]=f_E;
		lcd_buffer[2]=f_E;
		lcd_buffer[3]=f_P;
		}
		else
		if ( !(pwire0 & out_1wire) ){
			lcd_buffer[0]	=sG;
			lcd_buffer[1]	=sG;
			lcd_buffer[2]	=sG;
			lcd_buffer[3]	=sG;
			}
		else
		tempir_in_bcd(Temperatura);
	}
}
//-------------------------------------

int main(void)
{
uint8_t tim_10ms=0;    
	avr_init();
	ds18B20_init_12bit();
	read_struct_eep();
	
	cli();
	WDTCR=(1<<WDCE)|(1<<WDE);
	WDTCR=(1<<WDP0)|(1<<WDE);//32mSek
	asm("wdr");//сброс сторожевого таймера
	sei();
	
while(1)
{
	if (TIFR & _BV(OCF1A)) // 0.005 sek
	{
	TIFR = _BV(OCF1A);
	asm("wdr");//сброс сторожевого таймера
	
	data_led();
	skan_key();
	key_action();
	display();
	if ( ++znmesto==4 )
		{
		znmesto=0;
		}
	receive_t();

	if ( ++tim_10ms & _BV(0) ) //0.01 sek
		{
		time_wire--;
		if (--TimeViewp==0) time_flag &= ~BIT(_viewp);
		if (--TimeActiveEdit<0) ViewName=0;
		}
	}
	
}

}

