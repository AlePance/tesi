#include <SD.h>




 
#define F_CLK 8000000
#define prescaler_T1 64
#define SOGLIA_LINEARE  900// da definire
#define SOGLIA_INFERIORE 400// da definire 
#define PWM_HEATER1 5
#define PWM_HEATER2 6
//#define DEBUG

  uint32_t TIMER1_TOP = 65536;
  uint32_t ARRAY_TEMPI[] =  {125,1250,12500,125000};
  volatile uint8_t overflow_t1;
  long adcResult;


void adc_reset(){
  // Reset multiplexer and auto-trigger source
  ADMUX &= ~(1 << MUX4); 
  ADMUX &= ~(1 << MUX3); 
  ADMUX &= ~(1 << MUX2); 
  ADMUX &= ~(1 << MUX1); 
  ADMUX &= ~(1 << MUX0);
  ADCSRB &= ~(1 << MUX5); 
  ADCSRB &= ~(1 << ADTS3); 
  ADCSRB &= ~(1 << ADTS2);
  ADCSRB &= ~(1 << ADTS1);
  ADCSRB &= ~(1 << ADTS0);
  
  //disable auto triggering mode
  ADCSRA &= ~(1 << ADATE);

  
}

void adc_9(){

  
  // select channel 9 of ADC (pin 12)
  ADMUX |= (1 << MUX0);
  ADCSRB |= (1 << MUX5);

  // select auto trigger source (OC1B in this case)
  ADCSRB |= (1 << ADTS0);
  ADCSRB |= (1 << ADTS2);

  
}
void adc_autoTriggerEnable(){
  // enable auto trigerring mode
  ADCSRA |= (1 << ADATE);
}

void adc_autoTriggerDisable(){
  ADCSRA &= ~(1 << ADATE);
}


void adc_init(){
  
  // enable ADC conversion
  ADCSRA |= (1 << ADEN);

  // set a ADC prescaler (ADC clock must be into 50-200 KHz range) to 64 (ADC clock = 125 KHz)
  ADCSRA |= (1 << ADPS2);
  ADCSRA |= (1 << ADPS1);
}
  
// initialize timer 1
void timer1_init(void){
  
  /////
  //INTIALISAZION AUTO TRIGGER SOURCE
  /////
  
  // set OC1B toggle on compare match
  TCCR1A |= (1 << COM1B0);
  TCCR1A &= ~(1 << COM1B1);

  // set timer 1 in normal mode
  TCCR1A &= ~(1 << WGM10);
  TCCR1A &= ~(1 << WGM11);
  TCCR1B &= ~(1 << WGM12);
  TCCR1B &= ~(1 << WGM13);

  // set prescaler timer 1 to 64
  TCCR1B &= ~(1 << CS12);
  TCCR1B |=  (1 << CS11);
  TCCR1B |=  (1 << CS10);

  
  //enable overflow interrupt
  TIMSK1  |= (1 << TOIE1);
  

  ////////
  // IMPORTANT: SAMPLE AND HOLD TAKE 2 CICLE OF ADC CLOCK TO BE PERFORMED (16 us), KEEP IT IN MIND
  ////////
  #ifdef DEBUG
    DDRB |= (1 << DDB6);
  #endif
  
  
  }

void timer3_init(){
  
  // set prescaler to 8 
  TCCR3B &= ~(1 << CS32); 
  TCCR3B &= ~(1 << CS30);
  TCCR3B |= (1 << CS31);

  // set fast PWM mode (PWM frequency = 3.9 KHz)
  TCCR3B |= (1 << WGM32); 
  TCCR3B &= ~(1 << WGM33);
  TCCR3A &= ~(1 << WGM31);
  TCCR3A |= (1 << WGM30);

}

void timer4_init(){
  // set prescaler to 8 
  TCCR4B &= ~(1 << CS43); 
  TCCR4B &= ~(1 << CS41); 
  TCCR4B &= ~(1 << CS40);
  TCCR4B |= (1 << CS42);

  // set fast PWM mode (PWM frequency = 3.9 KHz)
  TCCR4D &= ~(1 << WGM40);
  TCCR4D &= ~(1 << WGM41);
}

/*  
// function for busy wait until OCF1B is set to 1 
void pulseGenerator(long t0 , long delta_t){
    
    long t = t0 + delta_t;

    // check timer 1 overflow (TOP = 65536)
    if( t > TIMER1_TOP){
         
      OCR1BH = (0xff00 & (t - TIMER1_TOP)) >> 8;
      OCR1BL = 0x00ff & (t - TIMER1_TOP);
      
    }
    else{
      
      OCR1BH = t >> 8;
      OCR1BL = 0x00ff & t;
      
    }
    // busy wait
    while(!(TIFR1 & (1 << OCF1B)));
    
    
*/
    
    /*
    // this sets the impulse lenght to 500us improving signal visibility on oscilloscope (TEST ONLY)
    delayMicroseconds(500);
    
    
    // force output compare on OC1B to toggle the output signal from high to low
    TCCR1C |= (1 << FOC1B);
    // reset flag of compare match on timer 1 channel B
    TIFR1  |= (1 << OCF1B);
    
    
  }
  */
  long adcRead(void){
    
    //busy wait
    while (!(ADCSRA & (1 << ADIF)));
    long result = ADCL | ( ADCH << 8);

    ADCSRA = (1 << ADIF); 
    
    return result;
  }



  
void setup() {
  
  adc_init();
  timer1_init();
  timer3_init();
  timer4_init();
  adc_9();
  #ifdef DEBUG
  pinMode(PWM_HEATER1,OUTPUT);
  #endif
  /*
  pinMode(PWM_HEATER2,OUTPUT);
  */
  Serial.begin(9600);
}
  
void loop() {
  
  
  uint16_t t0;
  
    adc_autoTriggerDisable();
    // enable interrupt
    interrupts();
   
    //  reset overflow count
      
    // disable autotrigger to prevent reading mistakes 
   
    overflow_t1 = 0;
    t0 = TCNT1L | (TCNT1H << 8);
      for(int i = 0; i < 4; i++){
      uint32_t t = t0 + ARRAY_TEMPI[i];
      uint8_t n_overflow = floor(t / TIMER1_TOP);
      t = t - (n_overflow*TIMER1_TOP);
      OCR1BH = t >> 8;
      OCR1BL = 0x00ff & t; 
      
      /*if(i==3) {
        // istruzioni da eseguire nell'intervallo di tempo tra 100 ms e 1000 ms 
        // .....
      }*/
      
      while(n_overflow > overflow_t1);
      adc_autoTriggerEnable();
      noInterrupts();
      
      //clear OCF1B flag
      TIFR1  = (1 << OCF1B);
        
      while(!(TIFR1 & (1 << OCF1B)));
      
      #ifdef DEBUG
      Serial.println(t0);
      Serial.println(overflow_t1);
      digitalWrite(PWM_HEATER1,HIGH);
      delayMicroseconds(900);
      digitalWrite(PWM_HEATER1,LOW);
      #endif
      
      TIFR1  = (1 << OCF1B);
      
      /// ADC READ NOT WORKING PROPERLY
      /*
      adcResult = adcRead();
      Serial.println(adcResult);
      */
      interrupts();
      
    
      }   
} 

ISR(TIMER1_OVF_vect){
  
  overflow_t1++;

}

ISR(TIMER1_COMPB_vect){

    
}

ISR(TIMER1_COMPC_vect){

    
}
ISR(TIMER0_COMPB_vect){

    
}
ISR(TIMER1_COMPA_vect){

    
}
ISR(TIMER0_COMPA_vect){

    
}
ISR(ADC_vect){
  
}
