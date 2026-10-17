#include <Arduino.h>
#include <Bluepad32.h>
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
        public:  uint8_t weapon: 1;       // indica se a arma está ativada
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
        digitalWrite(FURADEIRA, controlMode.settings.weapon = myControllers[0]->l2());
        
        #if ENABLE_DEBUG_PRINT
            #if ENABLE_COLORS_IN_PRINT
                Console.printf("\033[90mweapon: %s  ", controlMode.settings.weapon ? "\033[91mON " : "\033[97mOFF");
            #else
                Console.printf("weapon: %s  ", controlMode.settings.weapon ? "ON " : "OFF");
            #endif
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
            #if ENABLE_COLORS_IN_PRINT
                Console.printf("\033[90mturbo: %s\033[90m  mode: \033[97m%s  ",
                        controlMode.settings.turbo ? "\033[93mON " : "\033[97mOFF",
                        controlMode.settings.corrected ? "  corr" : "uncorr");
            #else
                Console.printf("turbo: %s  mode: %s  ",
                        controlMode.settings.turbo ? "ON " : "OFF",
                        controlMode.settings.corrected ? "  corr" : "uncorr");
            #endif
        #endif
        
        float linVel;
        float angVel;
        
        int RSpeed;
        int LSpeed;
        
        float turboMultiplier = 1;
        
        linVel = myControllers[0]->axisY();
        angVel = myControllers[0]->axisRX();
        
        #if ENABLE_DEBUG_PRINT
                #if ENABLE_COLORS_IN_PRINT
                Console.printf("\033[90mraw: (lin = \033[97m% 4.0f\033[90m  ang = \033[97m% 4.0f\033[90m) \033[34m-> ", linVel, angVel);
            #else
                Console.printf("raw: (lin = % 4.0f  ang = % 4.0f) -> ", linVel, angVel);
            #endif
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
            #if ENABLE_COLORS_IN_PRINT
                u_char linColor[3] = {linVel > 0 ? (u_char)((1 - linVel) * 255) : 255, linVel < 0 ? (u_char)((1 + linVel) * 255) : 255, (u_char)((1 - abs(linVel)) * 255)};
                u_char angColor[3] = {angVel > 0 ? (u_char)((1 - angVel) * 255) : 255, angVel < 0 ? (u_char)((1 + angVel) * 255) : 255, (u_char)((1 - abs(angVel)) * 255)};
                Console.printf("\033[90mrel: (lin = \033[38;2;%hhu;%hhu;%hhum% 6.1f\033[90m  ang = \033[38;2;%hhu;%hhu;%hhum% 6.1f\033[90m) \033[34m=> ",
                    linColor[0], linColor[1], linColor[2], linVel * 100, angColor[0], angColor[1], angColor[2], angVel * 100);
                
                u_char RColor[3] = {RSpeed > 0 ? (u_char)(255 - RSpeed) : 255, RSpeed < 0 ? (u_char)(255 + RSpeed) : 255, (u_char)(255 - abs(RSpeed))};
                u_char LColor[3] = {LSpeed > 0 ? (u_char)(255 - LSpeed) : 255, LSpeed < 0 ? (u_char)(255 + LSpeed) : 255, (u_char)(255 - abs(LSpeed))};
                Console.printf("\033[90mout: (V_R = \033[38;2;%hhu;%hhu;%hhum% 4d  \033[90mV_L = \033[38;2;%hhu;%hhu;%hhum% 4d\033[90m)\033[m\n",
                    RColor[0], RColor[1], RColor[2], RSpeed, LColor[0], LColor[1], LColor[2], LSpeed);
            #else
                Console.printf("rel: (lin = % 6.1f  ang = % 6.1f) -> out: (V_R = % 3d  V_L = % 3d)\n", linVel * 100, angVel * 100, RSpeed, LSpeed);
            #endif
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