#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdlib> //pour pouvoir utiliser abs (valeur absolue)

constexpr gpio_num_t LED_PIN = GPIO_NUM_2;

extern "C" void app_main(void) {
  gpio_reset_pin(LED_PIN);
  gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

  while (1) {
    gpio_set_level(LED_PIN, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(LED_PIN, 0);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

////// relire et integrer

/*
 * The sequence of control signals for 4 control wires is as follows:
 *
 * Step C0 C1 C2 C3
 *    1  1  0  1  0
 *    2  0  1  1  0
 *    3  0  1  0  1
 *    4  1  0  0  1
 */

#include <BYJ48Stepper.h>
// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx ?? je peux avoir motorpin1 ou il faut la
// mêmechose que dans la déclaration du constructeur ??
Stepper::Stepper(int number_of_steps, int motor_pin_1, int motor_pin_2,
                 int motor_pin_3, int motor_pin_4) {
  this->step_number = 0; // which step the motor is on
  this->direction = 0;   // motor direction
  this->number_of_steps =
      number_of_steps;    // total number of steps for this motor
  this->lastStepTime = 0; // timestamp in us of the last step taken

  // Arduino pins for the motor control connection:
  this->motor_pin_1 = motor_pin_1;
  this->motor_pin_2 = motor_pin_2;
  this->motor_pin_3 = motor_pin_3;
  this->motor_pin_4 = motor_pin_4;

  // setup the pins on the microcontroller:   //output car l'ESP envoi in
  // signal, c'est quelquechose qui sort. ça porte à confusion car pour moi un
  // input c'est quelquechose que l'on donne. mais là un input c'est l'écoute
  // d'un signal.
  gpio_set_direction((gpio_num_t)this->motor_pin_1,
                     GPIO_MODE_OUTPUT); // comprend pas ? pas de definition ?
  gpio_set_direction((gpio_num_t)this->motor_pin_2, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)this->motor_pin_3, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)this->motor_pin_4, GPIO_MODE_OUTPUT);
}

// Set motor speed in revs/min
void Stepper::setSpeed(long motorSpeed) {
  this->stepDelay =
      60L * 1000L / this->number_of_steps /
      motorSpeed; // L signifie long integer. pour pouvoir contenir 60 *1000. On
                  // peut dépasser la limite de int comme ça
                  // + motorSpeed est grand plus step delay est court donc plus
                  // le moteur tourne vite.
}

void Stepper::step(
    int number_of_steps) { // c'est cette méthode qui fait tourner le moteur.
  int steps_left =
      std::abs(number_of_steps); // abs = absolute value, est défini dans
                                 // les fonctions arduino de base. Permet de
                                 // toujours avoir une valeur positive

  // determine direction based on whether steps_to_mode is + or -:
  if (number_of_steps > 0) {
    this->direction = 1;
  }
  if (number_of_steps < 0) {
    this->direction = 0;
  }

  // decrement the number of steps, moving one step each time:
  while (steps_left > 0) {
    unsigned long now =
        esp_timer_get_time(); // cest une fonction de la librairie esp idf qui
                              // renvoie le temps écoulé depuis le démarrage en
                              // microseconde = 1'000 millième de seconde.
    // move only if the appropriate delay has passed:
    if (now - this->lastStepTime >=
        this->stepDelay) // si le temps écoulé depuis le démarage - le temps
                         // depuis le dernier step est plus grand ou égale au
                         // temps entre 2 step
    {
      // get the timeStamp of when you stepped:
      this->lastStepTime = now;
      // increment or decrement the step number,
      // depending on direction:
      if (this->direction == 1) {
        this->step_number++;
        if (this->step_number == this->number_of_steps) {
          this->step_number = 0;
        }
      } else {
        if (this->step_number == 0) {
          this->step_number = this->number_of_steps;
        }
        this->step_number--;
      }
      // decrement the steps left:
      steps_left--;
      // step the motor to step number 0, 1, 2, 3
      runStepMotor(
          this->step_number %
          4); // le modulo garanti qu'on reste dans l'intervale [0, 3]. il
              // regarde quel est le multiple de quatre le plus grand par lequel
              // on peut diviser le nombre et nous retourne le reste.
    }
  }
}

// Moves the motor forward or backwards.
void Stepper::runStepMotor(
    int thisStep) // this step est l'argument qu'on a donné à runStepMotor lors
                  // de son appel (expliqué juste en dessus avec le modulo)
{
  switch (thisStep) {
  case 0: // 1010
    gpio_set_level(motor_pin_1, 1);
    gpio_set_level(motor_pin_2, 0);
    gpio_set_level(motor_pin_3, 1);
    gpio_set_level(motor_pin_4, 0);
    break;
  case 1: // 0110
    gpio_set_level(motor_pin_1, 0);
    gpio_set_level(motor_pin_2, 1);
    gpio_set_level(motor_pin_3, 1);
    gpio_set_level(motor_pin_4, 0);
    break;
  case 2: // 0101
    gpio_set_level(motor_pin_1, 0);
    gpio_set_level(motor_pin_2, 1);
    gpio_set_level(motor_pin_3, 0);
    gpio_set_level(motor_pin_4, 1);
    break;
  case 3: // 1001
    gpio_set_level(motor_pin_1, 1);
    gpio_set_level(motor_pin_2, 0);
    gpio_set_level(motor_pin_3, 0);
    gpio_set_level(motor_pin_4, 1);
    break;
  }
}
