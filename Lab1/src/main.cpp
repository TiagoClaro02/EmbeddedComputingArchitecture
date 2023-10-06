#include <Arduino.h>
#include "FastLED.h"

#define NUM_LEDS 5
#define LED_PIN 6
#define LED_TIME 1000

#define Sgo 2
#define Sup 3
#define Sdown 4

CRGB leds[NUM_LEDS];


// States
typedef enum{
    MAIN_IDLE,
    MAIN_START,
    MAIN_PAUSE,
    COUNTDOWN_IDLE,
    COUNTDOWN_START,
    COUNTDOWN_PAUSE,
    COUNTDOWN_BLINK,
    AUX_IDLE,
    AUX_HIGH,
    AUX_LOW,
} state_t;

// Finite State Machine
typedef struct {
    state_t state, new_state;

    // tes - time entering state
    // tis - time in state
    unsigned long tes, tis;
} fsm_t;

// Inputs
bool Sgo_state, Sup_state, Sdown_state;

unsigned long interval, last_cycle;
unsigned long loop_micros;
uint16_t blink_period;

fsm_t main_fsm, countdown_fsm, aux_fsm;

bool Sgo_state_old = HIGH;
bool Sup_state_old = HIGH;
bool Sdown_state_old = HIGH;

bool Sgo_pressed = false;
bool Sup_pressed = false;
bool Sdown_pressed = false;

int high_leds = NUM_LEDS;
int low_leds = 0;
int countdown = 0;
int aux = 0;
int low_leds_old = 0;

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

    if (Sup_state == LOW && Sup_state_old == HIGH)
    {
        Serial.println("Sup");
        Sup_pressed = true;
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


    Sgo_state_old = Sgo_state;
    Sup_state_old = Sup_state;
    Sdown_state_old = Sdown_state;
}



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
    }
}

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
        }
        break;
    
    case COUNTDOWN_START:

        if(main_fsm.state == MAIN_PAUSE)
        {
            set_state(countdown_fsm, COUNTDOWN_PAUSE);
            countdown = countdown_fsm.tis;
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
            countdown_fsm.tis = countdown;
            low_leds = low_leds_old;
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
    }
}

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
        else if(aux_fsm.tis >= 750)
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
        else if(aux_fsm.tis >= 750)
        {
            set_state(aux_fsm, AUX_HIGH);
            Serial.println("AUX_HIGH");
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

        break;
    
    case MAIN_START:

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

        interval = countdown_fsm.tis - LED_TIME*low_leds;

        for(int i=0; i < low_leds; i++)
        {
            leds[i] = CRGB::Black;
        }
        for(int i=low_leds; i < high_leds; i++)
        {
            leds[i] = CRGB::Blue;
        }
        FastLED.show();

        if(interval >= LED_TIME){
            low_leds++;
        }

        break;

    case COUNTDOWN_PAUSE:
    
        break;

    case COUNTDOWN_BLINK:

        interval = countdown_fsm.tis - 1000*aux;

        if(interval <= 500){
            for(int i=0; i< NUM_LEDS; i++)
            {
                leds[i] = CRGB::Red;
            }
        }
        else if(interval <= 1000){
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
            leds[i] = CRGB::Blue;
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



void setup() 
{
    pinMode(Sgo, INPUT_PULLUP);
    pinMode(Sup, INPUT_PULLUP);
    pinMode(Sdown, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    digitalWrite(LED_BUILTIN, HIGH);

    FastLED.addLeds<NEOPIXEL, 6>(leds, NUM_LEDS);

    // Start the serial port with 115200 baudrate
    Serial.begin(115200);

    set_state(main_fsm, MAIN_IDLE);
    set_state(countdown_fsm, COUNTDOWN_IDLE);
    set_state(aux_fsm, AUX_IDLE);
     
}



void loop() 
{
    read_inputs();
    update_main_fsm();
    update_countdown_fsm();
    update_aux_fsm();

    unsigned long now = millis();
    main_fsm.tis = now - main_fsm.tes;
    countdown_fsm.tis = now - countdown_fsm.tes;
    aux_fsm.tis = now - aux_fsm.tes;

    update_main_outputs();
    update_countdown_outputs();
    update_aux_outputs();

}





