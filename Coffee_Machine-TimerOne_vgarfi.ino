#include <TimerOne.h>
//------------------------------------------------------------------------------------\\

#define BOUD_RATE         9600
#define VACIO             0
#define US_TIMERONE       1000

#define CORTO             0
#define LARGO             1

#define PIN_BTN1          12      
#define PIN_SENSOR        8

#define CAFE_CORTO        1
#define CAFE_LARGO        2
#define TIEMPO_CORTO      3000
#define TIEMPO_LARGO      5000

#define MODO_BTN_SENSOR   1
#define MODO_CORTO        2
#define MODO_LARGO        3
#define MODO_SENSOR       4

#define PIN_LED           5

int ms = 0;

int modoIsrTimer = MODO_BTN_SENSOR; // esta variable la uso en la funcion timer para saber hasta cuando contar y desactvar el boton cuando se está sirviendo el café
int msBotonSensor = 0;      // Contadores de milisegundos para los antirrebotes de los botones

/* -----------------------------Definiciones para la maquina de estados del boton----------------------- */

#define ESTADO_BOTON_ESPERA         0
#define ESTADO_BOTON_CONFIRMACION   1
#define ESTADO_BOTON_LIBERACION     2

#define ESTADO_BOTON_INICIAL    ESTADO_BOTON_ESPERA
#define MS_ANTIRREB 25  // Espera (en milisegundos) para el antirrebote

int estadoBoton = ESTADO_BOTON_INICIAL;    // Array de estados actuales de las maquinas de estados de los botones

char flagBoton = 0;   // Banderas que indican el estado actual de los botones. Los uso en la maq de estados
char pinBoton;    // Array de pines donde estan conectados los botones. Los uso en la maq de estados

/* ----------------------------Definiciones para la maquina de estados del sensor------------------------ */

#define ESTADO_SENSOR_ESPERA          0
#define ESTADO_SENSOR_CONFIRMACION    1
#define ESTADO_SENSOR_DETECTADO       2

int estadoMaquinaSensor = ESTADO_SENSOR_ESPERA;
bool flagSensor = false;
bool activarSensor = false;

/* -----------------Definiciones para la maquina de estados del lector de puerto serie------------------- */

#define ESTADO_ESPERA     0
#define ESTADO_LECTURA    1
#define ESTADO_CONFIRMA   2

int estadoMaquinaLectura = ESTADO_ESPERA;
String cafeLeido;
bool flagsCafes[2];

/* -----------------------------------------Definiciones de texto---------------------------------------- */

#define MSJ_INGRESE_CAFE  "Buenos dias. Para elegir el tipo de cafe, escriba CORTO o LARGO: "
#define MSJ_CAFE_CORTO    "CORTO"
#define MSJ_CAFE_LARGO    "LARGO"
#define MSJ_ERROR_CAFE1   "\nERROR, '"
#define MSJ_ERROR_CAFE2   "' no es un tipo de cafe permitido. Por favor, ingrese CORTO o LARGO" 
#define MSJ_PREPARANDO    "\nPreparando un rico cafe "
#define MSJ_PUNTOS_SUSP   "..."
#define MSJ_CAFE_LEIDO    "\nCafe leido con exito. Pulse el boton para poder comenzar a prepararlo"
#define MSJ_ERROR         "Error."
#define MSJ_CORTO_SERVIDO "Cafe corto servido"
#define MSJ_LARGO_SERVIDO "Cafe largo servido"
#define MSJ_RETIRE_CAFE   "Por favor, retire su café para poder realizar otro"
#define MSJ_GRACIAS       "Gracias!\n\n"
/* -----------------------------------------Prototipos de funciones---------------------------------------- */

void timer(void);
void FSM_Antirrebote(void);
void maquinaCafetera (void);
void maquinaLecturaPuertoSerie (void);
void maquinaLecturaSensorIR (void);

void setup() {
  // put your setup code here, to run once:
  
  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode (PIN_SENSOR, INPUT);
  pinBoton = PIN_BTN1;

  Timer1.initialize(US_TIMERONE);
  Timer1.attachInterrupt(timer);

  flagsCafes[CORTO] = false;
  flagsCafes[LARGO] = false;
  
  pinMode (PIN_LED, OUTPUT);
  Serial.begin(BOUD_RATE);
  Serial.println(MSJ_INGRESE_CAFE);
}

void loop()
{
  // put your main code here, to run repeatedly:

  maquinaLecturaPuertoSerie();
  FSM_Antirrebote();
  maquinaLecturaSensorIR();
  maquinaCafetera ();
}

void maquinaCafetera (void)
{
  if (flagBoton == 1)
  {
     if (flagsCafes[CORTO] == true) // activo la cafetera encendiendo el LED en el modo café corto
    {
     modoIsrTimer = MODO_CORTO;
     Serial.print(MSJ_PREPARANDO);
     Serial.print(MSJ_CAFE_CORTO);
     Serial.println(MSJ_PUNTOS_SUSP);
     flagsCafes[CORTO] = false;
    }

    if (flagsCafes[LARGO] == true)// activo la cafetera encendiendo el LED en el modo café largo
    {
      modoIsrTimer = MODO_LARGO;
      Serial.print(MSJ_PREPARANDO);
      Serial.print(MSJ_CAFE_LARGO);
      Serial.println(MSJ_PUNTOS_SUSP);
      flagsCafes[LARGO] = false;
    }
    flagBoton = 0;
  }
 
  if (flagSensor == true && activarSensor == true)
  {
    Serial.println(MSJ_GRACIAS);
    flagSensor = false;
    activarSensor = false;
    Serial.println (MSJ_INGRESE_CAFE);
  }
}

void maquinaLecturaSensorIR (void)
{
  int lecturaSensor;

  switch(estadoMaquinaSensor)
    {
      case ESTADO_SENSOR_ESPERA:
                              lecturaSensor = digitalRead(PIN_SENSOR);
                              
                              // Si se da la condicion de cambio de estado
                              if(lecturaSensor == 0)
                              {
                                msBotonSensor = 0;
                                estadoMaquinaSensor = ESTADO_BOTON_CONFIRMACION;
                              }
      break;
          
      case ESTADO_SENSOR_CONFIRMACION:
                                    lecturaSensor = digitalRead(PIN_SENSOR);
                                    
                                    if(lecturaSensor == 0 && msBotonSensor >= MS_ANTIRREB)
                                    {
                                      estadoMaquinaSensor = ESTADO_SENSOR_DETECTADO;
                                    }
                        
                                    // Si se da la condicion, se considera ruido
                                    if(lecturaSensor == 1 && msBotonSensor < MS_ANTIRREB)
                                    {
                                      estadoMaquinaSensor = ESTADO_SENSOR_ESPERA;
                                    }
      break;
          
      case ESTADO_SENSOR_DETECTADO:
                                  lecturaSensor = digitalRead(PIN_SENSOR);
                                  
                                  // Si se da la condicion, se considera el boton como presionado
                                  if(lecturaSensor == 1)
                                  {
                                    flagSensor = 1;
                                    estadoMaquinaSensor = ESTADO_SENSOR_ESPERA;
                                  }
      break;
          
      default:
              // Si entra aca, hay un error en la variable estado_FSM, o algo mal asignado en ella
              Serial.println(MSJ_ERROR);
              estadoMaquinaSensor = ESTADO_SENSOR_ESPERA;
              break;
    }
  
}


  
void maquinaLecturaPuertoSerie (void)
{
  switch (estadoMaquinaLectura)
  {
    case ESTADO_ESPERA:
                      if (Serial.available() > VACIO)
                      {
                        estadoMaquinaLectura = ESTADO_LECTURA;
                      }
    break;

    case ESTADO_LECTURA:
                        cafeLeido = Serial.readString();
                        
                        if (cafeLeido.equals(MSJ_CAFE_CORTO)|| cafeLeido.equals(MSJ_CAFE_LARGO))
                        {
                          Serial.print(MSJ_CAFE_LEIDO);
                          estadoMaquinaLectura = ESTADO_CONFIRMA;
                        }

                        else
                        {
                          Serial.print(MSJ_ERROR_CAFE1 + cafeLeido + MSJ_ERROR_CAFE2);
                          estadoMaquinaLectura = ESTADO_ESPERA;
                        }
    break;

    case ESTADO_CONFIRMA:
                         if (cafeLeido.equals(MSJ_CAFE_CORTO))
                         {
                          flagsCafes[CORTO] = true;
                         }

                         if (cafeLeido.equals(MSJ_CAFE_LARGO))
                         {
                          flagsCafes[LARGO] = true;
                         }
                         
                         estadoMaquinaLectura = ESTADO_ESPERA;

    break;
  }

}


// Maquina de estados de antirrebote del boton. Recibe un argumento, que es el indice del boton en el array
void FSM_Antirrebote(void)
{
    char estadoPin;
    
    switch(estadoBoton)
    {
      case ESTADO_BOTON_ESPERA:
                              estadoPin = digitalRead(pinBoton);
                              
                              // Si se da la condicion de cambio de estado
                              if(estadoPin == 0)
                              {
                                msBotonSensor = 0;
                                estadoBoton = ESTADO_BOTON_CONFIRMACION;
                              }
      break;
          
      case ESTADO_BOTON_CONFIRMACION:
                                    estadoPin = digitalRead(pinBoton);
                                    
                                    // Si se da la condicion, se considera el boton como presionado
                                    if( estadoPin == 0 && msBotonSensor >= MS_ANTIRREB)
                                    {
                                      estadoBoton = ESTADO_BOTON_LIBERACION;
                                    }
                        
                                    // Si se da la condicion, se considera ruido
                                    if( estadoPin == 1 && msBotonSensor < MS_ANTIRREB)
                                    {
                                      estadoBoton = ESTADO_BOTON_ESPERA;
                                    }
      break;
          
      case ESTADO_BOTON_LIBERACION:
                                  estadoPin = digitalRead(pinBoton);
                                  
                                  // Si se da la condicion, se considera el boton como presionado
                                  if( estadoPin == 1)
                                  {
                                    flagBoton = 1;
                                    estadoBoton = ESTADO_BOTON_ESPERA;
                                  }
      break;
          
      default:
          // Si entra aca, hay un error en la variable estado_FSM, o algo mal asignado en ella
          Serial.println(MSJ_ERROR);
          estadoBoton = ESTADO_BOTON_INICIAL;
          break;
    }
}


/* ----------------------------------------------------------------------------------------------------- */

// Rutina de atencion de interrupcion del Timer 1
void timer(void)
{  
  switch (modoIsrTimer)
  {
    case MODO_BTN_SENSOR:
                  msBotonSensor = msBotonSensor + 1;
    break;

    case MODO_CORTO:
                    ms = ms + 1;
                    digitalWrite(PIN_LED, HIGH);
                    
                    if (ms >= TIEMPO_CORTO)
                    {
                      digitalWrite(PIN_LED, LOW);
                      ms = 0;
                      Serial.println(MSJ_CORTO_SERVIDO);
                      modoIsrTimer = MODO_BTN_SENSOR;
                      Serial.println(MSJ_RETIRE_CAFE);
                      activarSensor = true;
                    }
    break;

    case MODO_LARGO:
                    ms = ms + 1;
                    digitalWrite (PIN_LED, HIGH);

                    if (ms >= TIEMPO_LARGO)
                    {
                      digitalWrite(PIN_LED, LOW);
                      ms = 0;
                      Serial.println(MSJ_LARGO_SERVIDO);
                      modoIsrTimer = MODO_BTN_SENSOR;
                      Serial.println(MSJ_RETIRE_CAFE);
                      activarSensor = true;
                    }
    break;
  }
    

}
