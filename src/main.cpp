#include <Arduino.h>
#include <Bluepad32.h>
#include <ESP32Servo.h>
#include <time.h>
#include <sys/time.h>

#include "settings.h"

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
            digitalWrite(FURADEIRA, LOW);
            digitalWrite(SOLENOIDE, LOW);
            analogWrite(MOTOR_R1, 0);
            analogWrite(MOTOR_R2, 0);
            analogWrite(MOTOR_L1, 0);
            analogWrite(MOTOR_L2, 0);
            break;
        }
    }
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(MOTOR_R1, OUTPUT);
    pinMode(MOTOR_R2, OUTPUT);
    pinMode(MOTOR_L1, OUTPUT);
    pinMode(MOTOR_L2, OUTPUT);
    pinMode(FURADEIRA, OUTPUT);
    pinMode(SOLENOIDE, OUTPUT);
    
    
    analogWrite(MOTOR_R1, 0);
    analogWrite(MOTOR_R2, 0);
    analogWrite(MOTOR_L1, 0);
    analogWrite(MOTOR_L2, 0);
    digitalWrite(FURADEIRA, 0);
    digitalWrite(SOLENOIDE, 0);
    
    // Inicializa o Bluepad32 com os callbacks
    BP32.setup(&onConnectedController, &onDisconnectedController);
    
}

float changeModeDelay = 0;
bool changeModeButtonHeld = false;

union ControlMode {
    uint8_t led;
    struct{
        private: uint8_t _firstLed: 1;    // sempre ligado
        public:  uint8_t corrected: 1;    // indica se é o modo corrigido
        private: uint8_t _paddignBit: 1;  // sempre desligado
        public:  uint8_t turbo: 1;        // indica se o turbo está ativado
    } settings;
} controlMode = {0b0011};

unsigned long timeOld = 0, timeNew, dt = 0;

void loop() {
    timeNew = millis();
    dt = timeNew - timeOld;
    timeOld = timeNew;
    
    // Atualiza o estado da antena Bluetooth
    BP32.update();

    // Se o controle 0 estiver conectado, lê os dados
    if (myControllers[0] && myControllers[0]->isConnected()) {
        
        // ativar a arma
        
        digitalWrite(FURADEIRA, myControllers[0]->l2());
        
        #if ENABLE_DEBUG_PRINT
            Console.printf("%s ", myControllers[0]->l2() ? "ON " : "OFF");
        #endif
        // mudar o modo de controle
        if (!changeModeButtonHeld && myControllers[0]->miscStart() && changeModeDelay < 0){
            changeModeButtonHeld = true;
            changeModeDelay = 100;
            controlMode.settings.corrected = !controlMode.settings.corrected;
        }
        else if (!myControllers[0]->miscStart()){
            changeModeButtonHeld = false;
        }
        if(changeModeDelay >= 0)
            changeModeDelay -= dt;
        
        controlMode.settings.turbo = myControllers[0]->r1();
        
        myControllers[0]->setPlayerLEDs(controlMode.led);
        
        #if ENABLE_DEBUG_PRINT
            Console.printf("%d ", myControllers[0]->getModel());
            Console.printf("%s ", myControllers[0]->getModelName());
            
            Console.printf("%s %s ",
                    controlMode.settings.turbo ? "turbo" : "     ",
                    controlMode.settings.corrected ? "  corr " : "uncorr ");
        #endif
        
        float linVel;
        float angVel;
        
        int RSpeed;
        int LSpeed;
        
        float turboMultiplier = 1;
        
        linVel = myControllers[0]->axisY();
        angVel = myControllers[0]->axisRX();
        
        #if ENABLE_DEBUG_PRINT
            Console.printf("lin = % 4.0f  ang = % 4.0f -> ", linVel, angVel);
        #endif
        
        linVel = NORMALIZE_CONTROLLER_INPUT(linVel, LEFT_ANALOG_Y_MIN, LEFT_ANALOG_Y_MAX);
        angVel = NORMALIZE_CONTROLLER_INPUT(angVel, RIGHT_ANALOG_X_MIN, RIGHT_ANALOG_X_MAX);
        
        if(abs(linVel) < VALID_INPUT_THRESHOLD)
            linVel = 0;
        if(abs(angVel) < VALID_INPUT_THRESHOLD)
            angVel = 0;
        
        RSpeed = (LINEAR_VELOCITY * linVel + (!controlMode.settings.corrected || linVel >= 0 ? 1 : -1) * ANGULAR_VELOCITY * angVel) / 2;
        LSpeed = (LINEAR_VELOCITY * linVel - (!controlMode.settings.corrected || linVel >= 0 ? 1 : -1) * ANGULAR_VELOCITY * angVel) / 2;
        
        // multiplicador para o modo turbo se as velocidades não forem nulas
        if (controlMode.settings.turbo && (linVel || angVel)){
            turboMultiplier = 255.0 * std::max(abs(linVel), abs(angVel)) / std::max(abs(RSpeed), abs(LSpeed));
        
            RSpeed *= turboMultiplier;
            LSpeed *= turboMultiplier;
        }
        
        RSpeed = CLAMP(RSpeed, -255, 255);
        LSpeed = CLAMP(LSpeed, -255, 255);
        
        #if ENABLE_DEBUG_PRINT
            Console.printf("lin = % 4.1f  ang = % 4.1f -> V_R = % 3d  V_L = % 3d\n", linVel * 100, angVel * 100, RSpeed, LSpeed);
        #endif

        if(RSpeed >= 0){
            analogWrite(MOTOR_R1, RSpeed);
            analogWrite(MOTOR_R2, 0);
        }
        else{
            analogWrite(MOTOR_R1, 0);
            analogWrite(MOTOR_R2, -RSpeed);
        }
        
        if(LSpeed >= 0){
            analogWrite(MOTOR_L1, LSpeed);
            analogWrite(MOTOR_L2, 0);
        }
        else{
            analogWrite(MOTOR_L1, 0);
            analogWrite(MOTOR_L2, -LSpeed);
        }
    }

    delay(20); 
}