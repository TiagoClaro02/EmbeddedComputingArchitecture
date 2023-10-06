#include <Arduino.h>
#include "FastLED.h"

#define NUM_LEDS 15
#define LED_PIN 6
#define LED_TIME 500
#define LED_BLINK_TIME 500
#define LED_BLINK_PAUSE 850
#define HOLD_TIME 3000
#define BLINK_TIME_1 1000
#define BLINK_TIME_2 2000
#define BLINK_TIME_5 5000
#define BLINK_TIME_10 10000


#define Sgo 2
#define Sup 3
#define Sdown 4

CRGB leds[NUM_LEDS];

// States
typedef enum{
    MAIN_IDLE,
    MAIN_START,
    MAIN_CONFIG,
    MAIN_PAUSE,
    COUNTDOWN_IDLE,
    COUNTDOWN_START,
    COUNTDOWN_PAUSE,
    COUNTDOWN_BLINK,
    COUNTDOWN_CONFIG,
    AUX_IDLE,
    AUX_HIGH,
    AUX_LOW,
    CONFIG_IDLE,
    CONFIG_HIGH_1,
    CONFIG_LOW_1,
    CONFIG_HIGH_2,
    CONFIG_LOW_2,
    CONFIG_HIGH_3,
    CONFIG_LOW_3,
    SETUP_IDLE,
    SETUP_HIGH,
} state_t;

// Finite State Machine
typedef struct {
    state_t state, new_state;

    // tes - time entering state
    // tis - time in state
    unsigned long tes, tis, aux, interval;
} fsm_t;

// Inputs
bool Sgo_state, Sup_state, Sdown_state;

fsm_t main_fsm, countdown_fsm, aux_fsm, config_fsm, setup_fsm;

bool Sgo_state_old = HIGH;
bool Sup_state_old = LOW;
bool Sdown_state_old = HIGH;

bool Sgo_pressed = false;
bool Sup_pressed = false;
bool Sdown_pressed = false;
bool Sup_hold = false;
bool Sup_hold_old = false;

bool flag;

int low_leds = 0;
int countdown = 0;
int aux = 0;
int low_leds_old = 0;
unsigned long hold = 0, start_hold = 0, blink_old = 0;

int blink_time = BLINK_TIME_10;

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
void read_inputs()
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
            start_hold = millis();
        }
        else{
            hold = millis() - start_hold;
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


    Sgo_state_old = Sgo_state;
    Sup_state_old = Sup_state;
    Sdown_state_old = Sdown_state;
}



//######## MAIN FSM ########



// Update main state machine
void update_main_fsm()
{
    switch (main_fsm.state)
    {
    case MAIN_IDLE:
        if (Sgo_pressed)
        {
            set_state(main_fsm, MAIN_START);
            Serial.println("MAIN_START");
            flag = false;
        }
        if(Sup_hold){
            set_state(main_fsm, MAIN_CONFIG);
            Serial.println("MAIN_CONFIG");
            Sup_hold = false;
        }
        break;
    
    case MAIN_START:

        if (Sdown_pressed)
        {
            set_state(main_fsm, MAIN_PAUSE);
            Serial.println("MAIN_PAUSE");
        }
        break;

    case MAIN_PAUSE:

        if (Sdown_pressed)
        {
            set_state(main_fsm, MAIN_START);
            Serial.println("MAIN_START");
        }
        break;
    

    case MAIN_CONFIG:
        if(Sup_hold)
        {
            set_state(main_fsm, MAIN_IDLE);
            Serial.println("MAIN_IDLE");
            Sup_hold = false;
        }
        break;
    }
}

// Update outputs
void update_main_outputs()
{
    switch (main_fsm.state)
    {
    case MAIN_IDLE:

        for(int i=0; i<NUM_LEDS; i++)
        {
            leds[i] = CRGB::Black;
        }
        FastLED.show();

        break;
    
    case MAIN_START:

        break;
    case MAIN_PAUSE:

        break;
    }
}



//######## COUNTDOWN FSM ########


//  Update countdown state machine
void update_countdown_fsm()
{
    switch (countdown_fsm.state)
    {
    case COUNTDOWN_IDLE:
        if (main_fsm.state == MAIN_START)
        {
            set_state(countdown_fsm, COUNTDOWN_START);
            Serial.println("COUNTDOWN_START");
            low_leds = 0;
            low_leds_old = 0;
            aux=0;
        }
        if(main_fsm.state == MAIN_CONFIG){
            set_state(countdown_fsm, COUNTDOWN_CONFIG);
            Serial.println("COUNTDOWN_CONFIG");
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
        break;

    case COUNTDOWN_PAUSE:
    
        if(main_fsm.state == MAIN_START)
        {
            set_state(countdown_fsm, COUNTDOWN_START);
            low_leds = low_leds_old;
            flag = true;
            aux=0;
            Serial.println("COUNTDOWN_START");
        }
        break;

    case COUNTDOWN_BLINK:
        if (Sgo_pressed)
        {
            set_state(main_fsm, MAIN_IDLE);
            set_state(countdown_fsm, COUNTDOWN_IDLE);
            set_state(aux_fsm, AUX_IDLE);
            Serial.println("MAIN_IDLE");
            Serial.println("COUNTDOWN_IDLE");
            Serial.println("AUX_IDLE");
        }
        break;

    case COUNTDOWN_CONFIG:
        if(main_fsm.state == MAIN_IDLE)
        {
            set_state(countdown_fsm, COUNTDOWN_IDLE);
            Serial.println("COUNTDOWN_IDLE");
        }
        break;
    }
}



// Update countdown outputs
void update_countdown_outputs()
{
    switch (countdown_fsm.state)
    {
    case COUNTDOWN_IDLE:
        for(int i=0; i< NUM_LEDS; i++)
        {
            leds[i] = CRGB::Black;
        }
        FastLED.show();
        break;
    
    case COUNTDOWN_START:

        Serial.println(low_leds);

        if(flag){
            countdown_fsm.aux = countdown_fsm.tis + low_leds_old*LED_TIME;
        }
        else{
            countdown_fsm.aux = countdown_fsm.tis;
        }

        countdown_fsm.interval = countdown_fsm.aux - LED_TIME*low_leds - aux*LED_TIME;

        for(int i=0; i < low_leds; i++)
        {
            leds[i] = CRGB::Black;
        }
        for(int i=low_leds; i < NUM_LEDS; i++)
        {
            leds[i] = CRGB::White;
        }
        FastLED.show();

        if(Sup_pressed && low_leds < NUM_LEDS && low_leds > 0){
            low_leds--;
            aux++;
            break;
        }

        if(countdown_fsm.interval >= LED_TIME){
            low_leds++;
        }

        break;

    case COUNTDOWN_PAUSE:
    
        break;

    case COUNTDOWN_BLINK:

        countdown_fsm.interval = countdown_fsm.tis - LED_BLINK_TIME*aux;

        if(countdown_fsm.interval <= LED_BLINK_TIME/2){
            for(int i=0; i< NUM_LEDS; i++)
            {
                leds[i] = CRGB::Red;
            }
        }
        else if(countdown_fsm.interval <= LED_BLINK_TIME){
            for(int i=0; i< NUM_LEDS; i++)
            {
                leds[i] = CRGB::Black;
            }
        }
        else{
            aux++;
        }
        FastLED.show();

        break;
    }
}



//######## AUX FSM ########



void update_aux_fsm()
{
    switch (aux_fsm.state)
    {
    case AUX_IDLE:
        if(countdown_fsm.state == COUNTDOWN_PAUSE)
        {
            set_state(aux_fsm, AUX_HIGH);
            Serial.println("AUX_HIGH");
        }
        break;
    
    case AUX_HIGH:
        if(countdown_fsm.state == COUNTDOWN_START)
        {
            set_state(aux_fsm, AUX_IDLE);
            Serial.println("AUX_IDLE");
        }
        else if(aux_fsm.tis >= LED_BLINK_PAUSE/2)
        {
            set_state(aux_fsm, AUX_LOW);
            Serial.println("AUX_LOW");
        }
        break;

    case AUX_LOW:
        if(countdown_fsm.state == COUNTDOWN_START)
        {
            set_state(aux_fsm, AUX_IDLE);
            Serial.println("AUX_IDLE");
        }
        else if(aux_fsm.tis >= LED_BLINK_PAUSE/2)
        {
            set_state(aux_fsm, AUX_HIGH);
            Serial.println("AUX_HIGH");
        }
        break;
    }
}

void update_aux_outputs()
{
    switch (aux_fsm.state)
    {
    case AUX_IDLE:
        break;
    
    case AUX_HIGH:

        for(int i=0; i<low_leds; i++)
        {
            leds[i] = CRGB::Black;
        }
        for(int i=low_leds; i<NUM_LEDS; i++)
        {
            leds[i] = CRGB::White;
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

void update_config_fsm()
{
    switch (config_fsm.state)
    {
    case CONFIG_IDLE:
        if(main_fsm.state == MAIN_CONFIG)
        {
            set_state(config_fsm, CONFIG_HIGH_1);
            Serial.println("CONFIG_SET");
        }
        break;
    
    case CONFIG_HIGH_1:
        if(main_fsm.state == MAIN_IDLE)
        {
            set_state(config_fsm, CONFIG_IDLE);
            Serial.println("CONFIG_IDLE");
        }
        if(config_fsm.tis >= LED_BLINK_PAUSE/2)
        {
            set_state(config_fsm, CONFIG_LOW_1);
            Serial.println("CONFIG_LOW_1");
        }
        if(Sup_pressed)
        {
            set_state(config_fsm, CONFIG_HIGH_2);
            Serial.println("CONFIG_HIGH_2");
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

    case CONFIG_LOW_1:
        if(main_fsm.state == MAIN_IDLE)
        {
            set_state(config_fsm, CONFIG_IDLE);
            Serial.println("CONFIG_IDLE");
        }
        if(config_fsm.tis >= LED_BLINK_PAUSE/2)
        {
            set_state(config_fsm, CONFIG_HIGH_1);
            Serial.println("CONFIG_HIGH_1");
        }
        if(Sup_pressed)
        {
            set_state(config_fsm, CONFIG_LOW_2);
            Serial.println("CONFIG_LOW_2");
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

    case CONFIG_HIGH_2:
        if(main_fsm.state == MAIN_IDLE)
        {
            set_state(config_fsm, CONFIG_IDLE);
            Serial.println("CONFIG_IDLE");
        }
        if(config_fsm.tis >= LED_BLINK_PAUSE/2)
        {
            set_state(config_fsm, CONFIG_LOW_2);
            Serial.println("CONFIG_HIGH_2");
        }
        if(Sup_pressed)
        {
            set_state(config_fsm, CONFIG_HIGH_3);
            Serial.println("CONFIG_HIGH_3");
        }
        break;

    case CONFIG_LOW_2:
        if(main_fsm.state == MAIN_IDLE)
        {
            set_state(config_fsm, CONFIG_IDLE);
            Serial.println("CONFIG_IDLE");
        }
        if(config_fsm.tis >= LED_BLINK_PAUSE/2)
        {
            set_state(config_fsm, CONFIG_HIGH_2);
            Serial.println("CONFIG_HIGH_2");
        }
        if(Sup_pressed)
        {
            set_state(config_fsm, CONFIG_HIGH_3);
            Serial.println("CONFIG_HIGH_3");
        }
        break;

    case CONFIG_HIGH_3:
        if(main_fsm.state == MAIN_IDLE)
        {
            set_state(config_fsm, CONFIG_IDLE);
            Serial.println("CONFIG_IDLE");
        }
        if(config_fsm.tis >= LED_BLINK_PAUSE/2)
        {
            set_state(config_fsm, CONFIG_LOW_3);
            Serial.println("CONFIG_LOW_3");
        }
        if(Sup_pressed)
        {
            set_state(config_fsm, CONFIG_HIGH_1);
            Serial.println("CONFIG_HIGH_1");
        }
        break;

    case CONFIG_LOW_3:
        if(main_fsm.state == MAIN_IDLE)
        {
            set_state(config_fsm, CONFIG_IDLE);
            Serial.println("CONFIG_IDLE");
        }
        if(config_fsm.tis >= LED_BLINK_PAUSE/2)
        {
            set_state(config_fsm, CONFIG_HIGH_3);
            Serial.println("CONFIG_HIGH_3");
        }
        if(Sup_pressed)
        {
            set_state(config_fsm, CONFIG_HIGH_1);
            Serial.println("CONFIG_HIGH_1");
        }
        break;

    }
    
}
    
void update_config_outputs()
{
    switch (config_fsm.state)
    {
    case CONFIG_IDLE:
        break;
    
    case CONFIG_HIGH_1:

        leds[0] = CRGB::White;

        for(int i=1; i<NUM_LEDS; i++)
        {
            leds[i] = CRGB::Black;
        }
        FastLED.show();

        break;

    case CONFIG_LOW_1:

        for(int i=0; i<NUM_LEDS; i++)
        {
            leds[i] = CRGB::Black;
        }
        FastLED.show();

        break;
    
    case CONFIG_HIGH_2:

        leds[0] = CRGB::Black;
        leds[1] = CRGB::White;

        for(int i=2; i<NUM_LEDS; i++)
        {
            leds[i] = CRGB::Black;
        }
        FastLED.show();

        break;
        
    case CONFIG_LOW_2:

        for(int i=0; i<NUM_LEDS; i++)
        {
            leds[i] = CRGB::Black;
        }
        FastLED.show();

        break;
    
    case CONFIG_HIGH_3:
    
        leds[0] = CRGB::Black;
        leds[1] = CRGB::Black;
        leds[2] = CRGB::White;

        for(int i=3; i<NUM_LEDS; i++)
        {
            leds[i] = CRGB::Black;
        }
        FastLED.show();

        break;
    
    case CONFIG_LOW_3:

        for(int i=0; i<NUM_LEDS; i++)
        {
            leds[i] = CRGB::Black;
        }
        FastLED.show();

        break;

    }

}


//######## SETUP FSM ########

void update_setup_fsm()
{
    switch (setup_fsm.state)
    {
    case SETUP_IDLE:
        if((config_fsm.state == CONFIG_HIGH_1 || config_fsm.state == CONFIG_LOW_1) && Sdown_pressed)
        {
            set_state(setup_fsm, SETUP_HIGH);
            blink_old = millis();
            Serial.println("SETUP_HIGH");
        }
        break;
    
    case SETUP_HIGH:

        
        if((millis() - blink_old) >= blink_time){
            set_state(setup_fsm, SETUP_IDLE);
            Serial.println("SETUP_IDLE");
        }

        break;
    }
    
}

void update_setup_outputs()
{
    switch (setup_fsm.state)
    {
    case SETUP_IDLE:

        leds[NUM_LEDS-1] = CRGB::Black;
        FastLED.show();

        break;
    
    case SETUP_HIGH:

        leds[NUM_LEDS-1] = CRGB::White;
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
    FastLED.setBrightness(10);

    // Start the serial port with 115200 baudrate
    Serial.begin(115200);

    set_state(main_fsm, MAIN_IDLE);
    set_state(countdown_fsm, COUNTDOWN_IDLE);
    set_state(aux_fsm, AUX_IDLE);
    set_state(config_fsm, CONFIG_IDLE);
    set_state(setup_fsm, SETUP_IDLE);
     
}



void loop() 
{
    read_inputs();
    update_main_fsm();
    update_countdown_fsm();
    update_aux_fsm();
    update_config_fsm();
    update_setup_fsm();

    unsigned long now = millis();
    main_fsm.tis = now - main_fsm.tes;
    countdown_fsm.tis = now - countdown_fsm.tes;
    aux_fsm.tis = now - aux_fsm.tes;
    config_fsm.tis = now - config_fsm.tes;
    setup_fsm.tis = now - setup_fsm.tes;

    update_main_outputs();
    update_countdown_outputs();
    update_aux_outputs();
    update_config_outputs();
    update_setup_outputs();

}
