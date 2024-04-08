#include <Wire.h> 
#include <TimerOne.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

LiquidCrystal_I2C lcd(0x3f, 16, 2);

#define DHTTYPE DHT11
#define DHTPIN 9

DHT sensor_humedad(DHTPIN, DHTTYPE);

///////////////Variables de control potencia focos///////////////
volatile int i = 0;
volatile boolean cruce_x_cero = false;
int Triac = 3;
int potencia_foco = 0;
int T_int = 100;
/////////////////////////////////////////////////////////////////

///////////////Parametros Encoder///////////////
const int pin_A = 7;               //CLK 
const int pin_B = 6;               //DT
const int boton_encoder  = 4;      //SW
int estado_actual_pin_A;
int ultimo_estado_pin_A;
bool boton_apretado = false;          //Esta variable de retorno en false aegurando que el boton del Encoder aun no se ha oprimido
///////////////////////////////////////////////

///////////////Parametros auxiliares del menu///////////////
int contador = 0; 
double aux_temperatura = 0;
double incrementoTemperatura = 0.1;
double temperatura_set;
int humedad_set;
int dias_set;
int dias_incubando;
int frecuencia_rotacion;
////////////////////////////////////////////////////////////


///////////////Variables para el control PID///////////////
double Temperatura_medida, salida_PID, Temperatura_medida_anterior;
double Kp = 70.0; // Coeficiente proporcional
double Ki = 1.0;   // Coeficiente integral
double Kd = 3.6;   // Coeficiente derivativo
////////////////////////////////////////////////////////////

///////////////Variables para ajuste de potencia///////////////
double maxPotencia = 100.0; // Máxima potencia del foco (0 a 100)
double minPotencia = 0.0;   // Mínima potencia del foco
double ParteIntegral = 0.0;
double prevError = 0.0;
///////////////////////////////////////////////////////////////

///////////////Variables para controlar el tiempo de ejecucion///////////////
unsigned long tiempoUltimo = 0;
const unsigned long intervaloTiempo = 5000; // Intervalo de actualización en milisegundos
unsigned long tiempoDeCorreccion = 0;
const unsigned long intervaloCorreccion = 1000; // Intervalo de corrección en milisegundos
unsigned long tiempoTranscurrido;
unsigned long tiempoDia = 86400000; // Milisegundos en un día (1000 ms/s * 60 s/min * 60 min/h * 24 h/día)
unsigned long tiempoUltimaRotacion = 0;

/////////////////////////////////////////////////////////////////////////////

int contadorMenu = 0;

int incubando = 0;
int ventilador_humedad = 11;
int motor_rotacion = 10;

//////////////////////////////MENU//////////////////////////////

String menu_1[] = {"Incubar Gallina","Incubar Codor.","Incubar Pato","Incubar Custom"};  //Inicializamos nuestro Array con los elementos del menu
int size_menu_1 = sizeof(menu_1) / sizeof(menu_1[0]);                           //Obtenemos el número de elementos ocupados en la matriz. en este caso 6     


String menu_2[] = {"Valor Temp en C","Valor Humedad %","Tiempo en Dias","Rotaciones x dia","Iniciar Incub.","Atras"};
int size_menu_2 = sizeof(menu_2) / sizeof(menu_2[0]);

String linea_lcd_1,linea_lcd_2;           //Lineas del LCD

int posicion = 0;
int nivel_del_menu = 0;             //Iniciamos la variable en el menu principal 0 --> 4
byte icono_flecha[] = {B01000,B00100,B00010,B11111,B11111,B00010,B00100,B01000};      //Creamos un array de 8 posiciones para la icono_flecha del menu

////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////FUNCIONES//////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

void selectOption(){
  if(digitalRead(boton_encoder) == LOW){
    delay(300);
    boton_apretado = true;
  }
}

void fn_menu(int posicion,String menus[],byte sizemenu){
  lcd.clear();
  linea_lcd_1="";
  linea_lcd_2="";
   
  if((posicion % 2) == 0){  
     lcd.setCursor(0, 0);
     lcd.write(byte(0));
     linea_lcd_1 = menus[posicion];
    
    if(posicion+1 != sizemenu){
      linea_lcd_2 = menus[posicion+1];
    }
    
  }else{
    linea_lcd_1 = menus[posicion-1];
    lcd.setCursor(0, 1);
    lcd.write(byte(0));
    linea_lcd_2 = menus[posicion];
  }
  
  lcd.setCursor(1, 0);
  lcd.print(linea_lcd_1);
  lcd.setCursor(1, 1);
  lcd.print(linea_lcd_2);  
}


bool fn_encoder(byte sizemenu){ 
  bool retorno = false;
  estado_actual_pin_A = digitalRead(pin_A); 
  if (estado_actual_pin_A != ultimo_estado_pin_A){     
    if (digitalRead(pin_B) != estado_actual_pin_A){        //DT != CLK     
     contador ++;
     delay(250);     
    }
    else {
     contador --;
     delay(250); 
    }
    if(contador <=0){
      contador = 0;
    }
    if(contador >= sizemenu-1 ){
      contador = sizemenu-1;
    }
    retorno = true;
  } 
  return retorno; 
}

void fn_contador_entero(int muestra){
  contador = muestra;
  estado_actual_pin_A = digitalRead(pin_A); 

  if (estado_actual_pin_A != ultimo_estado_pin_A){     
    if (digitalRead(pin_B) != estado_actual_pin_A){        //DT != CLK  ``````````    
      contador++;
      delay(250);     
    }
    else{
      contador--;
      if(contador<0){
        contador = 0;
      }
      delay(250); 
    } 
  }
}

void fn_contador_temperatura(){
    aux_temperatura = temperatura_set;

    estado_actual_pin_A = digitalRead(pin_A); 

    if (estado_actual_pin_A != ultimo_estado_pin_A){     

      if (digitalRead(pin_B) != estado_actual_pin_A){        //DT != CLK  ``````````    
        aux_temperatura += incrementoTemperatura;
        delay(250);     
      }
    else{
      aux_temperatura -= incrementoTemperatura;
      delay(250); 
    } 
  }
}

void deteccion_cruce_cero() {
  if(incubando == 1){
    cruce_x_cero = true;
    i = 0;
    digitalWrite(Triac, LOW);
  }
}

void variador() {
  if (cruce_x_cero) {
    if (i >= potencia_foco) {
      digitalWrite(Triac, HIGH);
      i = 0;
      cruce_x_cero = false;
    } else {
      i++;
    }
  }
}

void controlHumedad(float humedad){
  if(humedad >= humedad_set){
    digitalWrite(ventilador_humedad,HIGH);   
  }else{
    digitalWrite(ventilador_humedad,LOW);
  }
}


void controlRotacion(){
  unsigned long tiempoActualRotacion = millis();
  if(tiempoDia * dias_set < tiempoActualRotacion){
    unsigned long intervaloRotacion = tiempoDia / frecuencia_rotacion;

    if (tiempoActualRotacion - tiempoUltimaRotacion >= intervaloRotacion) {
      // Encender el motor
      digitalWrite(motor_rotacion, HIGH);
      // Actualizar el tiempo de la última rotación
      tiempoUltimaRotacion = tiempoActualRotacion;
    }else{
      digitalWrite(motor_rotacion, LOW);   
    }
  }else{
    digitalWrite(motor_rotacion, LOW);
  }
}

void computePID() {
  Temperatura_medida = (5.0 * 100.0 * analogRead(A0) / 1024.0); //Lectura LM35
  double error = temperatura_set - Temperatura_medida;
  ParteIntegral += Ki * error;

  // Limitar la integral para evitar acumulación excesiva
  ParteIntegral = constrain(ParteIntegral, minPotencia, maxPotencia);

  // Calcula la diferencia entre la potencia actual y la potencia que se debería aplicar según el control PID
  double potencia_deseada = 100 - (Kp * error + ParteIntegral - Kd * (Temperatura_medida - Temperatura_medida_anterior));
  double delta_potencia = potencia_deseada - potencia_foco;

  // Restricción en el incremento de potencia para frenar antes de alcanzar el setpoint
  double max_incremento = 5.0; // Ajusta este valor según tu necesidad
  if (fabs(delta_potencia) > max_incremento) {
    if (delta_potencia > 0) {
      delta_potencia = max_incremento;
    } else {
      delta_potencia = -max_incremento;
    }
  }

  // Actualiza la potencia aplicada al foco
  potencia_foco += int(delta_potencia);
  potencia_foco = int(constrain(potencia_foco, minPotencia, maxPotencia));

  Temperatura_medida_anterior = Temperatura_medida;
}


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  
  Serial.begin(9600);                     //Habilitamos la salida serial por USB
  
  pinMode(Triac, OUTPUT);
  pinMode(ventilador_humedad, OUTPUT);
  pinMode(motor_rotacion, OUTPUT);
  pinMode (pin_A,INPUT);
  pinMode (pin_B,INPUT);
  pinMode (boton_encoder,INPUT_PULLUP);
  pinMode(A0, INPUT);
  sensor_humedad.begin();
  attachInterrupt(0, deteccion_cruce_cero, RISING);
  Timer1.initialize(T_int);
  Timer1.attachInterrupt(variador, T_int);

  lcd.init();
  lcd.backlight();
  lcd.createChar(0, icono_flecha);              //Caracter personalizado       

  fn_menu(contador,menu_1,size_menu_1);      //Iniciamos presentando el menu principal

  ultimo_estado_pin_A = digitalRead(pin_A);         //Leemos el estado de la salida del Encoder usando el pin CLK

}

void loop() {
  potencia_foco = 0;
  i = 0;
  
  selectOption();                             //Funcion para detectar cuando se oprime el encoder.  boton_apretado == true

  
  if(nivel_del_menu == 0){                        //Esta variable corresponde al nivel principal del menu. nivel_del_menu = 0.
 
    if(fn_encoder(size_menu_1) ){               //Esta funcion muestra en el LCD el menu en el que estamos
      fn_menu(contador,menu_1,size_menu_1);      //Esta funcion muestra la posicion dentro de ese menu segun el valor de la variable contador
    }

    if(boton_apretado){                             //Verificamos si el boton del encoder fue oprimido. boton_apretado == true
      switch(contador){
        case 0: //GALLINA
          temperatura_set = 37.7;
          humedad_set = 65;
          frecuencia_rotacion = 7;
          dias_set = 21;
          break;
        case 1: //CODORNIZ
          temperatura_set = 37.7;
          humedad_set = 55;
          frecuencia_rotacion = 12;
          dias_set = 17;
          break;
        case 2: //PATO
          temperatura_set = 37.7;
          humedad_set = 57;
          frecuencia_rotacion = 6;
          dias_set = 28;
          break;
        case 3: //CUSTOM
          temperatura_set = 37.7;
          humedad_set = 65;
          frecuencia_rotacion = 7;
          dias_set = 21;
          break;
      }
      contador = 0;
      fn_menu(contador,menu_2,size_menu_2);
      nivel_del_menu = 1;
      boton_apretado = false;                               //  Nos aseguramos que esta variable de retorno de la funcion selectOption() vuelva incrementoTemperatura
    }
  }

  if(nivel_del_menu == 1){                                
    if(fn_encoder(size_menu_2)){                         //Nos desplazamos con el encoder sleccionando las diferentes opciones
      fn_menu(contador,menu_2,size_menu_2);
    }
    
    if(boton_apretado){                                      //Verificamos si el boton del encoder fue oprimido. boton_apretado == true
      switch(contador){
        case 0:
          lcd.clear();

          do{
            fn_contador_temperatura();
            temperatura_set = aux_temperatura;
            lcd.setCursor(0,0);
            lcd.print("Temperatura: ");
            lcd.setCursor(0,1);
            lcd.print(temperatura_set);
            delay(150);
            lcd.print("");
            delay(150);
          }while(digitalRead(boton_encoder) == HIGH);
          delay(500);
          contador = 6;
          break;
        case 1:
          lcd.clear();

          do{
            fn_contador_entero(humedad_set);            
            humedad_set = contador;
            lcd.setCursor(0,0);
            lcd.print("Humedad: ");
            lcd.setCursor(0,1);
            lcd.print(humedad_set);
          }while(digitalRead(boton_encoder) == HIGH);
          delay(500);
          contador = 6;
          break;
        case 2:
          lcd.clear();

          do{
            fn_contador_entero(dias_set);
            dias_set = contador;
            lcd.setCursor(0,0);
            lcd.print("Dias Incub.:");
            lcd.setCursor(0,1);
            lcd.print(dias_set);
          }while(digitalRead(boton_encoder) == HIGH);
          delay(200);
          contador = 6;
          break;
        case 3:
          lcd.clear();

          do{
            fn_contador_entero(frecuencia_rotacion);
            frecuencia_rotacion = contador;
            lcd.setCursor(0,0);
            lcd.print("Rot. por dia:");
            lcd.setCursor(0,1);
            lcd.print(frecuencia_rotacion);
          }while(digitalRead(boton_encoder) == HIGH);
          delay(200);
          contador = 6;
          break;
        case 4: //CONTINUAR INCUBACION

          incubando = 1;
          while(1){
              unsigned long tiempoInicio = millis();
              unsigned long tiempoActual = millis();
              // Actualizar la pantalla cada 5 segundos
              if (tiempoActual - tiempoUltimo >= intervaloTiempo) {
                tiempoUltimo = tiempoActual;
                contadorMenu++;
                // Ejecutamos la funcion "PID"
                computePID();
                float humedad = sensor_humedad.readHumidity();
                controlHumedad(humedad);
                controlRotacion();
                lcd.clear();
                lcd.setCursor(0, 0);
                
                switch(contadorMenu){
                  case 1:

                    lcd.print("Temp Set: ");
                    lcd.setCursor(10, 0);
                    lcd.print(temperatura_set);
                    lcd.setCursor(0, 1);
                    lcd.print("Temperatura: ");
                    lcd.setCursor(12, 1);
                    lcd.print(Temperatura_medida);
                  break;
                  case 2:

                    lcd.print("Hum Set: ");
                    lcd.setCursor(9, 0);
                    lcd.print(humedad_set);
                    lcd.setCursor(0, 1);
                    lcd.print("Humedad: ");
                    lcd.setCursor(9, 1);
                    lcd.print(humedad);
                  break;
                  case 3:

                    lcd.setCursor(0, 0);
                    lcd.print("Dias:Hs:Min: ");
                    lcd.setCursor(0, 1);

                    // Convierte el tiempo transcurrido a días, horas, minutos
                    unsigned long diasTranscurridos = tiempoInicio / tiempoDia;
                    unsigned long horasTranscurridas = (tiempoInicio % tiempoDia) / 3600000; // 3600000 milisegundos en una hora
                    unsigned long minutosTranscurridos = ((tiempoInicio % tiempoDia) % 3600000) / 60000; // 60000 milisegundos en un minuto
                    lcd.print(int(diasTranscurridos));
                    lcd.print(":");
                    lcd.print(int(horasTranscurridas));
                    lcd.print(":");
                    lcd.print(int(minutosTranscurridos));

                    contadorMenu=0; //contador para ir variando entre mostrar Temperatura, humedad y tiempo transcurrido
                  break;
                }
              }
              // Corregir el sistema cada segundo
              if (tiempoActual - tiempoDeCorreccion >= intervaloCorreccion) {
                tiempoDeCorreccion = tiempoActual;
                computePID();
              }
            }
          break;
        case 5: //PANTALLA DE ATRAS 
          contador = 0;
          fn_menu(contador,menu_1,size_menu_1);
          nivel_del_menu = 0;
          break;
        case 6: //VOLVER AL MENU 2
          contador = 0;
          fn_menu(contador,menu_2,size_menu_2);
          nivel_del_menu = 1;
          break;
      }
      boton_apretado = false;
    }
  }
}
