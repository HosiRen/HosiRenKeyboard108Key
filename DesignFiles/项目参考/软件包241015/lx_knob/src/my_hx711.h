#ifndef _HX711_H
#define _HX711_H

void hx711_init(void);

const int LOADCELL_DOUT_PIN = 40;
const int LOADCELL_SCK_PIN = 39;

#define STRAIN_MIN (-269000)
#define STRAIN_MAX (500000)
#endif // _MY_GUI_H
