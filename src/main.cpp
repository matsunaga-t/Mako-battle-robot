#include <Arduino.h>
#include <Bluepad32.h>
#include <ESP32Servo.h>
#include <time.h>
#include <sys/time.h>

#define MOTOR_R1 18
#define MOTOR_R2 19
#define MOTOR_L1 26
#define MOTOR_L2 25

#define CLAMP(x, lower, upper) (x < lower ? lower : x > upper ? upper : x) 

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

enum ControlMode: uint8_t {automaticUncorrected = 0b001, automaticCorrected = 0b010, manual = 0b100};

float changeModeDelay = 0;
bool changeModeButtonHeld = false;

union {
    struct{
        uint8_t mode:3;
        uint8_t turbo:1;
    };
    uint8_t led;
} controlMode = {.turbo = 0, .mode = ControlMode::automaticCorrected};

unsigned long timeOld = 0, timeNew, dt = 0;

void loop() {
    timeNew = millis();
    dt = timeNew - timeOld;
    timeOld = timeNew;
    
    // Atualiza o estado da antena Bluetooth
    BP32.update();

    // Se o controle 0 estiver conectado, lê os dados
    if (myControllers[0] && myControllers[0]->isConnected()) {
        
        if (!changeModeButtonHeld && myControllers[0]->miscStart() && changeModeDelay < 0){
            changeModeButtonHeld = true;
            changeModeDelay = 100;
            switch(controlMode.mode){
                case ControlMode::automaticCorrected:
                    controlMode.mode = ControlMode::automaticUncorrected;
                break;
                
                case ControlMode::automaticUncorrected:
                    controlMode.mode = ControlMode::manual;
                break;
                
                case ControlMode::manual:
                    controlMode.mode = ControlMode::automaticCorrected;
                break;
            }
        }
        else if (!myControllers[0]->miscStart()){
            changeModeButtonHeld = false;
        }
        if(changeModeDelay >= 0)
            changeModeDelay -= dt;
        
        controlMode.turbo = myControllers[0]->r1();
        
        myControllers[0]->setPlayerLEDs(controlMode.led);
        
        float linVel;
        float angVel;
        
        int RSpeed;
        int LSpeed;
        
        switch(controlMode.mode){
            case ControlMode::automaticUncorrected:
                linVel = -myControllers[0]->axisY();
                angVel = myControllers[0]->axisRX();
                
                linVel = CLAMP((linVel + 450) / (450 - (-490)) * 2 - 1, -1.0, 1.0);
                angVel = CLAMP((angVel + 530) / (530 - (-411)) * 2 - 1, -1.0, 1.0);
                
                RSpeed = (255 * linVel + 127 * angVel) / 2 * (controlMode.turbo ? 255 / 191 : 1);
                LSpeed = (255 * linVel - 127 * angVel) / 2 * (controlMode.turbo ? 255 / 191 : 1);
            break;
            
            case ControlMode::automaticCorrected:
                linVel = -myControllers[0]->axisY();
                angVel = myControllers[0]->axisRX();
                
                linVel = CLAMP((linVel + 450) / (450 - (-490)) * 2 - 1, -1.0, 1.0);
                angVel = CLAMP((angVel + 530) / (530 - (-411)) * 2 - 1, -1.0, 1.0);
                
                RSpeed = (255 * linVel + (linVel > 0 ? 1 : -1) * 127 * angVel) / 2 * (controlMode.turbo ? 255 / 191 : 1);
                LSpeed = (255 * linVel - (linVel > 0 ? 1 : -1) * 127 * angVel) / 2 * (controlMode.turbo ? 255 / 191 : 1);
            break;
            
            case ControlMode::manual:
                RSpeed = 255 * CLAMP((-myControllers[0]->axisY()  + 450) / (450 - (-490)) * 2 - 1, -1.0, 1.0);
                LSpeed = 255 * CLAMP((-myControllers[0]->axisRY() + 450) / (450 - (-490)) * 2 - 1, -1.0, 1.0);
                
            break;
        }
        // No Bluepad32, os eixos vão de -511 a 512
        
        //Console.printf("lin = % 4.1f  ang = % 4.1f -> V_R = % 3d  V_L = % 3d\n", linVel * 100, angVel * 100, RSpeed, LSpeed);
        //Console.printf("V_R = % 3d  V_L = % 3d\n", RSpeed, LSpeed);
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