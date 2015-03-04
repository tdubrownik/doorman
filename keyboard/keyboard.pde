
//klawiatura
#define KEYPIN_1 10
#define KEYPIN_2 11
#define KEYPIN_3 12
#define KEYPIN_4 13
#define KEYPIN_A 9
#define KEYPIN_B 8
#define KEYPIN_C 2

//ledy
#define LED_Y 7
#define LED_G 6
#define LED_R 5
#define LED_KEYS 4

//buzzer
#define BUZZ 3

//serial
//stałe protokołu komunikacji
#define BSIZE_IN 10
#define FRAME_LEN 2
//pola w ramce
#define SOF_OFF 0
#define CMD_OFF 1
//stan odbiornika
#define NO_FRAME 0
#define RECEIVING 1
#define FRAME_READY 2
//komendy
#define IDLE 0
#define PIN 'P'
#define OK 'R'
#define WRONG 'W'

//różne stałe
#define DEBOUNCE_DELAY 50
#define PIN_LENGTH 4


//--------------funkcje----------------------
int key_get(void)
{
  int i,j,k;
  static int timestamp;
  static int key_code_tmp,key_code_old,key_code,new_key;

  //skanuj klawiaturę
  key_code_tmp=0;
  for(i=0;i<4;i++)//domyslny kod klawisza - 0 (brak)
  {
    //wysteruj jedną linię klawiatury
    digitalWrite(10+i,0);
    delay(1);
    //sprawdź który klawisz jest wciśnięty
    if( !(k=digitalRead(KEYPIN_A)) )
      key_code_tmp=(3-i)*3+3;
    if( !(k=digitalRead(KEYPIN_B)) )
      key_code_tmp=(3-i)*3+2;
    if( !(k=digitalRead(KEYPIN_C)) )
      key_code_tmp=(3-i)*3+1;
    
    //wyczyść linię
    digitalWrite(10+i,1);
  }
  
  //2-debounce
  // stan klawisza się zmienił, resetuj timer
  if(key_code_tmp != key_code_old)
  {
    timestamp=millis();  
  }
  
 //zapisz ostatni kod klawisza   
  key_code_old=key_code_tmp;  
  
  //klawisz "siedzi", i czy kod klawisza różni się od ostatniego
  if( ((millis() - timestamp) >= DEBOUNCE_DELAY) && (key_code!=key_code_tmp) )
  {
    //zapisz kod klawisza
    key_code=key_code_tmp;
    //zwróć kod klawisza
    return key_code;
  }
  else
    return 0;
}

//*setup
void setup()
{
  
  //klawiatka
  //4 wyjścia
  pinMode(KEYPIN_1,OUTPUT);
  pinMode(KEYPIN_2,OUTPUT);
  pinMode(KEYPIN_3,OUTPUT);
  pinMode(KEYPIN_4,OUTPUT);
  //domyslny stan: wysoki
  digitalWrite(KEYPIN_1,1);
  digitalWrite(KEYPIN_2,1);
  digitalWrite(KEYPIN_3,1);
  digitalWrite(KEYPIN_4,1);
  //3 wejścia
  pinMode(KEYPIN_A,INPUT);
  pinMode(KEYPIN_B,INPUT);
  pinMode(KEYPIN_C,INPUT);

  //pullupy włączone
  digitalWrite(KEYPIN_A,1);
  digitalWrite(KEYPIN_B,1);
  digitalWrite(KEYPIN_C,1);

  //serial
  Serial.begin(19200);
  
  //ledy
  pinMode(LED_R,OUTPUT);
  pinMode(LED_G,OUTPUT);
  pinMode(LED_Y,OUTPUT);
  pinMode(LED_KEYS,OUTPUT);
  //domyslny stan: wysoki
  digitalWrite(LED_R,1);
  digitalWrite(LED_G,1);
  digitalWrite(LED_Y,1);
  digitalWrite(LED_KEYS,1); 
 
 //buzzer
  pinMode(BUZZ,OUTPUT);
  digitalWrite(BUZZ,0);  
  
 //mignij zasilaniem
   digitalWrite(LED_G,0);
   delay(1000);
   digitalWrite(LED_G,1);
}

void loop()
{
  static int dev_state;
  static char pin[4],num_digits;
  static char buf_in[BSIZE_IN],byte_cntr,serial_state;  
  int i,j,k;
  
  //skanuj serial
  if (Serial.available() > 0)
  { 
    //zapisz bajt
    buf_in[byte_cntr]=Serial.read();
    byte_cntr++;
    
   //parsuj komendę
    switch(serial_state)
    {
      case NO_FRAME:
        //złap początek ramki
        if(buf_in[SOF_OFF] == '#' )
          serial_state=RECEIVING;
        else
        //przejdź do stanu idle
          byte_cntr=0;
          num_digits=0;
          dev_state=IDLE;
        break;
        
      case RECEIVING:
        //czy przyszła cała ramka?
        if(byte_cntr==FRAME_LEN)
          serial_state=FRAME_READY;
        //bez break, przechodzimy do parsowania 
        
      case FRAME_READY:
        //parsuj ramkę
        switch(buf_in[CMD_OFF])
        {
          case PIN:
            //podaj pin
            dev_state=PIN;
            num_digits=0;
            break;
          
          case OK:
            //dobry pin
            dev_state=OK;
            num_digits=0;
            break;
          
          case WRONG:
            //zły pin
            dev_state=WRONG;
            num_digits=0;
            break;
            
          default:
            //błędna ramka - przejdź do stanu idle
            dev_state=IDLE;
            num_digits=0;
            break;    
        }
        
        //wyczyść bufor odbiorczy i stan odbiornika
        byte_cntr=0;
        serial_state=NO_FRAME;
        break;
        
      default:
      //nieprawidłowy stan - wyczyść wszystko i wróć do stanu domyślnego
      //ta pozycja switcha jest na wszelki wypadek jakby program zgłupiał. You never know.
        dev_state=IDLE;
        serial_state=NO_FRAME;
        byte_cntr=0;
        num_digits=0;
        break;
    }
  }
 
  //wykonaj komendę
  switch(dev_state)
  {
    case PIN:
    //zapal żóltą diodę i podświetlenie;
    digitalWrite(LED_Y,0);
    digitalWrite(LED_KEYS,0);
    //skanuj klawiaturę
    k=key_get();
    
    //nacisnieto klawisz 
    if(k && (k!=10) && (k!=12))
    {
      //piknij buzzerem
      digitalWrite(BUZZ,1);
      delay(10);
      digitalWrite(BUZZ,0);
      
      //kod klawisza 11 - klawisz "0" 
      if(k==11) 
        k=0;
      
      //zapisz klawisz    
      pin[num_digits]=k;
      num_digits++;
      
      // jezeli wpisano caly pin - wyslij pin
      if( num_digits>=PIN_LENGTH )
      {
        Serial.write('#');
        for(i=0;i<PIN_LENGTH;i++)
          Serial.print(pin[i],DEC); 
       //wyzeruj licznik znaków 
        num_digits=0;
        //zadanie wykonane
        dev_state=IDLE;
      }
    }
    break;
     
    case OK:
    
      //diodka zielona
      digitalWrite(LED_G,0);
      //dzwiek pin ok  
      for(i=0;i<4;i++)
      {
        digitalWrite(BUZZ,1);
        delay(100);
        digitalWrite(BUZZ,0);
        delay(100);
      }
      //poczekaj
      delay(1000);
      //zadanie wykonane
      dev_state=IDLE;
      break;
    
    case WRONG:
      //diodka czerwona
      digitalWrite(LED_R,0);
      //dzwiek pin wrong
      for(i=0;i<2;i++)
      {
        for(j=0;j<100;j++)
        {
          digitalWrite(BUZZ,1);
          delay(2);
          digitalWrite(BUZZ,0);
          delay(2);
        }
        delay(500);
      }
      //poczekaj
      delay(1000);
      //zadanie wykonane
      dev_state=IDLE;
      break;
    
    case IDLE:
      //gas diodki
      digitalWrite(LED_R,1);
      digitalWrite(LED_G,1);
      digitalWrite(LED_Y,1);
      digitalWrite(LED_KEYS,1); 
      //nic nie rob
      break;
      
    default:
      //nieprawidłowy stan - przejdź do idle, wyczysc bufor seriala
      //ta pozycja switcha jest na wszelki wypadek jakby program zgłupiał. You never know.
      dev_state=IDLE;
      serial_state=NO_FRAME;
      byte_cntr=0;
      break;
  }
}



  

