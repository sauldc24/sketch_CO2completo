#include <SimpleDHT.h>
#include <U8g2lib.h>
#include <U8x8lib.h>
#include <stdio.h>
#include <RTClib.h>
#include <SD.h>

//variables para seguimiento del tiempo y cálculo de CO2 (los tiempos se inicializan a 1 para evitar división entre cero)
unsigned long int HtoLTime = 0;
unsigned long int LtoHTime = 0;
unsigned long int PWM_H = 0;
unsigned long int PWM_L = 0;
unsigned long int CO2 = 0;

//bandera del timer
bool readyToSave = false;

//variable para la secuencia de pantalla
byte screencase = 0;

//genera los objetos RTC y DHT
RTC_DS1307 rtc;
SimpleDHT22 dht22(9);

//genera el objeto pantalla u8g2
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0);

//String para el manejo de las escrituras a archivo, pantalla y serial
char str[23];
File file;

float humidity = 0;
float temperature = 0;

void setup() {
  //Inicializar los registros de configuración del Timer1
  TCCR1A = 0;
  TCCR1B = 0;
  //Configuración del Prescaler a 1024
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12);
  TCCR1B |= (1 << CS10);
  //Configuración del registro de comparación para generar interrupción cada 2 segundos
  OCR1A = 31250;
  TIMSK1 = (1 << OCIE1A);//Activa la interrupción del timer 1

  //Configurar el pin digital 2 para aceptar las interrupciones del PWM
  pinMode(2, INPUT);
  attachInterrupt(0, interrupt, CHANGE);

  //inicia el puerto serial
  Serial.begin(9600);

//Aquí está la sección de código que permite establecer la hora solo hay que descomentar la línea adentro del if
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  //inicia la pantalla
  u8g2.begin();
  //intenta iniciar la tarjeta SD
  while (!SD.begin(10)) {//si no se logra inicializar la SD
          u8g2.firstPage();  
          do {//imprime en pantalla
           u8g2.setFont(u8g_font_unifont);
           u8g2.drawStr( 20, 18, "Inicializando");
           u8g2.drawStr( 20, 33, "SD");
          } while( u8g2.nextPage() );
          Serial.println("No SD");
          delay(500);//espera antes de volver a intentarlo 
    }
  Serial.println("SD INICIADA");
  u8g2.setFont(u8g_font_unifont);
}

void loop() {
  if (LtoHTime > HtoLTime && PWM_H && PWM_L)
  {
    CO2 = 5000*(PWM_H - 2000)/( PWM_H + PWM_L - 4000);
  }
  if (screencase == 0) {
      u8g2.firstPage();  
      sprintf(str, "%i ppm", CO2);
      do {
        u8g2.drawStr( 20, 20, "Concentracion");
        u8g2.drawStr( 20, 35, "de CO2");
        u8g2.drawStr( 20, 50, str);
      } while( u8g2.nextPage() );          
  }

  else if (screencase == 1) {
    u8g2.firstPage();  
    do {
      u8g2.drawStr( 20, 20, "Humedad: ");
      dtostrf(humidity, 4, 1, str);
      u8g2.drawStr( 90, 20, str);
      dtostrf(temperature, 4, 1, str);
      u8g2.drawStr( 50, 40, str);
    } while( u8g2.nextPage() );
  }

  else if (screencase == 2) {
      DateTime now = rtc.now();
      u8g2.firstPage();  
      do {
        sprintf(str, "%i:%i:%i", now.hour(), now.minute(), now.second());
        u8g2.drawStr( 20, 20, str);
      } while( u8g2.nextPage() );
  }
  
  if (readyToSave)
  {
    dht22.read2(&temperature, &humidity, NULL);
    DateTime now = rtc.now();
    sprintf(str, "%i_%i.txt", now.day(), now.month());
    file = SD.open(str, FILE_WRITE);
    if (file){
    sprintf(str, "%02i/%02i/%i\t%02i:%02i:%02i\t", now.day(), now.month(), now.year(), now.hour(),now.minute(), now.second());
    Serial.print(str);
    file.print(str);
    sprintf(str, "%i\t", CO2);
    Serial.print(str);
    file.print(str);
    dtostrf(temperature, 4, 1, str);
    Serial.print(str);
    Serial.print("\t");
    file.print(str);
    file.print("\t");
    dtostrf(humidity, 4, 1, str);
    Serial.println(str);
    file.println(str);    
    file.close();
    }
    else {
      Serial.println("Error Archivo");  
    }
    readyToSave = false;
  }
}


//ISR's

//interrupción del PWM
void interrupt()
{
  if (digitalRead(2) == HIGH)
  {
    LtoHTime = micros();
    PWM_L = LtoHTime - HtoLTime;
  }
  else
  {
    HtoLTime = micros();
    PWM_H = HtoLTime - LtoHTime;
  }
}

//interrupción para la ejecución del programa
ISR(TIMER1_COMPA_vect)
{
    readyToSave = true;
    if (screencase == 2)
    {
      screencase = 0;
    }
    else
    {++screencase;}
}
