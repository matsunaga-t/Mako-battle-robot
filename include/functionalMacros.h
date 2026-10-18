#ifndef MAKO_ROBOT_MACRO_FUNCTIONS_H
#define MAKO_ROBOT_MACRO_FUNCTIONS_H

#include "settings.h"

/* Contém macros que agem como funções
 * talvez substituir por funções ou classes no futuro
 */

 /**
  * @brief Esse macro serve para "clipar" um valor entre dois outros.
  * 
  * @param x o valor a ser clipado
  * @param lower o limite inferior
  * @param upper o limite superior
  * 
  * @returns `x`, se `x` estiver entre `lower` e `upper`, caso contrário, retornará `lower` ou `upper`.
*/
#define CLAMP(x, lower, upper) (x < lower ? lower : x > upper ? upper : x)


/**
 * @brief Esse macro normaliza a entrada do controle para um valor entre -1 e 1 com base nos limites 
 * inferior e superior.
 * 
 * @param input o valor de entrada do controle
 * @param lower o limite inferior da entrada (aquele que deve ser interpretado como -1)
 * @param upper o limite superior da entrada (aquele que deve ser interpretado como 1)
 * 
 * @returns o valor da entrada normalizada, entre -1 e 1. 
 */
#define NORMALIZE_CONTROLLER_INPUT(input, lower, upper) (CLAMP(((input) - (lower)) / ((upper) - (lower)) * 2 - 1, -1.0, 1.0))


/**
 * @brief Esse macro atualiza a velocidade das rodas com base na velocidade dada.
 * 
 * @param speed a velocidade a ser aplicada, entre -255 e 255
 * @param forward o pino que indica a velocidade para frente na ponte H
 * @param backward o pino que indica a velocidade para trás na ponte H
 */
#define SET_MOTOR_SPEED(speed, forward, backward) \
    if(speed >= 0){\
        analogWrite(forward, speed);\
        analogWrite(backward, 0);\
    }\
    else{\
        analogWrite(forward, 0);\
        analogWrite(backward, -speed);\
    }

    
/**
 * @brief Esse macro calcula a velocidade de uma das rodas com a fórmula clássica,
 * mas também permite inverter a velocidade angular, algo que ocorre quando se calcula 
 * a velocidade das duas rodas. Um dos lados sempre deve ser invertido.
 * 
 * @param linVel a velocidade linear do robô
 * @param angVel a velocidade angular do robô
 * @param invert um `bool` indicando se deve inverter a velocidade angular antes de somá-la
 * 
 * @returns a velocidade angular da roda 
 */
#define CALCULATE_MOTOR_SPEED(linVel, angVel, invert) (LINEAR_VELOCITY * linVel + (invert ? -1 : 1) * ANGULAR_VELOCITY * angVel) / 2;
#endif