/*
 * IIDX Controller
 * This code is developed for a Teense 3.1 by kiyoshigawa
 * It is designed to be a fully functional input device for a beatmania IIDX custom controller.
 * It is designed to run as a joystick, and as such this options must be selected as part of the board type.
 */

//Uncomment ENCODER_DEBUG to get serial feedback on encoder position.
//#define ENCODER_DEBUG
//Uncomment KEYPRESS_DEBUG to get feedback on key presses and releases sent
//#define KEYPRESS_DEBUG

//#define JOYSTICK_MODE
#define KEYBOARD_MODE

#include <Encoder.h>
#include <Bounce.h>

#define POSITIVE 1
#define STOPPED 0
#define NEGATIVE -1

//number of ms to ignore changes in button state after a change
#define BOUNCE_TIME 5

//number of bounce objects to iterate through
#define NUM_BUTTONS 9

//this is the minimum number of steps before the motion of the encoders registers. Decrease to make the wheel more sensitive, increase to make the wheel less sensitive. If this number is too low, the wheel may register hits when it is not being moved.
#define STEP_THRESHOLD 10

//this is the number of miliseconds from the last movement that will register as still pressing a key for scrolling purposes. If the wheel does not move for MOVE_TIMEOUT ms, it will stop pressing up or down. If the wheel changes direction before MOVE_TIMEOUT, it will still send the alternate direction as soon as the change is detected.
#define MOVE_TIMEOUT 100

#define ENCODER_SENSITIVITY (double) 16.0

//array of button pins
//                   Z   S   X   D   C   F   V   G   B
int p1_buttons[] = { 0,  7,  4, 16,  3,  2,  1,  5,  6};
int p1_leds[]    = {23, 22, 21, 21, 20, 19, 18, 17, 24};

#ifdef KEYBOARD_MODE
int keyboard_map[] = {KEY_Z, KEY_S, KEY_X, KEY_D, KEY_C, KEY_F, KEY_V, KEY_G, KEY_B};
#endif

//if encoders are running backwards, swap pin a and b.
int left_knob_pin_a = 11;
int left_knob_pin_b = 12;
int right_knob_pin_a = 9;
int right_knob_pin_b = 10;

//init bounce objects for all buttons;
Bounce p1_b1 = Bounce(p1_buttons[0], BOUNCE_TIME);
Bounce p1_b2 = Bounce(p1_buttons[1], BOUNCE_TIME);
Bounce p1_b3 = Bounce(p1_buttons[2], BOUNCE_TIME);
Bounce p1_b4 = Bounce(p1_buttons[3], BOUNCE_TIME);
Bounce p1_b5 = Bounce(p1_buttons[4], BOUNCE_TIME);
Bounce p1_b6 = Bounce(p1_buttons[5], BOUNCE_TIME);
Bounce p1_b7 = Bounce(p1_buttons[6], BOUNCE_TIME);
Bounce p1_b8 = Bounce(p1_buttons[7], BOUNCE_TIME);
Bounce p1_b9 = Bounce(p1_buttons[8], BOUNCE_TIME);

//array of bounce objects to iterate through
Bounce button_array[] = { p1_b1, p1_b2, p1_b3, p1_b4, p1_b5, p1_b6, p1_b7, p1_b8, p1_b9 };

//init encoder objects
Encoder knobLeft(left_knob_pin_a, left_knob_pin_b);
Encoder knobRight(right_knob_pin_a, right_knob_pin_b);

//global variables used below
long position_left  = 0;
long position_right = 0;
int x_axis = 0;
int y_axis = 0;
int direction_left = STOPPED;
int direction_right = STOPPED;
unsigned long last_left_move_time = 0;
unsigned long last_right_move_time = 0;

void setup() {
  Serial.begin(9600);
  #ifdef ENCODER_DEBUG
    Serial.println("Two Knobs Encoder Test:");
  #endif
  #ifdef KEYPRESS_DEBUG
    Serial.println("Keypress Testing:");
  #endif
  //num_buttons over 2 because I have 2 button arrays and each player has half the buttons
  //for(int i=0; i<(NUM_BUTTONS/2); i++){
  for(int i = 0; i < NUM_BUTTONS; i++){
    pinMode(p1_buttons[i], INPUT_PULLUP);
    pinMode(p1_leds[i], OUTPUT);
  }

#ifdef JOYSTICK_MODE
  //disable other joystick functions
  Joystick.Z(512);
  Joystick.Zrotate(512);
  Joystick.sliderLeft(512);
  Joystick.sliderRight(512);
  Joystick.X(-1);
#endif
}

void loop() {
  //normal controller operation - keypresses send joystick commands via USB, lighting control runs based on mode last selected.
  update_encoders();
  update_buttons();
  encoder_key_press();
}

// check state of buttons for change (bounced) and update joystick status based on this.
void update_buttons(){
  for (int i = 0; i < NUM_BUTTONS; i++) {
    //check for updates on each button
    if (button_array[i].update()) {
      //if the button was released set it to 0
      if (button_array[i].risingEdge()) {
        #ifdef JOYSTICK_MODE
        Joystick.button(i+1, 0); // +1 because joystick buttons are from 1-32, not 0-31.
        #endif
        #ifdef KEYBOARD_MODE
        Keyboard.release(keyboard_map[i]);
        #endif

        #ifdef KEYPRESS_DEBUG
        Serial.print("Released ");
        Serial.println(i+1);
        #endif

        // Turn off the corresponding LED
        digitalWrite(p1_leds[i], LOW);
      }
      //otherwise set it to pressed
      else if (button_array[i].fallingEdge()) {
        #ifdef JOYSTICK_MODE
        Joystick.button(i+1, 1); // +1 because joystick buttons are from 1-32, not 0-31.
        #endif
        #ifdef KEYBOARD_MODE
        Keyboard.press(keyboard_map[i]);
        #endif

        #ifdef KEYPRESS_DEBUG
        Serial.print("Pressed ");
        Serial.println(i+1);
        #endif

        // Turn on the corresponding LED
        digitalWrite(p1_leds[i], HIGH);
      }
    }
  }
}

//this will use the timing from the update_encoders() function to determine if the joystick should be pressed up, down, or not at all. Note that the joystick mapped buttons for up and down will be NUM_BUTTONS + 1, 2, 3, or 4, as the first NUM_BUTTONS buttons will be used by the keys, start, and select buttons.
void encoder_key_press(){
  if(direction_left == POSITIVE){
    //press left up button and release left down button
    Joystick.button(NUM_BUTTONS+1, 1);
    Joystick.button(NUM_BUTTONS+2, 0);
  }
  else if(direction_left == NEGATIVE){
    //press left down button and release left up button
    Joystick.button(NUM_BUTTONS+1, 0);
    Joystick.button(NUM_BUTTONS+2, 1);
  }
  else if(direction_left == STOPPED){
    //release both left up and left down buttons
    Joystick.button(NUM_BUTTONS+1, 0);
    Joystick.button(NUM_BUTTONS+2, 0);
  }
  if(direction_right == POSITIVE){
    //press left up button and release left down button
    Joystick.button(NUM_BUTTONS+3, 1);
    Joystick.button(NUM_BUTTONS+4, 0);
  }
  else if(direction_right == NEGATIVE){
    //press left down button and release left up button
    Joystick.button(NUM_BUTTONS+3, 0);
    Joystick.button(NUM_BUTTONS+4, 1);
  }
  else if(direction_right == STOPPED){
    //release both left up and left down buttons
    Joystick.button(NUM_BUTTONS+3, 0);
    Joystick.button(NUM_BUTTONS+4, 0);
  }
}

#ifdef JOYSTICK_MODE
void update_joystick_x(long position) {
  x_axis = position % 1024;
  if (x_axis < 0) {
    x_axis = 1024 + x_axis;
  }

  Joystick.X(x_axis);
}

void update_joystick_y(long position) {
  y_axis = position % 1024;
  if (y_axis < 0) {
    y_axis = 1024 + y_axis;
  }

  Joystick.Y(y_axis);
}
#endif

//this will update the time variables showing when the encoders last changed position.
void update_encoders(){
  long new_left, new_right, diff_left, diff_right;
  //unsigned long current_time = millis();
  new_left = knobLeft.read() / ENCODER_SENSITIVITY;
  new_right = knobRight.read() / ENCODER_SENSITIVITY;

  #ifdef JOYSTICK_MODE
  update_joystick_x(new_left);
  update_joystick_y(new_right);
  #endif

  #ifdef KEYBOARD_MODE
  diff_left = new_left - position_left;
  diff_right = new_right - position_right;

  Mouse.move(diff_left, diff_right);
  #endif

  /*
  //if going positive direction
  if (new_left > (position_left+STEP_THRESHOLD) ) {
	  direction_left = POSITIVE;
	  last_left_move_time = current_time;
    print_encoder_position();
  }
  //if going negative direction
  else if (new_left < (position_left-STEP_THRESHOLD) ) {
    direction_left = NEGATIVE;
    last_left_move_time = current_time;
    print_encoder_position();
  }
  //if it has not changed position since MOVE_TIMEOUT, stop sending either up or down
  else if ((current_time - last_left_move_time) > MOVE_TIMEOUT){
    direction_left = STOPPED;
  }
  
  //if going positive direction
  if (new_right > (position_right+STEP_THRESHOLD) ) {
    direction_right = POSITIVE;
    last_right_move_time = current_time;
    print_encoder_position();
  }
  //if going negative direction
  else if (new_right < (position_right-STEP_THRESHOLD) ) {
    direction_right = NEGATIVE;
    last_right_move_time = current_time;
    print_encoder_position();
  }
  //if it has not changed position since MOVE_TIMEOUT, stop sending either up or down
  else if ((current_time - last_right_move_time) > MOVE_TIMEOUT){
    direction_right = STOPPED;
  }
  */

  position_left = new_left;
  position_right = new_right;
	
  // if the value approaches the max, gracefully reset values to 0.
  if (abs(position_left) > 2000000000 || abs(position_right) > 2000000000) {
    knobLeft.write(0);
    position_left = 0;
    knobRight.write(0);
    position_right = 0;
  }
}

void print_encoder_position(){
	#ifdef ENCODER_DEBUG  
    Serial.print("Left = ");
    Serial.print(position_left);
    Serial.print(", Right = ");
    Serial.print(position_right);
    Serial.print(", X = ");
    Serial.print(x_axis);
    Serial.print(", Y = ");
    Serial.print(y_axis);
    Serial.println();
  #endif
}

