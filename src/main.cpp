#include <Arduino.h>
#include <Bluepad32.h>
#include <ESP32Servo.h>
#include <time.h>

#define MOTOR_R1 18
#define MOTOR_R2 19
#define MOTOR_L1 26
#define MOTOR_L2 25

#define clamp(x, lower, upper) (x < lower ? lower : x > upper ? upper : x) 


// Array para armazenar os controles conectados
ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// Callback: Quando um controle conecta
void onConnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            myControllers[i] = ctl;
            digitalWrite(LED_BUILTIN, HIGH);
            break;
        }
    }
}

// Callback: Quando um controle desconecta
void onDisconnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            myControllers[i] = nullptr;
            digitalWrite(LED_BUILTIN, LOW);
            break;
        }
    }
}

void setup() {
    //Serial.begin(115200);
    //Serial.println("Ligando Bluetooth... Pressione SYNC no controle.");
    
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(MOTOR_R1, OUTPUT);
    pinMode(MOTOR_R2, OUTPUT);
    pinMode(MOTOR_L1, OUTPUT);
    pinMode(MOTOR_L2, OUTPUT);
    // Inicializa o Bluepad32 com os callbacks
    BP32.setup(&onConnectedController, &onDisconnectedController);
    
}

void loop() {
    // Atualiza o estado da antena Bluetooth
    BP32.update();

    // Se o controle 0 estiver conectado, lê os dados
    if (myControllers[0] && myControllers[0]->isConnected()) {
        
        float linVel;
        float angVel;
        
        int RSpeed;
        int LSpeed;
        
        linVel = -myControllers[0]->axisY();
        angVel = myControllers[0]->axisRX();
        
        linVel = clamp((linVel + 450) / (450 - (-490)) * 2 - 1, -1.0, 1.0);
        angVel = clamp((angVel + 530) / (530 - (-411)) * 2 - 1, -1.0, 1.0);
        
        RSpeed = (255 * linVel + 127 * angVel) / 2;
        LSpeed = (255 * linVel - 127 * angVel) / 2;
            
        
        //Console.printf("lin = % 4.1f  ang = % 4.1f -> V_R = % 3d  V_L = % 3d\n", linVel * 100, angVel * 100, RSpeed, LSpeed);
        Console.printf("V_R = % 3d  V_L = % 3d\n", RSpeed, LSpeed);
        if(RSpeed > 20){
            analogWrite(MOTOR_R1, RSpeed);
            analogWrite(MOTOR_R2, 0);
        }
        else if(RSpeed < -20){
            analogWrite(MOTOR_R1, 0);
            analogWrite(MOTOR_R2, -RSpeed);
        }
        else{
            analogWrite(MOTOR_R1, 0);
            analogWrite(MOTOR_R2, 0);
        }
        
        if(LSpeed > 20){
            analogWrite(MOTOR_L1, LSpeed);
            analogWrite(MOTOR_L2, 0);
        }
        else if(LSpeed < -20){
            analogWrite(MOTOR_L1, 0);
            analogWrite(MOTOR_L2, -LSpeed);
        }
        else{
            analogWrite(MOTOR_L1, 0);
            analogWrite(MOTOR_L2, 0);
        }
    }

    delay(20); 
}