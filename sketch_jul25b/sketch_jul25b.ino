#include <Servo.h>

//#include <Pushbutton.h>

#include <NewPing.h>

#include <DHT11.h>

#include <Wire.h>               // Biblioteca utilizada para fazer a comunicação com o I2C
#include <LiquidCrystal_I2C.h>  // Biblioteca utilizada para fazer a comunicação com o display 16x2

//Pushbutton button1(A1);

String temp_style = "celsius";
String temps[] = { "celsius", "fahrenh" };

byte menu_step = 0;
unsigned long select = 1;

long peoplePass = 0;

byte peopleInRoom = 100;
byte Entering = 0;


byte girando = 0;

int dist = 0;

bool mode = true;



#define servoM 10

Servo myservo;

#define Enter 9
#define Left 8
#define Right 7
#define ChangeMode 6

DHT11 dht11(2);

#define col 16     // Serve para definir o numero de colunas do display utilizado
#define lin 2      // Serve para definir o numero de linhas do display utilizado
#define ende 0x27  // Serve para definir o endereço do display.

LiquidCrystal_I2C lcd(ende, col, lin);  // Chamada da funcação LiquidCrystal para ser usada com o I2C


#define TRIGGER_PIN 3  // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN 4     // Arduino pin tied to echo pin on ping sensor.

unsigned int max_dis = 70;
unsigned int old_max_dis = max_dis;

#define pinoLux A0

NewPing *sonar;



byte calibragem = 0;

bool rotate = false;

int pessoasPassando = 0;


unsigned int lux = 800;

unsigned long pingTimer;

unsigned long runningTimer;

unsigned long checkingTimer;

unsigned long buttonTimer;

unsigned long alertTimer;

void setup()  //Incia o display
{

  changeMaxDistance(max_dis);
  Serial.begin(9600);

  myservo.attach(10);

  myservo.write(0);

  pinMode(Enter, INPUT_PULLUP);
  pinMode(Left, INPUT_PULLUP);
  pinMode(Right, INPUT_PULLUP);
  pinMode(ChangeMode, INPUT_PULLUP);


  lcd.init();       // Serve para iniciar a comunicação com o display já conectado
  lcd.backlight();  // Serve para ligar a luz do display
  lcd.clear();      // Serve para limpar a tela do display

  lcd.setCursor(4, 0);
  lcd.print("DACIGERA");
  lcd.setCursor(6, 1);
  lcd.print("v1.0");
  delay(1000);
  lcd.setCursor(2, 0);
  lcd.print("Calibragem");
  lcd.setCursor(6, 1);
  lcd.print("v1.1");
  delay(1000);
  lcd.clear();


  myservo.write(45);
  delay(1000);
  myservo.write(0);


  buttonTimer = runningTimer = checkingTimer = alertTimer = pingTimer = millis();  // Start now.;

  select = max_dis;

 
}

void changeMaxDistance(unsigned int max_distance) {
  if (sonar != NULL) {
    delete sonar;  // Deleta a instância anterior
  }
  sonar = new NewPing(TRIGGER_PIN, ECHO_PIN, max_distance);  // Cria uma nova instância com a nova distância máxima
}

void loop() {

  Serial.print("Dist => ");
  Serial.print(dist);

  Serial.print("Mdist => ");
  Serial.println(max_dis);

  if (max_dis != old_max_dis) {
    Serial.print(" ### new Max_dis => ");
    Serial.println(max_dis);
    old_max_dis = max_dis;
    changeMaxDistance(max_dis);
  }

  if (analogRead(pinoLux) > lux) {


    desligando();

  } else {



    if (menu_step != 0) {
      checking();
    }





    int temperature = 0;
    int humidity = 0;

    int error = dht11.readTemperatureHumidity(temperature, humidity);

    if (!error) {

      application(humidity, temperature);

    } else {
      // Print error message based on the error code.
      Serial.println(DHT11::getErrorString(error));
      lcd.print(error);
    }
  }
}

void Alert() {

  if (millis() >= alertTimer) {
    lcd.clear();
    lcd.print("APAGUE A LUZ");
    Serial.println("Apague a Luz");
    Serial.println(girando);
    Serial.print("Pessoas Fora: ");
    Serial.println(peoplePass);



    if (girando < 45) {
      girando = 45;
      myservo.write(90);
      lcd.noBacklight();
      delay(250);

    } else {
      girando = 0;
      myservo.write(0);
      lcd.backlight();
    }
  }
}

void checking() {


  if (pessoasPassando == 1) {
    myservo.write(0);
    delay(1000);
    pessoasPassando = 4;
    rotate = false;
  }


  if (!rotate) {

    myservo.write(0);
    max_dis = old_max_dis;

    sonar->ping_timer(echoCheck);
    pingTimer += 50;

  } else {
    myservo.write(45);
    old_max_dis = max_dis;

    delay(500);

    sonar->ping_timer(echoCheck);
    pingTimer += 50;
    delay(100);
  }

  if (pessoasPassando == 2) {
    if (millis() >= checkingTimer) {
      Serial.println("Passaram para o outro lado!");
      myservo.write(0);

      peoplePass = 0;


      girando = 0;
      rotate = false;
      pessoasPassando = 0;
    }
  }
}


void echoCheck() {  // Timer2 interrupt calls this function every 24uS where you can check the ping status.
  // Don't do anything here!
  if (sonar->check_timer()) {

    if (millis() >= pingTimer) {
      // Se o ping foi recebido


      dist = (sonar->ping_result / US_ROUNDTRIP_CM);
      Serial.print("Ping: ");
      Serial.print(dist);
      Serial.println(" cm");
      if (dist > 0 && !rotate) {
        Serial.println("Alguém passou!");
        rotate = true;
        checkingTimer = millis() + 5000;
        pessoasPassando = 2;
        return false;
      }
      if (dist > 0 && rotate) {
        Serial.println("Realmente passou!");
        checkingTimer = millis();
        pessoasPassando = 1;
        alertTimer = millis() + 10000;
        peoplePass++;
      }
    }
  }
  // Don't do anything here!
}

void desligando() {


  for (int i = 3; i > 0; i--) {
    lcd.clear();
    lcd.print("Desligando em");

    lcd.setCursor(7, 1);
    lcd.print(i);
    delay(1000);
  }


  lcd.noBacklight();
  peoplePass = 0;
}

void application(int humi, int temp) {

  if (millis() >= buttonTimer) {
    ChangeModeON();
  }




  if (peoplePass >= peopleInRoom && menu_step != 0) {
    Alert();
  } else {

    lcd.backlight();
    if (mode) {  //Modo Ambiente
      switch (menu_step) {

        case 1:
          AmbStatus(humi, temp);
          break;
        case 2:
          LuxStatus();
          break;
      }
    } else {  //Modo de Configuração
      switch (menu_step) {

        case 1:
          config();
          break;
        case 2:
          format();
          break;
        case 3:
          menu_step = 0;
          mode = true;
          distConfig(false);
          break;
        case 4:
          luxConfig();
          break;
        case 5:
          peopleSetup();
          break;
      }
    }
  }


  //Modo de Start
  if (menu_step == 0 && calibragem < 2) {

    if (mode) {
      if (calibragem == 1) {
        mode = false;
      } else {
        distConfig(true);
      }
    } else {


      if (calibragem == 1) {
        peopleSetup();
      } else {
        calibrando(false);
      }
    }
  } else if (menu_step == 0 && calibragem == 2) {
    if (mode) {
      distConfig(false);
    } else {
      calibrando(true);
    }
  }
}

void calibrando(bool newConfig) {

  lcd.clear();
  lcd.print("Test of Sensor:");


  if (max_dis < 100) {
    lcd.setCursor(5, 1);
  }
  if (max_dis < 1000 && max_dis > 10) {

    lcd.setCursor(4, 1);
  }
  if (max_dis > 999) {

    lcd.setCursor(3, 1);
  }


  sonar->ping_timer(caliCheck);
  pingTimer += 50;

  lcd.print("[ ");
  lcd.print(dist);
  lcd.print(" ]");

  if (millis() >= buttonTimer) {

    if (EnterON()) {
      if (!newConfig) {
        calibragem = 1;
      } else {
        menu_step = 1;
      }
      select = 1;
    }
  }
}

void caliCheck() {

  if (sonar->check_timer()) {

    if (millis() >= pingTimer) {
      // Se o ping foi recebido


      dist = (sonar->ping_result / US_ROUNDTRIP_CM);
      Serial.print("Ping: ");
      Serial.print(dist);
      Serial.println(" cm");
    }
  }
}

void peopleSetup() {
  if (millis() >= buttonTimer) {

    if (LeftON()) {

      if (select > 1) {
        select--;
      } else {
        select = 9;
      }
    }

    if (RightON()) {
      if (select < 9) {
        select++;
      } else {
        select = 1;
      }
    }

    if (EnterON()) {
      peopleInRoom = select;
      menu_step = 1;
      calibragem = 2;
    }
  }

  lcd.clear();
  lcd.print(" People in Room");
  lcd.setCursor(5, 1);
  lcd.print("< ");
  lcd.print(select);
  Serial.println(select);
  lcd.print(" >");
  Serial.println(peopleInRoom);
}



void format() {

  if (millis() >= buttonTimer) {

    if (LeftON()) {

      if (select > 0) {
        select--;
      } else {
        select = 1;
      }
    }

    if (RightON()) {
      if (select < 1) {
        select++;
      } else {
        select = 0;
      }
    }

    if (EnterON()) {
      temp_style = temps[select];
      menu_step = 1;
    }
  }

  lcd.clear();
  lcd.print("Select Format");
  lcd.setCursor(3, 1);
  lcd.print("< ");
  lcd.print(temps[select]);
  lcd.print(" >");
}


void distConfig(bool cali) {
  lcd.clear();
  if (millis() >= buttonTimer) {

    if (LeftON()) {

      if (select > 30) {
        select -= 10;
      } else {
        select = 330;
      }
    }

    if (RightON()) {

      if (select < 330) {
        select += 10;
      } else {
        select = 30;
      }
    }
    if (EnterON()) {
      max_dis = select;

      if (cali) {
        calibragem = 1;
      } else {
        menu_step = 1;
      }
    }
  }


  max_dis = select;


  lcd.print("Dist. of Sensor:");

  if (select < 100) {
    lcd.setCursor(5, 1);
  }
  if (select < 1000 && select > 10) {

    lcd.setCursor(4, 1);
  }
  if (select > 999) {

    lcd.setCursor(3, 1);
  }


  lcd.print("< ");
  lcd.print(select);
  lcd.print(" >");

  if (menu_step == 1) {
    select = 1;
  }

  if (calibragem == 1 && menu_step == 0) {
    select = 1;
  }
}

void luxConfig() {
  lcd.clear();
  if (millis() >= buttonTimer) {

    if (LeftON()) {

      if (select > 30) {
        select -= 10;
      } else {
        select = 2000;
      }
    }

    if (RightON()) {

      if (select < 2000) {
        select += 10;
      } else {
        select = 30;
      }
    }

    if (EnterON()) {
      lux = select;
      menu_step = 1;
    }
  }



  lcd.print("Lux of Sensor:");

  if (select < 100) {
    lcd.setCursor(5, 1);
  }
  if (select < 1000 && select > 10) {

    lcd.setCursor(4, 1);
  }
  if (select > 999) {

    lcd.setCursor(3, 1);
  }


  lcd.print("< ");
  lcd.print(select);
  lcd.print(" >");

  if (menu_step == 1) {
    select = 1;
  }
}

void config() {

  lcd.clear();
  if (select < 5) {
    lcd.print("Format Dist Lux>");
  } else {
    lcd.clear();
    lcd.print("< People");
  }

  if (millis() >= buttonTimer) {

    if (select < 2) {
      select = 2;
    }
    if (select > 5) {
      select = 5;
    }

    if (LeftON()) {

      if (select > 2) {
        select--;
      } else {
        select = 5;
      }
    }

    if (RightON()) {
      if (select < 5) {
        select++;
      } else {
        select = 2;
      }
    }

    if (EnterON()) {
      menu_step = select;

      Serial.print("Selecionado: ");
      Serial.println(menu_step);
    }

    if (select == 2 or select == 5) {
      lcd.setCursor(2, 1);
    } else if (select == 3) {
      lcd.setCursor(8, 1);
    } else if (select == 4) {
      lcd.setCursor(13, 1);
    }

    lcd.print("*");
    Serial.print("Selecionando: ");

    Serial.println(select);

    if (menu_step == 5) {
      select = peopleInRoom;
    }
    if (menu_step == 4) {
      select = lux;
    }
    if (menu_step == 3) {
      select = max_dis;
    }
  }
}

void AmbStatus(int humi, int temp) {

  if (millis() >= runningTimer) {  // pingSpeed milliseconds since last ping, do another ping.
    runningTimer += 7000;          // Set the next ping time.

    menu_step = 2;

    lcd.clear();  // Limpa o display até o loop ser reiniciado

    lcd.print("Atual: ");

    if (temp_style == "celsius") {
      lcd.print(temp);
      lcd.print("C");
    } else if (temp_style == "fahrenh") {
      lcd.print(round(temp * 1.8 + 32));
      lcd.print("F");
    }


    lcd.print(" ");
    lcd.print(humi);
    lcd.print("%");

    lcd.setCursor(0, 1);

    lcd.print("Ideal: ");

    if (temp_style == "celsius") {
      lcd.print(25);
      lcd.print("C");
    } else if (temp_style == "fahrenh") {
      lcd.print(round(25 * 1.8 + 32));
      lcd.print("F");
    }

    lcd.print(" ");
    lcd.print(55);
    lcd.print("%");

    //---------------------------//

    Serial.print(" Temperature: ");
    Serial.print(temp);
    Serial.print(" °C\tHumidity: ");
    Serial.print(humi);
    Serial.println(" %");
  }
}

void LuxStatus() {


  if (millis() >= runningTimer) {  // pingSpeed milliseconds since last ping, do another ping.
    runningTimer += 7000;          // Set the next ping time.

    menu_step = 1;

    lcd.clear();  // Limpa o display até o loop ser reiniciado

    lcd.print("Lux Atual: ");

    lcd.print(analogRead(pinoLux));

    lcd.setCursor(0, 1);

    lcd.print("Lux Defi.: ");

    lcd.print(lux);

    //---------------------------//


    if (analogRead(pinoLux) > lux) {
      Serial.println("Está Apagada");
    } else {
      Serial.println("Está Acesa");
    }
  }
}
bool EnterON() {



  if (!digitalRead(Enter)) {

    buttonTimer = millis() + 300;

    return true;
  }

  return false;
}


bool RightON() {


  if (!digitalRead(Right)) {

    buttonTimer = millis() + 150;

    return true;
  }

  return false;
}


bool LeftON() {

  if (!digitalRead(Left)) {

    buttonTimer = millis() + 150;

    return true;
  }
  return false;
}


bool ChangeModeON() {



  if (!digitalRead(ChangeMode)) {

    buttonTimer = millis() + 300;

    mode = !mode;

    if (menu_step != 0) {
      menu_step = 1;
      select = 1;
    }

    return true;
  }

  return false;
}