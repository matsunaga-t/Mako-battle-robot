/*=========================================================================
 * DEFINIÇÃO DOS PINOS
 *=========================================================================
 * NÃO MUDAR SEM MOTIVO
 *=========================================================================*/
#define MOTOR_R1 18
#define MOTOR_R2 19
#define MOTOR_L1 26
#define MOTOR_L2 25
#define FURADEIRA 23
#define SOLENOIDE 21

/*=========================================================================
 * CALIBRAÇÃO DAS ENTRADAS ANALÓGICAS
 *=========================================================================
 * NÃO MUDAR SEM MOTIVO
 * ========================================================================*/
#define LEFT_ANALOG_Y_MIN    490  // (int) 
#define LEFT_ANALOG_Y_MAX   -450  // (int) 
#define RIGHT_ANALOG_X_MIN  -530  // (int) 
#define RIGHT_ANALOG_X_MAX   411  // (int) 

/*=========================================================================
 * CREDENCIAIS DO CONTROLE
 *=========================================================================
 * NÃO MUDAR SEM MOTIVO
 * ========================================================================*/
const long long controllerBTAdr[6] = {0x8c, 0xcd, 0xe8, 0x73, 0x72, 0x0e};
const short controllerVID =  1406;
const short controllerPID = 816;

/*=========================================================================
 * CONFIGURAÇÕES DE CONTROLE
 *=========================================================================*/

// valor para considerar a entrada como nula
#define VALID_INPUT_THRESHOLD .2

// multiplicador da velocidade linear 
#define LINEAR_VELOCITY  255

// multiplicador da velocidade angular 
#define ANGULAR_VELOCITY 127


// MACROS ADICIONAIS

#define CLAMP(x, lower, upper) (x < lower ? lower : x > upper ? upper : x)

#define NORMALIZE_CONTROLLER_INPUT(input, lower, upper) (CLAMP(((input) - (lower)) / ((upper) - (lower)) * 2 - 1, -1.0, 1.0))

#define ENABLE_DEBUG_PRINT 1
#define ENABLE_COLORS_IN_PRINT 1

#define ENABLE_CONNECTION_DEBUG_PRINT 1

