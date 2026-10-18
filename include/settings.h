#ifndef MAKO_ROBOT_SETTINGS_H
#define MAKO_ROBOT_SETTINGS_H

/*=========================================================================
 * DEFINIÇÃO DOS PINOS
 *=========================================================================
 * NÃO MUDAR SEM MOTIVO
 *=========================================================================*/
#define MOTOR_R1 18    // (int) pino das rodas direitas (para frente)
#define MOTOR_R2 19    // (int) pino das rodas direitas (para trás)
#define MOTOR_L1 26    // (int) pino das rodas esquerdas (para frente)
#define MOTOR_L2 25    // (int) pino das rodas esquerdas (para trás)
#define FURADEIRA 23   // (int) pino da furadeira
#define SOLENOIDE 21   // (int) pino do solenoide (provavelmente não será usado)


/*=========================================================================
 * CALIBRAÇÃO DAS ENTRADAS ANALÓGICAS
 *=========================================================================
 * NÃO MUDAR SEM MOTIVO
 * ========================================================================*/
#define LEFT_ANALOG_Y_MIN    490  // (int) menor valor Y do analógico esquerdo
#define LEFT_ANALOG_Y_MAX   -450  // (int) maior valor Y do analógico esquerdo
#define RIGHT_ANALOG_X_MIN  -530  // (int) menor valor X do analógico direito
#define RIGHT_ANALOG_X_MAX   411  // (int) maior valor X do analógico direito


/*=========================================================================
 * CREDENCIAIS DO CONTROLE
 *=========================================================================
 * NÃO MUDAR SEM MOTIVO
 * ========================================================================*/
// endereço BlueTooth do controle aceito
const u_int8_t controllerBTAdr[6] = {0x8c, 0xcd, 0xe8, 0x73, 0x72, 0x0e};

// ID do vendedor aceito (por redundância)
const u_int16_t controllerVID =  1406;

// ID do produto aceito (por redundância)
const u_int16_t controllerPID = 816;


/*=========================================================================
 * CONFIGURAÇÕES DE CONTROLE
 *=========================================================================*/
// limiar para considerar a entrada como nula
#define VALID_INPUT_THRESHOLD .2

// multiplicador da velocidade linear 
#define LINEAR_VELOCITY  255

// multiplicador da velocidade angular 
#define ANGULAR_VELOCITY 127


/*=========================================================================
 * CONFIGURAÇÕES DE IMPRESSÃO PARA DEBUG
 *=========================================================================*/
// ativa a impressão das entradas dos analógicos e saídas das rodas
#define ENABLE_IO_DEBUG_PRINT !1

// ativa a impressão do estado do robô (arma, turbo e modo de controle)
#define ENABLE_ROBOT_STATE_DEBUG_PRINT !1

// ativa logs de conexão e desconexão dos controles
#define ENABLE_CONNECTION_DEBUG_PRINT !1

// ativa impressão de `dt`, o tempo decorrido no último ciclo
#define ENABLE_TIME_DELTA_DEBUG_PRINT !1

// ativa cores nos logs de debug.
// recomendo usar apenas quando o terminal tiver suporte, porque adiciona latência
#define ENABLE_COLORS_IN_PRINT 1

#endif