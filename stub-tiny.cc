// h8300-elf-gcc -mh -mn -O -nodefaultlibs -nostdlib -nostartfiles -Tstub.x stub.cc -o stub.elf

asm(" .section .text");
asm(" .global _start");
asm("_start:");
asm(" mov.w #_end, sp");
asm(" jsr @_main:16");
asm("_exit:");
asm(" bra _exit");

struct BIT {
    unsigned char   b7:1;       /* bit7 */
    unsigned char   b6:1;       /* bit6 */
    unsigned char   b5:1;       /* bit5 */
    unsigned char   b4:1;       /* bit4 */
    unsigned char   b3:1;       /* bit3 */
    unsigned char   b2:1;       /* bit2 */
    unsigned char   b1:1;       /* bit1 */
    unsigned char   b0:1;       /* bit0 */
};

#define FLMCR1      (*(volatile unsigned char *)0xFF90)     /* Flash Memory Control Register 1  */
#define FLMCR1_BIT  (*(volatile struct BIT *)0xFF90)        /* Flash Memory Control Register 1  */
#define SWE         FLMCR1_BIT.b6                           /* Software Write Enable            */
#define ESU         FLMCR1_BIT.b5                           /* Erase Setup                      */
#define PSU         FLMCR1_BIT.b4                           /* Program Setup                    */
#define EV          FLMCR1_BIT.b3                           /* Erase Verify                     */
#define PV          FLMCR1_BIT.b2                           /* Program Verify                   */
#define ERS         FLMCR1_BIT.b1                           /* Erase                            */
#define PGM         FLMCR1_BIT.b0                           /* Program                          */
#define FLMCR2      (*(volatile unsigned char *)0xFF91)     /* Flash Memory Control Register 2  */
#define FLMCR2_BIT  (*(volatile struct BIT *)0xFF91)        /* Flash Memory Control Register 2  */
#define FLER        FLMCR2_BIT.b7                           /* Error                            */
#define EBR1        (*(volatile unsigned char *)0xFF93)     /* Erase Block Register             */
#define FENR        (*(volatile unsigned char *)0xFF9B)     /* Flash Memory Enable Register     */
#define FENR_BIT    (*(volatile struct BIT *)0xFF9B)        /* Flash Memory Enable Register     */
#define FLSHE       FENR_BIT.b7                             /* Flash Memory Control Reg Enable  */
#define SMR         (*(volatile unsigned char *)0xFFA8)     /* Serial Mode Register             */
#define BRR         (*(volatile unsigned char *)0xFFA9)     /* Bit Rate Register                */
#define SCR3        (*(volatile unsigned char *)0xFFAA)     /* Serial Control Register 3        */
#define SCR3_BIT    (*(volatile struct BIT *)0xFFAA)        /* Serial Control Register 3        */
#define TE          SCR3_BIT.b5                             /* Transmit Enable                  */
#define RE          SCR3_BIT.b4                             /* Receive Enable                   */
#define CKE1        SCR3_BIT.b1                             /* Clock Enable 1                   */
#define CKE0        SCR3_BIT.b0                             /* Clock Enable 0                   */
#define TDR         (*(volatile unsigned char *)0xFFAB)     /* Transmit Data Register           */
#define SSR         (*(volatile unsigned char *)0xFFAC)     /* Serial Status Register           */
#define SSR_BIT     (*(volatile struct BIT *)0xFFAC)        /* Serial Status Register           */
#define TDRE        SSR_BIT.b7                              /* Transmit Data Register Empty     */
#define RDRF        SSR_BIT.b6                              /* Receive Data Register Full       */
#define OER         SSR_BIT.b5                              /* Overrun Erorr                    */
#define FER         SSR_BIT.b4                              /* Framing Erorr                    */
#define PER         SSR_BIT.b3                              /* Parity Erorr                     */
#define TEND        SSR_BIT.b2                              /* Transmit End                     */
#define RDR         (*(volatile unsigned char *)0xFFAD)     /* Receive data Register            */
#define TCSRWD      (*(volatile unsigned char *)0xFFC0)     /* Timer Control/Status Register W  */
#define TCWD        (*(volatile unsigned char *)0xFFC1)     /* Timer Counter W                  */
#define TMWD        (*(volatile unsigned char *)0xFFC2)     /*                                  */
#define PDR1 (*(volatile unsigned char *)0xFFD4)
#define PDR2 (*(volatile unsigned char *)0xFFD5)
#define PCR1 (*(volatile unsigned char *)0xFFE4)
#define PCR2 (*(volatile unsigned char *)0xFFE5)

extern unsigned char wbuf[256];
extern unsigned char rbuf[128];
extern unsigned char abuf[128];
#if 1
#define STR_1(x) #x
#define STR(x) STR_1(x)
#define usleep_inline(x) usleep_1(x, STR(__LINE__))
#define usleep_1(x, l) do { \
	unsigned __x = (x)*4; \
	asm volatile \
		( \
		  ".lp" l ":\n\t" \
		  "  dec.w #1, %0\n\t" \
		  "  nop\n\t" \
		  "  bne .lp" l "\n\t" \
		  : "=r"(__x): "0"(__x) : "cc" \
		  ); \
} while (0)
#else
#define usleep_inline(x) do { for (int __x=0; (volatile int)__x<x*3; ++__x) asm("nop"); } while (0)
#endif
#define nop() ({ asm volatile ("nop"); })
void usleep(int x) { usleep_inline(x); }
#define EOF ((int)(-1))

#if 0
inline void lcd_wait40() { usleep(40); }
inline void lcd_wait2000() { usleep(2000); }
static void send_to_lcd(unsigned hb, bool rs) {
	PDR1 = (hb<<4) | (rs<<2);
	PDR1|= 0x01;
	nop();
	PDR1&=~0x01;
}

static void lcd_send8(unsigned char b) {
	send_to_lcd(b, false);
	lcd_wait40();
}

static void lcd_send(unsigned char b, bool data = false) {
	send_to_lcd(b>>4, data);
	send_to_lcd(b, data);
	lcd_wait40();
}
static inline void lcd_putc(char c) { lcd_send(c, true); }
static inline void lcd_puts(const char *s) { while (*s) lcd_putc(*s++); }
static inline void lcd_ff() { lcd_send(0x02); lcd_wait2000(); }
static inline void lcd_puthexb(char c) {
	const char *const hex = "0123456789ABCDEF";
	lcd_putc(hex[(c>>4)&0xF]);
	lcd_putc(hex[(c>>0)&0xF]);
}
static void lcd_init() {
	PCR1 = 0b11111111;
	PCR2 = 0b00011000;
	PDR2 = 0b00001000;
	lcd_send8(0x03);
	lcd_wait2000();
	lcd_send8(0x03);
	lcd_wait2000();
	lcd_send8(0x03);
	lcd_wait2000();
	lcd_send8(0x02);
	lcd_wait2000();
	lcd_send(0x2C);
	lcd_send(0x08);
	lcd_send(0x01);
	lcd_wait2000();
	lcd_send(0x02);
	lcd_wait2000();
	lcd_send(0x06);
	lcd_send(0x0C);
}
#endif
inline int recvb() {
    do {
		if (SSR&0x38) return EOF;
    } while (!RDRF);
    return RDR&0xFF;
}
inline void sendb(int c) {
	while (!TDRE);
	TDR = c;
}
int fill_buf() {
	for (int n=0; n<256; ++n) {
		int c = recvb();
		if (c==EOF) return EOF;
		wbuf[n] = c;
	}
	return 0;
}

void pulse(volatile unsigned char *target, const unsigned char *source, int usec) {
	for (int i=0; i<128; ++i) target[i] = source[i];

	TMWD = 0xFF;
	TCSRWD = 0x50;
	TCWD = 0xF0;
	TCSRWD = 0x56;
	
	PSU = 1;
	usleep_inline(50);
	PGM = 1;
	usleep_inline(usec);
	PGM = 0;
	usleep(5);
	PSU = 0;
	usleep(5);

	TCSRWD = 0x52;
}

bool flash(volatile unsigned char *target, const unsigned char *source) {
	SWE = 1;
	usleep(2);
	for (int i=0; i<128; ++i) rbuf[i] = source[i];
	int n;
	for (n=0; n<1000; ++n) {
		pulse(target, rbuf, n<6?30:200);
		PV = 1;
		usleep(4);
		bool matched = true;
		for (int i=0; i<128; ++i) {
			target[i] = 0xFF;
			usleep_inline(2);
			unsigned char t = target[i];
			abuf[i] = rbuf[i] | t;
			rbuf[i] = source[i] | ~t;
			if (source[i]!=t) matched = false;
		}
		PV = 0;
		usleep(2);
		if (n<6) pulse(target, abuf, 10);
		if (matched) break;
	}
	SWE = 0;
	usleep(100);
	return n<1000;
}

int main() {
	TE = 1;
	RE = 1;
	FLSHE = 1;
	usleep(400);
	volatile unsigned char *target = 0;
	sendb('B');
	if (1) {
		int brr = recvb(); // new BRR
		usleep(400);
		TE = RE = 0;
		usleep(400);
		BRR = brr;
		usleep(400);
		TE = RE = 1;
		usleep(400);
		sendb('S'); // START
	}
	const unsigned count = recvb(); // bytes/256
	if ((int)count<0) goto ERROR;
	for (unsigned n=0; n<count; ++n) {
		sendb('A'); // ACK
		if (fill_buf()) goto ERROR;
		if (!flash(target, &wbuf[  0])) { sendb('V'); goto ERROR; }
		target += 128;
		if (!flash(target, &wbuf[128])) { sendb('v'); goto ERROR; }
		target += 128;
		if (FLER) goto ERROR;
	}
	sendb('F'); // FINISH
	return 0;
ERROR:
	sendb('E'); // ERROR
	return 0;
}
unsigned char wbuf[256];
unsigned char rbuf[128];
unsigned char abuf[128];
