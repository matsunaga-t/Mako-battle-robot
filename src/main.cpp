#include <Arduino.h>
#include <Bluepad32.h>
#include <time.h>
#include <sys/time.h>

#include "settings.h"

// Ponteiro para armazenar o controle
ControllerPtr myController = nullptr;

// Callback: Quando um controle conecta
void onConnectedController(ControllerPtr ctl) {
    ControllerProperties prop =  ctl->getProperties();

    if (prop.btaddr[0]  == controllerBTAdr[0] &&
        prop.btaddr[1]  == controllerBTAdr[1] &&
        prop.btaddr[2]  == controllerBTAdr[2] &&
        prop.btaddr[3]  == controllerBTAdr[3] &&
        prop.btaddr[4]  == controllerBTAdr[4] &&
        prop.btaddr[5]  == controllerBTAdr[5] &&
        prop.vendor_id  == controllerVID      &&
        prop.product_id == controllerPID
    ){
        #if ENABLE_CONNECTION_DEBUG_PRINT
            Console.printf("controller %s accepted\nFurther connections disabled\n", ctl->getModelName());
        #endif
        myController = ctl;
        digitalWrite(LED_BUILTIN, HIGH);
        BP32.enableNewBluetoothConnections(false);
    }
    else {
        #if ENABLE_CONNECTION_DEBUG_PRINT
            Console.printf("controller %s rejected\n", ctl->getModelName());
        #endif
        ctl->disconnect();
        BP32.forgetBluetoothKeys();
    }
}

// Callback: Quando um controle desconecta
void onDisconnectedController(ControllerPtr ctl) {
    #if ENABLE_CONNECTION_DEBUG_PRINT
        Console.printf("controller %s disconnected\n", ctl->getModelName());
    #endif
    if (myController == ctl) {
        #if ENABLE_CONNECTION_DEBUG_PRINT
            Console.print("failsafe activated\n");
        #endif
        myController = nullptr;
        digitalWrite(LED_BUILTIN, LOW);
        digitalWrite(FURADEIRA, LOW);
        digitalWrite(SOLENOIDE, LOW);
        analogWrite(MOTOR_R1, 0);
        analogWrite(MOTOR_R2, 0);
        analogWrite(MOTOR_L1, 0);
        analogWrite(MOTOR_L2, 0);
    }
}

void setup() {
    // LED builtin para a conexão com o controle
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
    // Calcula o tempo decorrido desde a última atualização
    timeNew = millis();
    dt = timeNew - timeOld;
    timeOld = timeNew;
    
    #if ENABLE_DEBUG_PRINT
        #if ENABLE_COLORS_IN_PRINT
            Console.printf("\033[90mdt= % 4lu  ", dt);
        #else
            Console.printf("dt= % 4lu  ", dt);
        #endif
    #endif
    
    // Atualiza o estado da antena Bluetooth
    BP32.update();
    
    // Se o controle 0 estiver conectado, lê os dados
    if (myController && myController->isConnected()) {

        // ativar a arma
        digitalWrite(FURADEIRA, controlMode.settings.weapon = myController->l2());
        
        #if ENABLE_DEBUG_PRINT
            #if ENABLE_COLORS_IN_PRINT
                Console.printf("\033[90mweapon: %s  ", controlMode.settings.weapon ? "\033[91mON " : "\033[97mOFF");
            #else
                Console.printf("weapon: %s  ", controlMode.settings.weapon ? "ON " : "OFF");
            #endif
        #endif
        // mudar o modo de controle
        if (!changeModeButtonHeld && myController->miscStart() && changeModeDelay < 0){
            changeModeButtonHeld = true;
            changeModeDelay = 100;
            controlMode.settings.corrected = !controlMode.settings.corrected;
        }
        else if (!myController->miscStart()){
            changeModeButtonHeld = false;
        }
        if(changeModeDelay >= 0)
            changeModeDelay -= dt;
        
        // aciona o modo turbo
        controlMode.settings.turbo = myController->r1();
        
        // atualiza os LEDs do controle
        myController->setPlayerLEDs(controlMode.led);
        
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
        
        linVel = myController->axisY();
        angVel = myController->axisRX();
        
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