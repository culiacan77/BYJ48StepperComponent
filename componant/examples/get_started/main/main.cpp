#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

Stepper::Stepper(int numberOfSteps, int motorPin1, int motorPin2, int motorPin3,
                 int motorPin4) {
  this->stepNumber = 0;                // which step the motor is on
  this->direction = 0;                 // motor direction
  this->numberOfSteps = numberOfSteps; // total number of steps for this motor
  this->lastStepTime = 0;              // timestamp in us of the last step taken

  // Arduino pins for the motor control connection:
  this->motorPin1 = motorPin1;
  this->motorPin2 = motorPin2;
  this->motorPin3 = motorPin3;
  this->motorPin4 = motorPin4;

  // setup the pins on the microcontroller:   //output car l'ESP envoi in
  // signal, c'est quelquechose qui sort. ça porte à confusion car pour moi un
  // input c'est quelquechose que l'on donne. mais là un input c'est l'écoute
  // d'un signal.
  pinMode(this->motorPin1, OUTPUT);
  pinMode(this->motorPin2, OUTPUT);
  pinMode(this->motorPin3, OUTPUT);
  pinMode(this->motorPin4, OUTPUT);
}

// Set motor speed in revs/min
void Stepper::setSpeed(long motorSpeed) {
  this->stepDelay =
      60L * 1000L / this->numberOfSteps /
      motorSpeed; // L signifie long integer. pour pouvoir contenir 60 *1000 *
                  // grand nombre. On peut dépasser la limite de int comme ça
}

void Stepper::step(int numberOfSteps) {
  int stepsLeft = abs(numberOfSteps); // abs = absolute value, est défini dans
                                      // les fonctions arduino de base

  // determine direction based on whether steps_to_mode is + or -:
  if (numberOfSteps > 0) {
    this->direction = 1;
  }
  if (numberOfSteps < 0) {
    this->direction = 0;
  }

  // decrement the number of steps, moving one step each time:
  while (stepsLeft > 0) {
    unsigned long now =
        micros(); // cest une fonction de la librairie arduino qui renvoie le
                  // temps écoulé depuis le démarrage en microseconde = 1'000
                  // millième de seconde.
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
        this->stepNumber++;
        if (this->stepNumber == this->numberOfSteps) {
          this->stepNumber = 0;
        }
      } else {
        if (this->stepNumber == 0) {
          this->stepNumber = this->numberOfSteps;
        }
        this->stepNumber--;
      }
      // decrement the steps left:
      steps_left--;
      // step the motor to step number 0, 1, 2, 3
      runStepMotor(
          this->stepNumber %
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
    digitalWrite(motor_pin_1, HIGH);
    digitalWrite(motor_pin_2, LOW);
    digitalWrite(motor_pin_3, HIGH);
    digitalWrite(motor_pin_4, LOW);
    break;
  case 1: // 0110
    digitalWrite(motor_pin_1, LOW);
    digitalWrite(motor_pin_2, HIGH);
    digitalWrite(motor_pin_3, HIGH);
    digitalWrite(motor_pin_4, LOW);
    break;
  case 2: // 0101
    digitalWrite(motor_pin_1, LOW);
    digitalWrite(motor_pin_2, HIGH);
    digitalWrite(motor_pin_3, LOW);
    digitalWrite(motor_pin_4, HIGH);
    break;
  case 3: // 1001
    digitalWrite(motor_pin_1, HIGH);
    digitalWrite(motor_pin_2, LOW);
    digitalWrite(motor_pin_3, LOW);
    digitalWrite(motor_pin_4, HIGH);
    break;
  }
}
