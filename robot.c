// Proejkt na predmet IMR
// Autor: Adam Fabo
// Subor obsahuje kod pre ovladanie pohybu robota po ciernej ciare 

#include <Servo.h>
// 3 lavy

// data pre all-star melodiu
const short PROGMEM tones[]   =  {370, 554, 466, 466, 415, 370, 370, 494, 466, 466, 415, 415, 370, 370, 554, 466, 466, 415, 370, 370, 311, 277, 370, 370, 554, 466, 466, 415, 415, 369, 369, 494, 466, 466, 415, 415, 369, 369, 554, 466, 466, 415, 369, 369, 415, 311, 369, 311, 369, 369, 369, 369, 311, 277, 369, 369, 369, 369, 369, 369, 369, 369, 369, 369, 369, 369, 369, 466, 369, 466, 466, 554, 494, 466, 369, 415, 415, 415, 415, 466, 369, 369, 311, 277, 369, 369, 369, 466, 554, 466, 622, 466, 554, 466, 622, 466, 554, 494, 466, 415, 369, 369, 415, 369, 369, 369, 369, 369, 369, 369, 369, 369, 369, 369, 466, 369, 369, 369, 311, 311};
const unsigned short dels[] PROGMEM  = {16932, 16930, 8738, 8740, 8740, 8804, 8738, 16930, 8738, 9250, 8770, 4964, 4386, 8721, 4625, 4385, 4625, 8467, 8466, 8498, 4386, 4642, 4626, 4626, 8738, 16914, 8465, 16913, 4369, 8466};

Servo myservo;

// sonar
#define ECHO_PIN 3
#define TRIG_PIN 2

//buzzer
#define PIEZO_PIN 4
#define TONES_LEN 120

//servo
#define SERVO_PIN 5

//motor
#define MOTOR_L1 6
#define MOTOR_L2 7
#define MOTOR_R1 8
#define MOTOR_R2 9

//krokovy motor
#define STEPPER_1 10
#define STEPPER_2 11
#define STEPPER_3 12
#define STEPPER_4 13

//pocet krokov potrebnych na zodvihnutie korby
#define STEPPER_STEPS 150

//led driver
#define LED_PIN 19

// flagy pre nastavovanie lediek
#define NONE  B00000000
#define RBLUE B00000001
#define LBLUE B00000010
#define LRED  B00000100
#define RRED  B00001000



#define DIST_target 15.0
#define DIST_target_wall 22.0
#define DIST_koef_add 9
#define DIST_koef_sub 3


#define target  700       // Koeficienty P regulatora pre snimanie stredu ciary
#define P_koef_add  0.7
#define P_koef_sub  1.5


#define target_low 200   // Koeficienty P regulatora pre snimanie hrany ciary
#define target_high 700
#define P_koef_add_hrana  0.2
#define P_koef_sub_hrana  0.5

#define hysteresis_up 650
#define hysteresis_down 450

#define koef 2

unsigned int min_speed     = 300 * koef;    // toto teoreticky cez define
unsigned int max_speed_add = 40  * koef;
unsigned int max_speed_sub = 20  * koef;

unsigned int periode = 1000 * koef;         // toto teoreticky cez define

unsigned int avgR = 500;
unsigned int avgL = 500;

unsigned long int smerovka_vpravo = 0;
unsigned long int smerovka_vlavo  = 0;

enum Mode { MODE_STRAIGHT, MODE_LEFT, MODE_RIGHT, MODE_GIVEN_RIGHT, MODE_GIVEN_LEFT,  MODE_KORBA};
Mode mode = MODE_STRAIGHT;
Mode mode_old = -1;// na zaciatok nejaku nevalidnu hodnotu

Mode modes_arr[] = {MODE_GIVEN_LEFT,MODE_KORBA};
int modes_counter = 0;

unsigned int filter_up = 18;              // toto teoreticky cez define
unsigned int filter_down = 40;            // toto teoreticky cez define


unsigned int counter = 6;                 // toto teoreticky cez define

unsigned int cnt = 0;

long duration;
float distance;

unsigned int delR = min_speed;
unsigned int delL = min_speed;

unsigned int num_of_cycles = 250;

unsigned int range_slow = 0;
bool once  = true;


unsigned int not_ciara_koef = 50 * koef;


int x = 20000;
bool can_go = true;
int pos = 0;
#define treshold  800;



void setup() {
  pinMode(TRIG_PIN, OUTPUT);     // sonar
  pinMode(ECHO_PIN, INPUT);

  pinMode(PIEZO_PIN, OUTPUT);

  pinMode(MOTOR_L1, OUTPUT);    // motor
  pinMode(MOTOR_L2, OUTPUT);
  pinMode(MOTOR_R1, OUTPUT);
  pinMode(MOTOR_R2, OUTPUT);

  pinMode(STEPPER_1, OUTPUT);   // krokovy
  pinMode(STEPPER_2, OUTPUT);
  pinMode(STEPPER_3, OUTPUT);
  pinMode(STEPPER_4, OUTPUT);

  digitalWrite(STEPPER_1, LOW);
  digitalWrite(STEPPER_2, LOW);
  digitalWrite(STEPPER_3, LOW);
  digitalWrite(STEPPER_4, LOW);

  myservo.attach(SERVO_PIN);
  Serial.begin(230400);
  Serial.println("Lets start this bitch");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}


// hlavna smycka programu
void loop() {

  // stara sa o detekciu prekazok, ak na nejaku narazi tak prebera vedenie robota a postara sa o obidenie prekazky
  // ak ziadnu prekazku nedetekoval tak konci
  // ak detekoval potencialnu prekazku tak nastavuje hodnotu range_slow ktora spomali auto az dokedy bud nedetekuje prekazku uplne
  // alebo prekazka zmizne
  // taktiez nastavuje aj num_of_cycles - cim blizsie ku prekazke, tym menej cyklov
  obstacle_detection();


  main_func();

}

// hlavna funkcia robota
void main_func() {

  for (unsigned i = 0; i < num_of_cycles; i++) {
    // nastavenie serva dopredu
    myservo.write(80);

    // spomalenie robota ak je blizko prekazky
    delR -= min(range_slow, delR);
    delL -= min(range_slow, delL);

    motor_forward_PWM();



    // hodoty hlavneho snimaca ciary
    avgR = analogRead(1);
    avgL = analogRead(0);


    // branie kazdej n-tej hodnoty na spomalenie rastu hodnoti s kombinacou filtra
    // ak napr auto krizuje inu ciaru tak aby to nebralo ako falosnu detekciu smerovacej ciary
    if (i % counter == 0) {
      smerovka_vpravo = (filter_down * smerovka_vpravo + analogRead(2)) / (filter_down + 1) ;
      smerovka_vlavo  = (filter_down * smerovka_vlavo  + analogRead(3)) / (filter_down + 1) ;
    }

    /*Serial.print("\t");

      Serial.print(" smerovka_vpravo:  ");
      Serial.print(smerovka_vpravo);
      Serial.print("\t");
      Serial.print(" smerovka_vlavo:  ");
      Serial.println(smerovka_vlavo);*/


    if (mode == MODE_RIGHT || mode == MODE_GIVEN_RIGHT) {

      // Drzanie sa pravej hrany pasky

      if (mode != mode_old ) {
        Serial.println("MODE_RIGHT");
        Serial.println(mode);
        mode_old = mode;
        if (mode == MODE_RIGHT)
          shine_LED(RRED);
        else
          shine_LED(RBLUE | RRED);
      }


      //cast hysterezy
      if (mode == MODE_RIGHT && smerovka_vpravo < hysteresis_down ) {
        mode = MODE_STRAIGHT;
      } else if (mode == MODE_GIVEN_RIGHT && smerovka_vpravo < hysteresis_down && smerovka_vlavo < hysteresis_down) {
        mode = MODE_STRAIGHT;
      }

      calc_speed_right();

    } else if (mode == MODE_LEFT || mode == MODE_GIVEN_LEFT) {

      // Drzanie sa lavej hrany pasky

      if (mode != mode_old ) {
        Serial.println("MODE_LEFT");
        Serial.println(mode);
        mode_old = mode;
        if (mode == MODE_LEFT) {
          shine_LED( LRED);
        } else {
          shine_LED(LBLUE | LRED);
        }

      }
      //Serial.print("Dolava ");

      //cast hysterezy
      if (mode == MODE_LEFT && smerovka_vlavo < hysteresis_down ) {
        mode = MODE_STRAIGHT;
      } else if (mode == MODE_GIVEN_LEFT && smerovka_vpravo < hysteresis_down && smerovka_vlavo < hysteresis_down) {
        mode = MODE_STRAIGHT;
      }

      calc_speed_left();



    } else if (mode == MODE_STRAIGHT) {

      if (mode != mode_old ) {
        Serial.println("MODE_STRAIGHT");
        Serial.println(mode);
        mode_old = mode;
        shine_LED(LBLUE | RBLUE);
      }

      //Serial.print("No smer ");

      if (smerovka_vpravo > hysteresis_up && smerovka_vlavo < hysteresis_down) {
        mode  = MODE_RIGHT;

      } else if (smerovka_vlavo > hysteresis_up && smerovka_vpravo < hysteresis_down) {
        mode = MODE_LEFT;

      } else if (smerovka_vpravo > hysteresis_up && smerovka_vlavo > hysteresis_down) {

        mode = modes_arr[modes_counter % 2];
        modes_counter++;
        if(mode == MODE_KORBA){
          stepper_seq();
          mode = MODE_GIVEN_LEFT;
        }
          
      }

      calc_speed_straight();
    }

    delR = min(delR, periode);
    delL = min(delL, periode);
    Serial.print("  ");
    Serial.print(range_slow);
    Serial.print("  ");


    if (delR > delL) {
      Serial.print(" dolava \t");
    } else if (delR < delL) {
      Serial.print(" doprava \t");
    } else {
      Serial.print(" rovno \t");
    }

    Serial.print("\t");
    Serial.print(avgR);
    Serial.print(" R");
    Serial.print("\t");
    Serial.print(avgL);
    Serial.print(" L");


    Serial.print("\t");
    Serial.print(delR);
    Serial.print(" delR");
    Serial.print("\t");
    Serial.print(delL);
    Serial.println(" delL");


  }
}





void calc_speed_right() {
  if (avgL >= target_high && avgR <= target_low) { //chod rovno
    delR = min_speed;
    delL = min_speed;

  } else if (avgL <= target_high && avgL >= target_low && avgR <= target_low) { //ak je mimo kraja, viacej vlavo
    delL = min_speed - min(P_koef_sub_hrana * (target_high - avgL), min_speed);
    delR = min_speed + P_koef_add_hrana * (target_high - avgL);

  } else if (avgL >= target_high && avgR >= target_low) {                       //ak je trosku mimo stredu, viacej vpravo
    delL = min_speed + P_koef_add_hrana * (avgR - target_low);
    delR = min_speed - min(P_koef_sub_hrana * (avgR - target_low), min_speed);

  } else if (avgL <= target_low && avgR <= target_low) {    //ak je mimo ciary pokracuj v predoslom smere
    ;
    //delR = 0;
    //delL = 0;
  }
}


void calc_speed_left() {

  if (avgR >= target_high  &&  avgL <= target_low) { //chod rovno
    delR = min_speed;
    delL = min_speed;

  } else if (avgR <= target_high && avgR >= target_low && avgL <= target_low) { //ak je mimo kraja, viacej vlavo
    delR = min_speed - min(P_koef_sub_hrana * (target_high - avgR), min_speed);
    delL = min_speed + P_koef_add_hrana * (target_high - avgR);

  } else if (avgR >= target_high && avgL >= target_low) {                     //ak je trosku mimo stredu, viacej vpravo
    delR = min_speed + P_koef_add_hrana * (avgL - target_low);
    delL = min_speed - min(P_koef_sub_hrana * (avgL - target_low), min_speed);


  } else if (avgL <= target_low && avgR <= target_low) {                    //ak je mimo ciary pokracuj v predoslom smere
    ;//delR = 0;
    Serial.print(" POKRACUJEM  ");
    //delL = 0;
  }
}

void calc_speed_straight() {
  if (avgR >= target && avgL >= target) {                               //chod rovno
    delR = min_speed;
    delL = min_speed;

  } else if (avgR >= target && avgL < target) {                            //ak je to trosku mimo stredu, viacej vlavo
    delR = min_speed - min(P_koef_sub * (target - avgL), min_speed);
    delL = min_speed + P_koef_add * (target - avgL);

  } else if (avgR < target && avgL >= target) {                           //ak je to trosku mimo stredu, viacej vpravo
    delR = min_speed + P_koef_add * (target - avgR);
    delL = min_speed -  min(P_koef_sub * (target - avgR), min_speed);

  } else if (avgR < target && avgL < target && avgR < 100 && avgL < 100) { //ak su obe mimo pokracuj v smere kam si siel
    ;//delR = 0;
    Serial.print(" POKRACUJEM  ");
    // delL = 0;

  } else if (avgR < target && avgL < target && avgR > avgL) {             //ak je to mimo vlavo
    delR = min_speed - min(P_koef_sub * (target - avgL), min_speed); //0
    delL = min_speed + P_koef_add * (target - avgL);

  } else if (avgR < target && avgL < target && avgR < avgL) {            //ak je to mimo vpravo
    delR = min_speed + P_koef_add * (target - avgR);
    delL = min_speed - min(P_koef_sub * (target - avgR), min_speed); //0
  }
}

// stara sa o detekciu prekazok, ak na nejaku narazi tak prebera vedenie robota a postara sa o obidenie prekazky
// ak ziadnu prekazku nedetekoval tak konci
// ak detekoval potencialnu prekazku tak nastavuje hodnotu range_slow ktora spomali auto az dokedy bud nedetekuje prekazku uplne
// alebo prekazka zmizne
// taktiez nastavuje aj num_of_cycles - cim blizsie ku prekazke, tym menej cyklov
void obstacle_detection() {


  sonar();

  if (distance > DIST_target &&  distance < DIST_target + 15) {
    // ak este nenarazil na prekazku ale prekazka je uz blizko

    num_of_cycles = max(250 - (DIST_target - distance) * 10, 100);
    range_slow    = min((DIST_target + 15 - distance  ) * 9, 80);
    

    Serial.print(num_of_cycles );
    Serial.print(" " );
    Serial.print(range_slow );
    Serial.print(" " );
    Serial.print(distance);
    Serial.println(" cm");


  } else if (distance < DIST_target) {
    //ak uz narazil na prekazku

    delR = 0;
    delL = 0;

    stop_motor();

    obstacle_seq();
    range_slow = 0;
    return;

  } else {
    num_of_cycles = 500 ;
    range_slow    = 0;
  }
}

// sekvencia ktora sa vykona ak robot natrafi na prekazku
void obstacle_seq() {
  // na zaciatku nejake zapiskanie
  //sing_a_song();
  delay(5000);

  // nasnimanie vzdialenosti
  int dst = 15;
  for (int i = 0; i < 10; i++) {
    sonar();

    //vyfiltrovanie vzdialenosti
    dst = (9 * dst + distance) / (9 + 1);
  }

  //ak tam uz prekazka nie je tak ide sa dalej
  if (dst > 15)
    return;

  //zistenie ci nahodou robot nestoju na pomocnych pasikoch ktore by udavali jeho nasledovny smer
  for (int i = 0 ; i < 20; i++) {
    //filter na spomalenie rastu hodnoty
    smerovka_vpravo = (9 * smerovka_vpravo + analogRead(2)) / (9 + 1) ;
    smerovka_vlavo  = (9 * smerovka_vlavo +  analogRead(3)) / (9 + 1) ;


    /*Serial.print("\t");
      Serial.print(smerovka_vpravo);
      Serial.print(" R");
      Serial.print("\t");
      Serial.print(smerovka_vlavo);
      Serial.println(" L");*/
  }

  int dstR = 0;
  int dstL = 0;


  if (smerovka_vlavo < 500 &&  smerovka_vpravo < 500) {
    // ak sa ani jeden pasik nenachadza tak sa robot rozhodne sam
    // podla toho na ktorej strane je vacsia volna vzdialenost tak tam pojde

    myservo.write(0);
    delay(1000);

    // ziskanie vzdialenosti doprava od robota
    for (int i = 0; i < 30; i++) {
      sonar();
      dstR = (9 * dstR + distance) / (9 + 1);
    }

    myservo.write(180);
    delay(1000);

    // ziskanie vzdialenosti dolava od robota
    for (int i = 0; i < 30; i++) {
      sonar();
      dstL = (9 * dstL + distance) / (9 + 1);
    }

    if (dstR < dstL) {
      rotate_left();

      myservo.write(0);
      delay(500);

      follow_right();

    } else {
      rotate_right();

      myservo.write(180);
      delay(500);

      follow_left();
    }


  } else if (smerovka_vlavo > 700 &&  smerovka_vpravo < 500) {
    // ak pomocny pasik je nalavo od robota

    rotate_left();

    myservo.write(0);
    delay(500);

    follow_right();

  } else if (smerovka_vlavo < 500 &&  smerovka_vpravo > 700) {
    // ak pomocny pasik je napravo od robota

    rotate_right();

    myservo.write(180);
    delay(500);

    follow_left();

  } else if ( (smerovka_vlavo > 500 &&  smerovka_vpravo > 700) || (smerovka_vlavo > 700 &&  smerovka_vpravo > 500) ) {
    ; // podla pamate

    rotate_right();

    myservo.write(180);
    delay(500);

    follow_left();
    
  }

}

// robot sa bude drzat steny co je nalavo od neho
void follow_left() {
  avgR = 0;
  avgL = 0;

  while (1) {
    myservo.write(170); // nastavenie serva dolava
    sonar();

    // orezanie maximalnej hodnoty ktoru moze sonar nasnimat
    // bez toho vdaka napr chybnej vzdialenosti alebo velkej hodnote by mohol robot zabludit
    distance = min(distance, 30);

    // P regulator na udrzanie si vzdialenosti od steny
    if (distance < DIST_target_wall) {
      delR = min_speed - min(DIST_koef_add * (abs(distance - DIST_target_wall)) + not_ciara_koef, min_speed);
      delL = min_speed + min(DIST_koef_sub * (abs(distance - DIST_target_wall)) + not_ciara_koef, min_speed);

    } else {
      delR = min_speed + min(DIST_koef_sub * (abs(distance - DIST_target_wall)) + not_ciara_koef, min_speed);
      delL = min_speed - min(DIST_koef_add * (abs(distance - DIST_target_wall)) + not_ciara_koef, min_speed);

    }

    for (int i = 0; i < 25; i++) {
      // snimanie honot z hlavneho snimaca,
      // na hodnoty je pouzity filter na spomalenie rastu hodnoty
      avgR = (9 * avgR + analogRead(1)) / (9 + 1) ;
      avgL = (9 * avgL + analogRead(0)) / (9 + 1) ;

      // ak narazil na ciaru tak sa nastavi zatocenie doprava a funkcia sa konci
      if (avgR > 700 || avgL > 700) {
        delR = 0;
        delL = 750;

        for (int j = 0; j < 100; j++) {
          Serial.println(" tuna sooooooooooooom");
          motor_forward_PWM();
        }

        return;
      }

      motor_forward_PWM();
    }
  }
}


// robot sa bude drzat steny co je napravo od neho
void follow_right() {
  avgR = 0;
  avgL = 0;

  while (1) {
    myservo.write(0);   // nastavenie serva doprava
    sonar();

    // orezanie maximalnej hodnoty ktoru moze sonar nasnimat
    // bez toho vdaka napr chybnej vzdialenosti alebo velkej hodnote by mohol robot zabludit
    distance = min(distance, 30);

    // P regulator na udrzanie si vzdialenosti od steny
    if (distance < DIST_target_wall) {
      delR = min_speed + min(DIST_koef_add * (abs(distance - DIST_target_wall)) + not_ciara_koef, min_speed);
      delL = min_speed - min(DIST_koef_sub * (abs(distance - DIST_target_wall)) + not_ciara_koef, min_speed);

    } else {
      delR = min_speed - min(DIST_koef_sub * (abs(distance - DIST_target_wall)) + not_ciara_koef, min_speed);
      delL = min_speed + min(DIST_koef_add * (abs(distance - DIST_target_wall)) + not_ciara_koef, min_speed);

    }

    for (int i = 0; i < 25; i++) {
      // snimanie honot z hlavneho snimaca,
      // na hodnoty je pouzity filter na spomalenie rastu hodnoty
      avgR = (9 * avgR + analogRead(1)) / (9 + 1) ;
      avgL = (9 * avgL + analogRead(0)) / (9 + 1) ;

      // ak narazil na ciaru tak sa nastavi zatocenie doprava a funkcia sa konci
      if (avgR > 700 || avgL > 700) {
        delR = 750;
        delL = 0;

        for (int j = 0; j < 100; j++)
          motor_forward_PWM();

        return;
      }

      motor_forward_PWM();
    }
  }
}

// otoci robota dolaba o zhruba 90 deg
// funkcia nema ziadnu spatnu vazbu takze zalezi iba od poctu cyklov
void rotate_left() {

  for (int i = 0; i < 550; i++) {

    digitalWrite(MOTOR_L1, HIGH);
    digitalWrite(MOTOR_R2, HIGH);


    digitalWrite(MOTOR_R1, LOW);
    digitalWrite(MOTOR_L2, LOW);

    int rotation_speed = 300 * koef;

    delayMicroseconds (rotation_speed);

    digitalWrite(MOTOR_R2, LOW);
    digitalWrite(MOTOR_L1, LOW);
    delayMicroseconds(periode - rotation_speed);
  }

  stop_motor();
}


// otoci robota doprava o zhruba 90 deg
// funkcia nema ziadnu spatnu vazbu takze zalezi iba od poctu cyklov
void rotate_right() {

  for (int i = 0; i < 550; i++) {
    digitalWrite(MOTOR_L1, LOW);
    digitalWrite(MOTOR_R2, LOW);


    digitalWrite(MOTOR_R1, HIGH);
    digitalWrite(MOTOR_L2, HIGH);

    int rotation_speed = 300 * koef;

    delayMicroseconds (rotation_speed);

    digitalWrite(MOTOR_R1, LOW);
    digitalWrite(MOTOR_L2, LOW);
    delayMicroseconds(periode - rotation_speed);
  }

  stop_motor();
}

// vytvori kratky brzdiaci impulz
void stop_motor() {

  digitalWrite(MOTOR_R1, HIGH);
  digitalWrite(MOTOR_L1, HIGH);

  digitalWrite(MOTOR_L2, HIGH);
  digitalWrite(MOTOR_R2, HIGH);
  delay(1);
}


// hlavna funkcia pre pohyb robota smerom vpred
// funkcia zoberie delR a delL a na kazdy motor pusti PWM ktoreho dlzka zodpoveda periode
// a za stav v jednotke zodpoveda velkost delR a delL
// pre kombinacie pohybu viz. https://www.onsemi.com/pdf/datasheet/lv8406t-d.pdf
void motor_forward_PWM() {

  digitalWrite(MOTOR_R1, LOW);    //nastavenie motorov
  digitalWrite(MOTOR_L1, LOW);

  if (delR == 0 && delL == 0) {
    digitalWrite(MOTOR_L2, LOW);
    digitalWrite(MOTOR_R2, LOW);
  } else {
    digitalWrite(MOTOR_L2, HIGH);
    digitalWrite(MOTOR_R2, HIGH);
  }


  if (delR > delL) {                //perioda pwm
    delayMicroseconds(delL);
    digitalWrite(MOTOR_L2, LOW);

    delayMicroseconds(delR - delL);

    digitalWrite(MOTOR_R2, LOW);
    delayMicroseconds(periode - delR);

  } else if (delR < delL) {
    delayMicroseconds(delR);
    digitalWrite(MOTOR_R2, LOW);

    delayMicroseconds(delL - delR);
    digitalWrite(MOTOR_L2, LOW);
    delayMicroseconds(periode - delL);

  } else {
    delayMicroseconds(delR);

    digitalWrite(MOTOR_L2, LOW);
    digitalWrite(MOTOR_R2, LOW);
    delayMicroseconds(periode - delR);
  }
}


// hlavna funkcia pre pohyb robota smerom vpred
// funkcia zoberie delR a delL a na kazdy motor pusti PWM ktoreho dlzka zodpoveda periode
// a za stav v jednotke zodpoveda velkost delR a delL
// pre kombinacie pohybu viz. https://www.onsemi.com/pdf/datasheet/lv8406t-d.pdf
void motor_backward_PWM() {

  digitalWrite(MOTOR_R2, LOW);    //nastavenie motorov
  digitalWrite(MOTOR_L2, LOW);

  if (delR == 0 && delL == 0) {
    digitalWrite(MOTOR_L1, LOW);
    digitalWrite(MOTOR_R1, LOW);
  } else {
    digitalWrite(MOTOR_L1, HIGH);
    digitalWrite(MOTOR_R1, HIGH);
  }


  if (delR > delL) {                //perioda pwm
    delayMicroseconds(delL);
    digitalWrite(MOTOR_L1, LOW);

    delayMicroseconds(delR - delL);

    digitalWrite(MOTOR_R1, LOW);
    delayMicroseconds(periode - delR);

  } else if (delR < delL) {
    delayMicroseconds(delR);
    digitalWrite(MOTOR_R1, LOW);

    delayMicroseconds(delL - delR);
    digitalWrite(MOTOR_L1, LOW);
    delayMicroseconds(periode - delL);

  } else {
    delayMicroseconds(delR);

    digitalWrite(MOTOR_L1, LOW);
    digitalWrite(MOTOR_R1, LOW);
    delayMicroseconds(periode - delR);
  }
}

void sing_a_song() {

  // short na arduine ma 16 bitov
  // delay-e v pesnicke su nasobky 150 ms takze ak predelime delay-e 150 dostaneme cisla z intervalu 0-15 a to sa zmesti na 4 bity
  // tym padom sa daju do jedneho shoru zapisat 4 cisla ktore sa potom daju vybrat pomocou masky

  unsigned short d;
  unsigned short x = 15;
  for (int i = 0; i < TONES_LEN; i++) {
    d = (( pgm_read_word_near(dels + i / 4) & (x << ((i % 4) * 4)) ) >> ((i % 4) * 4)) * 150;
    tone(PIEZO_PIN, pgm_read_word_near(tones + i), d );
    delay(d);
  }


}


// nastavi ledky podla parametra flags
void shine_LED(unsigned flags) {
  // Ledky su zoradene takto: RBLUE, LBLUE, LRED, PRAZDNE, PRAZDNE, RRED
  // pricom na prazdne sa davaju nuly
  // kazda ledka berie 8 bitov

  // https://www.tme.eu/sk/details/ws2818b/drivery-led/worldsemi/
  // https://www.tme.eu/Document/ddc250a349c0084fadc3ded4c327f335/WS2818B.pdf

  // hoci na toto existuje library
  // https://github.com/FastLED/FastLED

  int i = 0;
  int k = 0;
  unsigned mask = 1;

  PORTC = PORTC & B11011111; // reset kontrolera
  delayMicroseconds(30);

  // opakovanie instrukcii PORTC = PORTC ...; je kvoli kommunikacii s mikrocipom, pretoze neexistuje delay v nanosekundach
  for (k = 0; k < 4; k++) {
    if (flags & mask ) {        //zapni led

      //horne 2 bity led som nastavil na 0 lebo moc svietili
      PORTC = PORTC | B00100000;
      PORTC = PORTC | B00100000;
      PORTC = PORTC | B00100000;
      PORTC = PORTC & B11011111;
      delayMicroseconds(2);
      PORTC = PORTC | B00100000;
      PORTC = PORTC | B00100000;
      PORTC = PORTC | B00100000;
      PORTC = PORTC & B11011111;
      delayMicroseconds(2);
      PORTC = PORTC | B00100000;
      PORTC = PORTC | B00100000;
      PORTC = PORTC | B00100000;
      PORTC = PORTC & B11011111;
      delayMicroseconds(2);

      for (i = 0; i < 5; i++) {
        PORTC = PORTC | B00100000;
        delayMicroseconds(2);
        PORTC = PORTC & B11011111;
        PORTC = PORTC & B11011111;
        PORTC = PORTC & B11011111;
        PORTC = PORTC & B11011111;
      }

    } else {                   // vypni led
      for (i = 0; i < 8; i++) {
        PORTC = PORTC | B00100000;
        PORTC = PORTC | B00100000;
        PORTC = PORTC | B00100000;
        PORTC = PORTC & B11011111;
        delayMicroseconds(2);
      }
    }

    // vypnut ledky ktore niesu pripojene
    if (k == 2) {
      for (i = 0; i < 16; i++) {
        PORTC = PORTC | B00100000;
        PORTC = PORTC | B00100000;

        PORTC = PORTC & B11011111;
        delayMicroseconds(2);
      }
    }

    mask = mask << 1;
  }
}

//sekvencia pre vyklad nakladu
void stepper_seq() {
  stop_motor();
  delay(1000);
  
  stepper_UP(STEPPER_STEPS);
  stepper_set(0, 0, 0, 0);
  delay(2000);
  
  stepper_DOWN(STEPPER_STEPS);
  stepper_set(0, 0, 0, 0);
  delay(2000);
}



// nastavi jednotlive vstupy krokoveho motora na zadane hodnoty
void stepper_set(int a, int b, int c, int d) {
  digitalWrite(STEPPER_1, a);
  digitalWrite(STEPPER_2, b);
  digitalWrite(STEPPER_3, c);
  digitalWrite(STEPPER_4, d);
}


// zodvihne korbu
void stepper_UP(unsigned steps) {

  for (int i = 0; i < steps; i++) {
    stepper_set(0, 0, 0, 1);
    delay(2);
    stepper_set(0, 0, 1, 0);
    delay(2);
    stepper_set(0, 1, 0, 0);
    delay(2);
    stepper_set(1, 0, 0, 0);
    delay(2);
  }
}


// polozi korbu
void stepper_DOWN(unsigned steps) {

  for (int i = 0; i < steps; i++) {
    stepper_set(1, 0, 0, 0);
    delay(2);
    stepper_set(0, 1, 0, 0);
    delay(2);
    stepper_set(0, 0, 1, 0);
    delay(2);
    stepper_set(0, 0, 0, 1);
    delay(2);

  }

}



// ziska vzdialenost zo sonara
void sonar() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);


  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.034 / 2;

  // Serial.print(distance);
  // Serial.print(" cm");

  // pauza kvoli kratkym vzdialenostiam
  delayMicroseconds(2000);
}






/*Serial.print("  ");
  Serial.print(range_slow);
  Serial.print("  ");


  if (delR > delL) {
  Serial.print(" dolava \t");
  } else if (delR < delL) {
  Serial.print(" doprava \t");
  } else {
  Serial.print(" rovno \t");
  }

  Serial.print("\t");
  Serial.print(avgR);
  Serial.print(" R");
  Serial.print("\t");
  Serial.print(avgL);
  Serial.print(" L");


  Serial.print("\t");
  Serial.print(delR);
  Serial.print(" delR");
  Serial.print("\t");
  Serial.print(delL);
  Serial.println(" delL");*/
