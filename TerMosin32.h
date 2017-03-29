
#define BIT	_BV

#define PINLED	PINB
#define PORTLED	PORTB
#define	DDRLED	DDRB
#define	PORTZN	PORTC
#define DDRZN	DDRC
#define	ZN0		BIT(0)
#define	ZN1		BIT(1)
#define	ZN2		BIT(2)
#define	ZN3		BIT(3)
#define	BITZN	(ZN0|ZN1|ZN2|ZN3)


#define PORTDRIVE	PORTD
#define DDRDRIVE	DDRD
#define DRIVE_TEN_1	BIT(PD5)
#define DRIVE_COOL	BIT(PD6)

#define PINDRIVE	PIND
#define SW_3_DIG	BIT(PD4)
#define ON_3_DIGIT	((PINDRIVE & SW_3_DIG)==0)
#define SW_COOLING	BIT(PD3)
#define ON_COOLING	((PINDRIVE & SW_COOLING)==0)
#define SW_COM_ANOD	BIT(PD2)
#define ON_COM_ANOD	((PINDRIVE & SW_COM_ANOD)==0)
#define SW_WORK_ON_ERR	BIT(PD7)
#define OFF_SW_WORK_ON_ERR	((PINDRIVE & SW_WORK_ON_ERR)==0)

#define	btn_plus		BIT(2)
#define	btn_minus		BIT(3)
#define	btn_set			BIT(4)


#define	PORT_1wire	PORTC
#define	DDR_1wire	DDRC
#define	PIN_1wire	PINC
#define	pwire0 			BIT(PB4)	//бит DQ DS18B20
#define	pwire1 			BIT(PB5)

struct {
	int16_t temperature;
	uint8_t gisterezis;
	uint8_t ohlagdenie;
	uint8_t GistCooling;
	uint16_t crc;
	}Termostat;	

uint8_t	lcd_buffer[4], znmesto, state_wire,
				ViewName, 
				anti_dr, key, btn_speed, n_btn_sp, inc, 
				out_1wire, temp_lsb[2], temp_msb[2], time_wire, eeprom_eer; 
uint8_t time_flag,  TimeViewp;				
//--------time_flag-----------
#define _viewp		0
#define _termo_disable 1
int16_t Temperatura, TimeActiveEdit;

#define sA	0x01
#define sB	0x02
#define sC	0x04
#define sD	0x08
#define sE	0x10
#define sF	0x20
#define sG	0x40
#define sH	0x80
/*		 -A-
		F	B
		 -G-
		E	C
		 -D-	*/
const uint8_t font[]={
sA|sB|sC|sD|sE|sF,//0
sB|sC,//1
sA|sB|sG|sE|sD,//2
sA|sB|sG|sC|sD,//3
sF|sG|sB|sC,//4
sA|sF|sG|sC|sD,//5
sA|sF|sG|sC|sD|sE,//6
sA|sB|sC,//7
sA|sB|sC|sD|sE|sF|sG,//8
sA|sB|sC|sD|sG|sF//9
};

#define f_E		(sA|sD|sE|sF|sG)	//E
#define f_r		(sE|sG|sH)			//r
#define f_t		(sD|sE|sF|sG)		//t
#define f_d		(sB|sC|sD|sE|sG)	//d
#define f_A		(sA|sB|sC|sE|sF|sG)	//A
#define f_P		(sA|sB|sE|sF|sG)	//P
#define f_y		(sB|sC|sD|sF|sG)	//У
#define f_F		(sA|sE|sF|sG)		//F
#define f_n		(sA|sB|sC|sE|sF)	//П
#define f_U		(sF|sE|sD|sC|sB)	//U
#define f_u		(sE|sD|sC)			//u
#define f_G		(sA|sF|sE)			//Г
#define f_gr	(sA|sB|sF|sG)		//gr
#define f_c		(sG|sD|sE)			//c
#define f_C		(sA|sD|sE|sF)		//C
#define f_h		(sC|sE|sF|sG)		//h
#define f_L		(sD|sE|sF)			//L
#define f_H		(sB|sC|sE|sF|sG)	//H



//-------------EEPROM-----------------
#define EEP_ADR	10
//------------------------------------

