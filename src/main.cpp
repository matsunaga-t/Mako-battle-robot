#include <Arduino.h>
#include <Bluepad32.h>
#include <time.h>
#include <sys/time.h>

#include "settings.h"
#include "functionalMacros.h"

// Ponteiro para armazenar o controle
ControllerPtr myController = nullptr;

// Callback: Quando um controle conecta
void onConnectedController(ControllerPtr ctl) {
    ControllerProperties prop =  ctl->getProperties();

    // confere se as suas credenciais batem com as do controle desejado
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
            #if ENABLE_COLORS_IN_PRINT
                Console.printf("\ncontroller \033[97m%s \033[32maccepted\n\033[93mFurther connections disabled\033[m\n", ctl->getModelName());
            #else
                Console.printf("\ncontroller %s accepted\nFurther connections disabled\n", ctl->getModelName());
            #endif
        #endif
        myController = ctl;
        digitalWrite(LED_BUILTIN, HIGH);
        
        // desativa localização por novos controles
        // dessa forma, apenas o controle desejado consegue se conectar
        BP32.enableNewBluetoothConnections(false);
    }
    else {
        #if ENABLE_CONNECTION_DEBUG_PRINT
            #if ENABLE_COLORS_IN_PRINT
                Console.printf("\ncontroller \033[97m%s \033[91mrejected\033[m\n", ctl->getModelName());
            #else
                Console.printf("\ncontroller %s rejected\n", ctl->getModelName());
            #endif
        #endif
        
        // desconecta o controle e deleta as chaves, para que não se reconecte a ele por engano
        ctl->disconnect();
        BP32.forgetBluetoothKeys();
    }
}

// Callback: Quando um controle desconecta
void onDisconnectedController(ControllerPtr ctl) {
    #if ENABLE_CONNECTION_DEBUG_PRINT
        #if ENABLE_COLORS_IN_PRINT
                Console.printf("\ncontroller \033[97m%s\033[m disconnected\n", ctl->getModelName());
            #else
                Console.printf("\ncontroller %s disconnected\n", ctl->getModelName());
            #endif
    #endif
    if (myController == ctl) {
        #if ENABLE_CONNECTION_DEBUG_PRINT
            #if ENABLE_COLORS_IN_PRINT
                Console.print("\033[93mfailsafe activated\033[m\n");
            #else
                Console.print("failsafe activated\n");
            #endif
        #endif
        myController = nullptr;
        
        // desativa todas as saídas como failsafe
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
    pinMode(LED_BUILTIN, OUTPUT);  // para saber o estado da conexão com o controle
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

/**
 * Serve para armazenar flags das configurações do controle 
 * e mapeá-las aos LEDs dele
 */
union ControlMode {
    // o bitmap dos LEDs
    uint8_t led;
    
    // bitmap das configurações
    struct{       
        // sempre ligado
        // usado para saber que o controle está conectado
        private: uint8_t _firstLed: 1;
        
        // indica se é o modo corrigido
        // nesse modo, o robô é controlado como um carro normal
        public:  uint8_t corrected: 1;
        
        // indica se a arma está ativada
        public:  uint8_t weapon: 1;
        
        // indica se o turbo está ativado
        public:  uint8_t turbo: 1;
    } settings; 
} controlMode = {0b0011};

unsigned long timeOld = 0, timeNew, dt = 0;

void loop() {
    // Calcula o tempo decorrido desde a última atualização
    timeNew = millis();
    dt = timeNew - timeOld;
    timeOld = timeNew;
    
    #if ENABLE_TIME_DELTA_DEBUG_PRINT
        #if ENABLE_COLORS_IN_PRINT
            Console.printf("\033[90mdt= \033[97m%3lu  ", dt);
        #else
            Console.printf("dt= %3lu  ", dt);
        #endif
    #endif
    
    // Atualiza o estado da antena Bluetooth
    BP32.update();
    
    // Se o controle estiver conectado, lê os dados
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
        // o código exige um tempo de 100ms entre mudanças, além de ser necessário soltar o botão `+`
        // do controle.
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
        
        float linVel;  // velocidade linear
        float angVel;  // velocidade angular
        
        int RSpeed;    // PWM enviado para as rodas direitas
        int LSpeed;    // PWM enviado para as rodas esquerdas
        
        // lê as entradas
        linVel = myController->axisY();
        angVel = myController->axisRX();
        
        #if ENABLE_DEBUG_PRINT
                #if ENABLE_COLORS_IN_PRINT
                Console.printf("\033[90mraw: (lin = \033[97m% 4.0f\033[90m  ang = \033[97m% 4.0f\033[90m) \033[34m-> ", linVel, angVel);
            #else
                Console.printf("raw: (lin = % 4.0f  ang = % 4.0f) -> ", linVel, angVel);
            #endif
        #endif
        
        // normaliza para -1 e 1
        linVel = NORMALIZE_CONTROLLER_INPUT(linVel, LEFT_ANALOG_Y_MIN, LEFT_ANALOG_Y_MAX);
        angVel = NORMALIZE_CONTROLLER_INPUT(angVel, RIGHT_ANALOG_X_MIN, RIGHT_ANALOG_X_MAX);
        
        // confere se passam do limiar
        // isso ajuda a evitar os pequenos offsets que existem nos analógicos do controle
        if(abs(linVel) < VALID_INPUT_THRESHOLD)
            linVel = 0;
        if(abs(angVel) < VALID_INPUT_THRESHOLD)
            angVel = 0;
        
        // calcula a velocidade de cada roda
        // notando que a da roda esquerda é invertida
        // também é importante destacar que o modo corrigido atua aqui
        RSpeed = CALCULATE_MOTOR_SPEED(linVel, angVel,   controlMode.settings.corrected && linVel < 0);
        LSpeed = CALCULATE_MOTOR_SPEED(linVel, angVel, !(controlMode.settings.corrected && linVel < 0));
        
        // multiplicador para o modo turbo se as velocidades não forem nulas
        if (controlMode.settings.turbo && (linVel || angVel)){
            float turboMultiplier = 255.0 * std::max(abs(linVel), abs(angVel)) / std::max(abs(RSpeed), abs(LSpeed));
        
            RSpeed *= turboMultiplier;
            LSpeed *= turboMultiplier;
        }
        
        // isso garante que o valor esteja nos limites desejados
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
                Console.printf("\033[90mout: (V_R = \033[38;2;%hhu;%hhu;%hhum% 4d  \033[90mV_L = \033[38;2;%hhu;%hhu;%hhum% 4d\033[90m)\033[m",
                    RColor[0], RColor[1], RColor[2], RSpeed, LColor[0], LColor[1], LColor[2], LSpeed);
            #else
                Console.printf("rel: (lin = % 6.1f  ang = % 6.1f) -> out: (V_R = % 3d  V_L = % 3d)", linVel * 100, angVel * 100, RSpeed, LSpeed);
            #endif
        #endif

        // atualiza a velocidade dos motores
        SET_MOTOR_SPEED(RSpeed, MOTOR_R1, MOTOR_R2);
        SET_MOTOR_SPEED(LSpeed, MOTOR_L1, MOTOR_L2);
        
    }
    #if ENABLE_DEBUG_PRINT || ENABLE_TIME_DELTA_DEBUG_PRINT
    Console.println();
    #endif

    delay(20); 
}