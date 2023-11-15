#include <Arduino.h>
#include "FastLED.h"

#define NUM_LEDS 5
#define LED_PIN 6
#define LED_BLINK_PAUSE 850
#define HOLD_TIME 3000
#define BLINK_TIME_1 1000
#define BLINK_TIME_2 2000
#define BLINK_TIME_5 5000
#define BLINK_TIME_10 10000
#define CYCLE_TIME 40
#define IDLE_TIME 30000

#define BRIGHTNESS  255

#define Sgo 2   // Pin button Sgo
#define Sup 3   // Pin button Sup
#define Sdown 4 // Pin button Sdown

CRGB leds[NUM_LEDS]; // Define leds array

// State machine states
typedef enum{   
    MAIN_INIT,
    MAIN_START,
    MAIN_CONFIG,
    MAIN_PAUSE,
    MAIN_IDLE,
    COUNTDOWN_INIT,
    COUNTDOWN_START,
    COUNTDOWN_PAUSE,
    COUNTDOWN_BLINK,
    COUNTDOWN_CONFIG,
    COUNTDOWN_IDLE,
    AUX_INIT,
    AUX_HIGH,
    AUX_LOW,
    CONFIG_INIT,
    CONFIG_1,
    CONFIG_2,
    CONFIG_3,
    CONFIG_IDLE,
} state_t;


// Finite State Machine
typedef struct {
    state_t state, new_state;

    // tes - time entering state
    // tis - time in state
    unsigned long tes, tis, interval;
} fsm_t;


// Inputs
bool Sgo_state, Sup_state, Sdown_state;


// State machines declaration
fsm_t main_fsm, countdown_fsm, aux_fsm, config_fsm, setup_fsm;


// Read rising edge of inputs
bool Sgo_state_old = HIGH;
bool Sup_state_old = LOW;
bool Sdown_state_old = HIGH;

// Button flags
bool Sgo_pressed = false;
bool Sup_pressed = false;
bool Sdown_pressed = false;
bool Sup_hold = false;
bool Sup_hold_old = false;


// Blink variables
int blinkMotion = 0;
int fadeMotion = 0;

// Countdown aux variables
bool first;
bool pause;


// State aux variables
int low_leds = 0;
int aux = 0;
int low_leds_old = 0;
state_t old_state_countdown_config, old_state_countdown_idle, old_state_main_config, old_state_MAIN_IDLE, old_state_config;


// Time variables
unsigned long hold = 0, start_hold = 0, blink_old = 0;
unsigned long cycle_time = 0, pause_time = 0, idle_time = 0;
unsigned long idle_button = 0;


// Config variables
int transition = 0;
int blink_time = BLINK_TIME_2;
struct CRGB colour = CRGB::White;
struct CHSV colour_HSV(0,0,BRIGHTNESS);


// loop cycle time
unsigned long last_now, last_action;


// Set new state
void set_state(fsm_t& fsm, state_t new_state)
{
    if (fsm.state != new_state) {  // if the state chnanged tis is reset
        fsm.state = new_state;
        fsm.tes = millis();
        fsm.tis = 0;
    }
}

// Read rising edge of inputs
void read_inputs(unsigned long now)
{
    Sgo_state = digitalRead(Sgo);
    Sup_state = digitalRead(Sup);
    Sdown_state = digitalRead(Sdown);

    if (Sgo_state == LOW && Sgo_state_old == HIGH)
    {
        Serial.println("Sgo");
        Sgo_pressed = true;
    }
    else
    {
        Sgo_pressed = false;
    }

    if (Sup_state == HIGH && Sup_state_old == LOW)
    {
        if(!Sup_hold_old){
            Serial.println("Sup");
            Sup_pressed = true;
        }
        else if(Sup_hold_old){
            Sup_pressed = false;
            Sup_hold_old = false;
        }
    }
    else
    {
        Sup_pressed = false;
    }

    if (Sdown_state == LOW && Sdown_state_old == HIGH)
    {
        Serial.println("Sdown");
        Sdown_pressed = true;
    }
    else
    {
        Sdown_pressed = false;
    }

    if(Sup_state == LOW){

        if(start_hold == 0){
            start_hold = now;
        }
        else{
            hold = now - start_hold;
        }

        if(hold >= HOLD_TIME){
            Sup_hold = true;
            Sup_hold_old = true;
            hold = 0;
            start_hold = 0;
            Serial.println("Sup_hold");
        }
    }
    else{
        Sup_hold = false;
        hold = 0;
        start_hold = 0;
    }

    if(Sgo_pressed || Sup_pressed || Sdown_pressed || Sup_hold){
        idle_button = 0;
        last_action = now;
    }
    else {
        idle_button = now - last_action;
    }

    Sgo_state_old = Sgo_state;
    Sup_state_old = Sup_state;
    Sdown_state_old = Sdown_state;
}


//######## MAIN FSM ########


// Update main state machine
void update_main_fsm(unsigned long now)
{
    switch (main_fsm.state)
    {
    case MAIN_INIT:
        if (Sgo_pressed)
        {
            set_state(main_fsm, MAIN_START);
            Serial.println("MAIN_START");
        
        }
        if(Sup_hold){
            set_state(main_fsm, MAIN_CONFIG);
            Serial.println("MAIN_CONFIG");
            Sup_hold = false;
            old_state_main_config = MAIN_INIT;
        }
        if(main_fsm.tis >= IDLE_TIME && idle_button >= IDLE_TIME){
            set_state(main_fsm, MAIN_IDLE);
            Serial.println("MAIN_IDLE");
            old_state_MAIN_IDLE = MAIN_INIT;
        }
        
        break;
    
    case MAIN_START:

        if (Sdown_pressed)
        {
            set_state(main_fsm, MAIN_PAUSE);
            Serial.println("MAIN_PAUSE");
        }
        if(Sup_hold){
            set_state(main_fsm, MAIN_CONFIG);
            Serial.println("MAIN_CONFIG");
            Sup_hold = false;
            old_state_main_config = MAIN_START;
        }
        if(main_fsm.tis >= IDLE_TIME && countdown_fsm.state == COUNTDOWN_BLINK && countdown_fsm.tis >= IDLE_TIME  && idle_button >= IDLE_TIME ){
            set_state(main_fsm, MAIN_IDLE);
            Serial.println("MAIN_IDLE");
            old_state_MAIN_IDLE = MAIN_START;
        }
        break;

    case MAIN_PAUSE:

        if (Sdown_pressed)
        {
            set_state(main_fsm, MAIN_START);
            Serial.println("MAIN_START");
        }
        if(Sup_hold){
            set_state(main_fsm, MAIN_CONFIG);
            Serial.println("MAIN_CONFIG");
            Sup_hold = false;
            old_state_main_config = MAIN_PAUSE;
        }
        if(main_fsm.tis >= IDLE_TIME && idle_button >= IDLE_TIME){
            set_state(main_fsm, MAIN_IDLE);
            Serial.println("MAIN_IDLE");
            old_state_MAIN_IDLE = MAIN_PAUSE;
        }
        break;
    

    case MAIN_CONFIG:
        if(Sup_hold)
        {
            set_state(main_fsm, old_state_main_config);
            Serial.print(old_state_main_config);
            Serial.println("MAIN_OLD_STATE");
            Sup_hold = false;
            for(int i=0; i<NUM_LEDS; i++){
                leds[i] = CRGB::Black;
            }
            FastLED.show();
        }
        if(main_fsm.tis >= IDLE_TIME  && idle_button >= IDLE_TIME){
            set_state(main_fsm, MAIN_IDLE);
            Serial.println("MAIN_IDLE");
            old_state_MAIN_IDLE = MAIN_CONFIG;
        }
        break;

    case MAIN_IDLE:

        if(Sup_pressed){
            set_state(main_fsm, old_state_MAIN_IDLE);
            Serial.print(old_state_MAIN_IDLE);
            main_fsm.tis = 0;
            Serial.println("MAIN_OLD_STATE");
            Sup_pressed = false;
            for(int i=0; i<NUM_LEDS; i++){
                leds[i] = CRGB::Black;
            }
            FastLED.show();
        }
        else if(Sdown_pressed){
            set_state(main_fsm, old_state_MAIN_IDLE);
            main_fsm.tis = 0;
            Serial.print(old_state_MAIN_IDLE);
            Serial.println("MAIN_OLD_STATE");
            Sdown_pressed = false;
            for(int i=0; i<NUM_LEDS; i++){
                leds[i] = CRGB::Black;
            }
            FastLED.show();
        }
        else if(Sgo_pressed){
            set_state(main_fsm, old_state_MAIN_IDLE);
            Serial.print(old_state_MAIN_IDLE);
            main_fsm.tis = 0;
            Serial.println("MAIN_OLD_STATE");
            Sgo_pressed = false;
            for(int i=0; i<NUM_LEDS; i++){
                leds[i] = CRGB::Black;
            }
            FastLED.show();
        }

        break;
    }
}

// Update outputs
void update_main_outputs(unsigned long now)
{ 
    switch (main_fsm.state)
    {
    case MAIN_INIT:

        break;
    
    case MAIN_START:

        break;
    case MAIN_PAUSE:

        break;
    case MAIN_IDLE:

        for (int j = 0; j < 255; j++) {
            for (int i = 0; i < NUM_LEDS; i++) {
                leds[i] = CHSV(i - (j * 2), 255, BRIGHTNESS);
            }
            FastLED.show();
        }
        break;
    }
}



//######## COUNTDOWN FSM ########


//  Update countdown state machine
void update_countdown_fsm(unsigned long now)
{
    switch (countdown_fsm.state)
    {
    case COUNTDOWN_INIT:
        if (main_fsm.state == MAIN_START)
        {
            set_state(countdown_fsm, COUNTDOWN_START);
            Serial.println("COUNTDOWN_START");
            low_leds = 0;
            low_leds_old = 0;
            aux=0;
            blinkMotion = 0;
            blink_old = now;
            first = true;
        }
        if(main_fsm.state == MAIN_CONFIG){
            set_state(countdown_fsm, COUNTDOWN_CONFIG);
            Serial.println("COUNTDOWN_CONFIG");
            old_state_countdown_config = COUNTDOWN_INIT;
        }
        if(main_fsm.state == MAIN_IDLE){
            set_state(countdown_fsm, COUNTDOWN_IDLE);
            old_state_countdown_idle = COUNTDOWN_INIT;
            Serial.println("COUNTDOWN_IDLE");
        }
        break;
    
    case COUNTDOWN_START:

        if(main_fsm.state == MAIN_PAUSE)
        {
            set_state(countdown_fsm, COUNTDOWN_PAUSE);
            low_leds_old = low_leds;
            Serial.println("COUNTDOWN_PAUSE");
        }
        else if (low_leds == NUM_LEDS)
        {
            set_state(countdown_fsm, COUNTDOWN_BLINK);
            aux = 0;
            Serial.println("COUNTDOWN_BLINK");
        }
        if(main_fsm.state == MAIN_CONFIG){
            set_state(countdown_fsm, COUNTDOWN_CONFIG);
            low_leds_old = low_leds;
            old_state_countdown_config = COUNTDOWN_START;
            Serial.println("COUNTDOWN_CONFIG");
        }
        if(Sgo_pressed){
            low_leds = 0;
            low_leds_old = 0;
            aux=0;
            blinkMotion = 0;
            blink_old = now;
            first = true;
        }
        break;

    case COUNTDOWN_PAUSE:
    
        if(main_fsm.state == MAIN_START)
        {
            pause_time = countdown_fsm.tis;
            set_state(countdown_fsm, COUNTDOWN_START);
            low_leds = low_leds_old;
            pause = true;
            aux=0;
            Serial.println("COUNTDOWN_START");
        }
        if(main_fsm.state == MAIN_CONFIG){
            set_state(countdown_fsm, COUNTDOWN_CONFIG);
            old_state_countdown_config = COUNTDOWN_PAUSE;
            Serial.println("COUNTDOWN_CONFIG");
        }
        if(main_fsm.state == MAIN_IDLE){
            set_state(countdown_fsm, COUNTDOWN_IDLE);
            old_state_countdown_idle = COUNTDOWN_PAUSE;
            Serial.println("COUNTDOWN_IDLE");
        }
        break;

    case COUNTDOWN_BLINK:
        if (Sdown_pressed)
        {
            set_state(main_fsm, MAIN_INIT);
            set_state(countdown_fsm, COUNTDOWN_INIT);
            set_state(aux_fsm, AUX_INIT);
            Serial.println("MAIN_INIT");
            Serial.println("COUNTDOWN_INIT");
            Serial.println("AUX_INIT");
            for(int i=0; i<NUM_LEDS; i++){
                leds[i] = CRGB::Black;
            }
            FastLED.show();
        }
        if (Sgo_pressed){
            set_state(countdown_fsm, COUNTDOWN_START);
            Serial.println("COUNTDOWN_START");
            low_leds = 0;
            low_leds_old = 0;
            aux=0;
            blinkMotion = 0;
            blink_old = now;
            first = true;
        }
        if(main_fsm.state == MAIN_CONFIG){
            set_state(countdown_fsm, COUNTDOWN_CONFIG);
            old_state_countdown_config = COUNTDOWN_BLINK;
            Serial.println("COUNTDOWN_CONFIG");
        }
        if(main_fsm.state == MAIN_IDLE){
            set_state(countdown_fsm, COUNTDOWN_IDLE);
            Serial.println("COUNTDOWN_IDLE");
            old_state_countdown_idle = COUNTDOWN_BLINK;
        }
        break;

    case COUNTDOWN_CONFIG:
        if(main_fsm.state == MAIN_IDLE){
            set_state(countdown_fsm, COUNTDOWN_IDLE);
            Serial.println("COUNTDOWN_IDLE");
            old_state_countdown_idle = COUNTDOWN_CONFIG;
        }
        else if(main_fsm.state != MAIN_CONFIG)
        {
            pause_time = countdown_fsm.tis;
            set_state(countdown_fsm, old_state_countdown_config);
            Serial.print(old_state_countdown_config);
            if(old_state_countdown_config == COUNTDOWN_START){
                low_leds = low_leds_old;
                pause = true;
                aux=0;
            }
            Serial.println("COUNTDOWN_OLDSTATE");
        }
        break;

    case COUNTDOWN_IDLE:

        if(main_fsm.state != MAIN_IDLE){
            set_state(countdown_fsm, old_state_countdown_idle);
            Serial.print(old_state_countdown_idle);
            Serial.println("COUNTDOWN_OLDSTATE");
        }
    }
}

// Update countdown outputs
void update_countdown_outputs(unsigned long now)
{
    switch (countdown_fsm.state)
    {
    case COUNTDOWN_INIT:

        break;
    
    case COUNTDOWN_START:

        if(first){
            cycle_time = now;
            first = false;
        }
        if(pause){
            cycle_time = cycle_time + pause_time + 2*CYCLE_TIME;
            low_leds = low_leds_old;
            pause = false;
        }

        for(int i=0; i < low_leds; i++)
        {
            leds[i] = CRGB::Black;
        }
        for(int i=low_leds; i < NUM_LEDS; i++)
        {
            leds[i] = colour;
        }

        if(transition == 1){

            if((now - blink_old) >= blink_time/2){
            
                if(blinkMotion>=6){
                    blinkMotion = 0;
                }

                if(blinkMotion<3){
                    leds[low_leds] = CRGB::Black;
                    blinkMotion++;
                }
                else if(blinkMotion<6){
                    leds[low_leds] = colour;
                    blinkMotion++;
                }
            }

        }
        else if(transition == 2){
            if((now - blink_old) > blink_time){
                blink_old = now;
            }
            else if((now - blink_old) < 0){
                blink_old = 0;
            }
            fadeMotion = map((now - blink_old), 0, blink_time, BRIGHTNESS, 0);
            leds[low_leds].setHSV(colour_HSV.hue, colour_HSV.saturation, fadeMotion);
        }

        FastLED.show();

        if(Sup_pressed && low_leds < NUM_LEDS && low_leds > 0){
            low_leds--;
            break;
        }

        if((now - cycle_time) >= blink_time){
            low_leds++;
            cycle_time = now;
            blink_old = now;
            Serial.println(low_leds);
        }

        Serial.print((now - cycle_time));
        Serial.print(" ");
        Serial.println(blink_time);
        
        break;

    case COUNTDOWN_PAUSE:
    
        if(Sup_pressed && low_leds < NUM_LEDS && low_leds > 0){
            low_leds_old--;
            low_leds--;
            break;
        }

        break;

    case COUNTDOWN_BLINK:

        if(aux <= 10){
            for(int i=0; i< NUM_LEDS; i++)
            {
                leds[i] = CRGB::Red;
            }
            aux++;
        }
        else if(aux <= 20){
            for(int i=0; i< NUM_LEDS; i++)
            {
                leds[i] = CRGB::Black;
            }
            aux++;
        }
        else{
            aux = 0;
        }

        FastLED.show();

        break;
    }
}



//######## AUX FSM ########


void update_aux_fsm(unsigned long now)
{
    switch (aux_fsm.state)
    {
    case AUX_INIT:
        if(countdown_fsm.state == COUNTDOWN_PAUSE)
        {
            set_state(aux_fsm, AUX_HIGH);
            Serial.println("AUX_HIGH");
        }
        break;
    
    case AUX_HIGH:
        if(main_fsm.state == MAIN_IDLE){
            set_state(aux_fsm, AUX_INIT);
            Serial.println("AUX_INIT");
        }

        if(countdown_fsm.state == COUNTDOWN_START || countdown_fsm.state == COUNTDOWN_CONFIG)
        {
            set_state(aux_fsm, AUX_INIT);
            Serial.println("AUX_INIT");
        }
        else if(aux_fsm.tis >= LED_BLINK_PAUSE/2)
        {
            set_state(aux_fsm, AUX_LOW);
            Serial.println("AUX_LOW");
        }
        break;

    case AUX_LOW:
        if(main_fsm.state == MAIN_IDLE){
            set_state(aux_fsm, AUX_INIT);
            Serial.println("AUX_INIT");
        }

        if(countdown_fsm.state == COUNTDOWN_START || countdown_fsm.state == COUNTDOWN_CONFIG)
        {
            set_state(aux_fsm, AUX_INIT);
            Serial.println("AUX_INIT");
        }
        else if(aux_fsm.tis >= LED_BLINK_PAUSE/2)
        {
            set_state(aux_fsm, AUX_HIGH);
            Serial.println("AUX_HIGH");
        }
        break;
    }
}

void update_aux_outputs(unsigned long now)
{
    switch (aux_fsm.state)
    {
    case AUX_INIT:
        break;
    
    case AUX_HIGH:

        for(int i=0; i<low_leds; i++)
        {
            leds[i] = CRGB::Black;
        }
        for(int i=low_leds; i<NUM_LEDS; i++)
        {
            leds[i] = colour;
        }
        FastLED.show();
        break;

    case AUX_LOW:

        for(int i=0; i<NUM_LEDS; i++)
        {
            leds[i] = CRGB::Black;
        }
        FastLED.show();
        break;
    }
}


//######## CONFIG FSM ########

void update_config_fsm(unsigned long now)
{
    switch (config_fsm.state)
    {
    case CONFIG_INIT:
        if(main_fsm.state == MAIN_CONFIG)
        {   
            set_state(config_fsm, CONFIG_1);
            config_fsm.interval = 0;
            blink_old = now;
            Serial.println("CONFIG_SET");
        }
        if(main_fsm.state == MAIN_IDLE){
            set_state(config_fsm, CONFIG_IDLE);
            Serial.println("CONFIG_IDLE");
            old_state_config = CONFIG_INIT;
        }
        break;
    
    case CONFIG_1:
        if(main_fsm.state == MAIN_IDLE){
            set_state(config_fsm, CONFIG_IDLE);
            Serial.println("CONFIG_IDLE");
            old_state_config = CONFIG_1;
        }
        else if(main_fsm.state != MAIN_CONFIG)
        {
            set_state(config_fsm, CONFIG_INIT);
            Serial.println("CONFIG_INIT");
        }
        if(Sup_pressed)
        {
            set_state(config_fsm, CONFIG_2);
            blink_old = now;
            blinkMotion = 0;
            Serial.println("CONFIG_2");
        }
        if(Sdown_pressed)
        {
            if(blink_time == BLINK_TIME_1)
            {
                blink_time = BLINK_TIME_2;
            }
            else if(blink_time == BLINK_TIME_2)
            {
                blink_time = BLINK_TIME_5;
            }
            else if(blink_time == BLINK_TIME_5)
            {
                blink_time = BLINK_TIME_10;
            }
            else if(blink_time == BLINK_TIME_10)
            {
                blink_time = BLINK_TIME_1;
            }
        }
        
        break;

    case CONFIG_2:
        if(main_fsm.state == MAIN_IDLE){
            set_state(config_fsm, CONFIG_IDLE);
            Serial.println("CONFIG_IDLE");
            old_state_config = CONFIG_2;
        }
        else if(main_fsm.state != MAIN_CONFIG)
        {
            set_state(config_fsm, CONFIG_INIT);
            Serial.println("CONFIG_INIT");
        }
        if(Sup_pressed)
        {
            set_state(config_fsm, CONFIG_3);
            blink_old = now;
            Serial.println("CONFIG_3");
        }
        if(Sdown_pressed)
        {
            if(transition == 0){
                transition = 1;
            }
            else if(transition == 1){
                transition = 2;
            }
            else if(transition == 2){
                transition = 0;
            }
        }
        
        break;

    case CONFIG_3:
        if(main_fsm.state == MAIN_IDLE){
            set_state(config_fsm, CONFIG_IDLE);
            Serial.println("CONFIG_IDLE");
            old_state_config = CONFIG_3;
        }
        else if(main_fsm.state != MAIN_CONFIG)
        {
            set_state(config_fsm, CONFIG_INIT);
            Serial.println("CONFIG_INIT");
        }
        if(Sup_pressed)
        {
            set_state(config_fsm, CONFIG_1);
            blink_old = now;
            Serial.println("CONFIG_1");
        }
        if(Sdown_pressed){
            if(colour == CRGB::White){
                colour = CRGB::Violet;
                colour_HSV = rgb2hsv_approximate(colour);
            }
            else if(colour == CRGB::Violet){
                colour = CRGB::Blue;
                colour_HSV = rgb2hsv_approximate(colour);
            }
            else if(colour == CRGB::Blue){
                colour = CRGB::Cyan;
                colour_HSV = rgb2hsv_approximate(colour);
            }
            else if(colour == CRGB::Cyan){
                colour = CRGB::Green;
                colour_HSV = rgb2hsv_approximate(colour);
            }
            else if(colour == CRGB::Green){
                colour = CRGB::Yellow;
                colour_HSV = rgb2hsv_approximate(colour);
            }
            else if(colour == CRGB::Yellow){
                colour = CRGB::Orange;
                colour_HSV = rgb2hsv_approximate(colour);
            }
            else if(colour == CRGB::Orange){
                colour = CRGB::White;
                colour_HSV = rgb2hsv_approximate(colour);
            }
        }
        
        break;

        case CONFIG_IDLE:
            if(main_fsm.state != MAIN_IDLE){
                set_state(config_fsm, old_state_config);
                Serial.print(old_state_config);
                Serial.println("CONFIG_OLDSTATE");
            }
        break;
    }
    
}
    
void update_config_outputs(unsigned long now)
{
    switch (config_fsm.state)
    {
    case CONFIG_INIT:
        break;
    
    case CONFIG_1:

        
        if(config_fsm.interval < 10){
            leds[0] = CRGB::White;

            for(int i=1; i<NUM_LEDS-1; i++)
            {
                leds[i] = CRGB::Black;
            }
            config_fsm.interval++;

        }
        else if(config_fsm.interval < 20){
            for(int i=0; i<NUM_LEDS-1; i++)
            {
                leds[i] = CRGB::Black;
            }
            config_fsm.interval++;
        }
        else
            config_fsm.interval = 0;


        if((now - blink_old) < blink_time)
            leds[NUM_LEDS-1] = colour;
        else 
            leds[NUM_LEDS-1] = CRGB::Black;

        if(Sdown_pressed)
            blink_old = now;

        FastLED.show();

        break;
    
    case CONFIG_2:

        if(config_fsm.interval < 10){
            leds[0] = CRGB::Black;
            leds[1] = CRGB::White;

            for(int i=2; i<NUM_LEDS-1; i++)
            {
                leds[i] = CRGB::Black;
            }
            config_fsm.interval++;

        }
        else if(config_fsm.interval < 20){
            for(int i=0; i<NUM_LEDS-1; i++)
            {
                leds[i] = CRGB::Black;
            }
            config_fsm.interval++;
        }
        else
            config_fsm.interval = 0;


        if((now - blink_old) < blink_time){
            if(transition == 0){
                leds[NUM_LEDS-1] = colour;
                FastLED.show(); 
            }
            else if(transition == 1){
                leds[NUM_LEDS-1] = colour;

                if((now - blink_old) >= blink_time/2){
                
                    if(blinkMotion>=6){
                        blinkMotion = 0;
                    }

                    if(blinkMotion<3){
                        leds[NUM_LEDS-1] = CRGB::Black;
                        blinkMotion++;
                    }
                    else if(blinkMotion<6){
                        leds[NUM_LEDS-1] = colour;
                        blinkMotion++;
                    }
                }
            }
            else if(transition == 2){
                leds[NUM_LEDS-1] = colour;
                fadeMotion = map((now - blink_old), 0, blink_time, BRIGHTNESS, 0);
                leds[NUM_LEDS-1].setHSV(colour_HSV.hue, colour_HSV.saturation, fadeMotion);
            }
        }
        else 
            leds[NUM_LEDS-1] = CRGB::Black;

        if(Sdown_pressed){
            blink_old = now;
            blinkMotion = 0;
        }

        FastLED.show();

        break;
    
    case CONFIG_3:

        if(config_fsm.interval < 10){
            leds[0] = CRGB::Black;
            leds[1] = CRGB::Black;
            leds[2] = CRGB::White;

            for(int i=3; i<NUM_LEDS-1; i++)
            {
                leds[i] = CRGB::Black;
            }
            
            config_fsm.interval++;

        }
        else if(config_fsm.interval < 20){
            for(int i=0; i<NUM_LEDS-1; i++)
            {
                leds[i] = CRGB::Black;
            }
            config_fsm.interval++;
        }
        else
            config_fsm.interval = 0;
    

        leds[NUM_LEDS-1] = colour;


        FastLED.show();

        break;
    }

}


void setup() 
{
    pinMode(Sgo, INPUT_PULLUP);
    pinMode(Sup, INPUT_PULLUP);
    pinMode(Sdown, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    digitalWrite(LED_BUILTIN, HIGH);

    FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);

    // Start the serial port with 115200 baudrate
    Serial.begin(115200);

    set_state(main_fsm, MAIN_INIT);
    set_state(countdown_fsm, COUNTDOWN_INIT);
    set_state(aux_fsm, AUX_INIT);
    set_state(config_fsm, CONFIG_INIT);

    last_now = millis();

    for(int i=0; i<NUM_LEDS; i++)
    {
        leds[i] = CRGB::Black;
    }
    FastLED.show();
     
}



void loop() 
{
    unsigned long now = millis();
    

    if(now - last_now >= CYCLE_TIME){

        FastLED.setBrightness(BRIGHTNESS);
            
        read_inputs(now);
        
        update_main_fsm(now);
        update_countdown_fsm(now);
        update_aux_fsm(now);
        update_config_fsm(now);

        main_fsm.tis = now - main_fsm.tes;
        countdown_fsm.tis = now - countdown_fsm.tes;
        aux_fsm.tis = now - aux_fsm.tes;
        config_fsm.tis = now - config_fsm.tes;
        setup_fsm.tis = now - setup_fsm.tes;

        update_main_outputs(now);
        update_countdown_outputs(now);
        update_aux_outputs(now);
        update_config_outputs(now);
        
        last_now = now;
    }
}

