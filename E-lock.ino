#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <gfxfont.h>
// default pass 1337
#include <Adafruit_Fingerprint.h>
#define serial_lock_pin 5
#define lock_pin 6
#define unlock_state 1
#define power_pin 7
#define led_pin 13
#define INTERUPT_UP 0x01
#define INTERUPT_ENTER 0x03
#define INTERUPT_DOWN 0x02
#define master_id_eeprom_address 0x01
int8_t master_id;
volatile int8_t interupt_event=0;
int8_t master_mode=0;
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);
Adafruit_SSD1306 display(4);
void setup() {
 pinMode(power_pin, OUTPUT);
 pinMode(lock_pin,OUTPUT);
 pinMode(led_pin,OUTPUT);
 pinMode(serial_lock_pin,INPUT_PULLUP);
 pinMode(2,INPUT_PULLUP);
 pinMode(3,INPUT_PULLUP);
 pinMode(18,INPUT_PULLUP);
 attachInterrupt(digitalPinToInterrupt(2),up_interupt_routine,FALLING);
 attachInterrupt(digitalPinToInterrupt(3),down_interupt_routine,FALLING);
 attachInterrupt(digitalPinToInterrupt(18),enter_interupt_routine,FALLING);
 digitalWrite(lock_pin,!unlock_state); //lock the lock
 digitalWrite(power_pin,1); // keep the power_on
 digitalWrite(led_pin,1);//indicate no need to hold button
  //intial
 int p=-1;
 Serial.begin(57600);
 EEPROM.begin();
 //EEPROM.write(master_id_eeprom_address,1);
 master_id=EEPROM.read(master_id_eeprom_address);
 display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
 if(!digitalRead(serial_lock_pin)) serial_handle();
 finger.begin(57600);
 display.clearDisplay();
 display.setCursor(0,0);
 display.setTextSize(1);
 display.setTextColor(WHITE);
 //check the sensor
 if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
    display.print("Sensor...ok");
    display.display();
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    display.print("Sensor...error");
    display.display();
    while (1);
        }
 //scan the fingerprint
 delay(500);
 display.clearDisplay();
 display.setCursor(0,0);
 display.println("Place your finger on sensor");
 while(p==-1)
 {
 display.clearDisplay();
 display.setCursor(0,0);
 display.println("Scanning....");
  Serial.println("Waiting for fingerprint");
  display.display();
  p=getFingerprintIDez();
  delay(100);
 }
 //finger_notfound
 if(p==FINGERPRINT_NOTFOUND)
 {
  Serial.println("Fingerprint not found");
 display.clearDisplay();
 display.setCursor(0,0);
 display.println("Invalid fingerprint");
 display.display();
 while(1); //stop
 }
 //master_finger found
 if(p==master_id) 
 {
  master_mode=1;
  Serial.println("Master mode enabled");
  display.clearDisplay();
 display.setCursor(0,0);
 display.println("Master mode enabled");
 display.display();
 delay(500);
 master_finger_handle();
 }
 //normal_finger_handle
else
{
display.clearDisplay();
 display.setCursor(0,0);
 display.print("Welcome ID:#"); display.print(finger.fingerID);
 display.display();
 delay(500);
 normal_finger_handle();
}
}
void loop() {
  
}
//______________check_fingerprint__________________________
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return FINGERPRINT_NOTFOUND;
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID; 
}
//__________________INTERUPT_HANDLE_________________________//
void up_interupt_routine()
{
  static long last_millis;
  if(interupt_event==0&&(millis()-last_millis>200))  interupt_event=INTERUPT_UP;
  else return;
  last_millis=millis();
}
void enter_interupt_routine()
{
  static long last_millis;
  if(interupt_event==0&&(millis()-last_millis>200)) interupt_event=INTERUPT_ENTER;
  else return;
  last_millis=millis();
}
void down_interupt_routine()
{
  static long last_millis;
  if(interupt_event==0&&(millis()-last_millis>200)) interupt_event=INTERUPT_DOWN;
  else return;
  last_millis=millis();
}
//_________________SERIAL_KEY_HANDLE_________________________//
void serial_handle()
{
  String Serial_data;
  while(Serial.available()>0) 
  {
    char c=Serial.read();
    Serial_data+=c;
  }
  if(strcmp(Serial_data.c_str(), "PASSWORD")==0) master_finger_handle();
  else
  {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("invalid password");
    display.display();
    while(1);
  }
}
//__________________MASTER_MODE_HANDLE________________________//
void master_finger_handle()
{
  static int menu_pos=0;
  interupt_event=0;
  while(1)
  {
    switch(interupt_event)
    {
      case INTERUPT_DOWN:
      {
        menu_pos++;
        if(menu_pos>=4) menu_pos=0;
        interupt_event=0;
        break;
      }
      case INTERUPT_UP:
      {
        menu_pos--;
        if(menu_pos<0) menu_pos=3;
        interupt_event=0;
        break;
      }
      case INTERUPT_ENTER:
      {
        switch(menu_pos)
        {
          case 0:
          {
            enroll_mode();
            break;
          }
          case 1:
          {
            delete_mode();
            break;
          }
          case 2:
          {
            set_master_finger();
            break;
          }
          case 3:
          {
            set_unlock_time();
            break;
          }
        }
        interupt_event=0;
        break;
      }
      case 0:
      {
        draw_menu(menu_pos);
        break;
        interupt_event=0;
      }
    }
  }
}
void normal_finger_handle()
{
  
}
//___________________SERVICE_FOR_MASTER_MODE______________________________
void draw_menu(int menu_pos)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print((menu_pos==0)?(">Enroll new finger"):("Enroll new finger"));
  display.setCursor(0,12);
  display.print((menu_pos==1)?(">Delete finger"):("Delete finger"));
  display.setCursor(0,24);
  display.print((menu_pos==2)?(">Set master finger"):("Set master finger"));
  display.setCursor(0,36);
  display.print((menu_pos==3)?(">Set unlock time"):("Set unlock time"));
  display.display();
}
void enroll_mode()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Enroll new finger");
  display.display();
  delay(500);
  int id=get_id_to_process(0);
  while(id==master_id)
  {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Error!");
  display.setCursor(0,10);
  display.print("This id is master");
  display.display();
  delay(1000);
  id=get_id_to_process(0);
  }
  //________CHOOSED_ID__________
  int sensor_status=-1;
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Place your finger.");
  display.display();
  do 
  {
    sensor_status=getFingerprintIDez();
  }
  while(sensor_status==-1);
  if(sensor_status!=FINGERPRINT_NOTFOUND)
  {
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Finger already enroll");
  display.setCursor(0,10);
  display.print("Slot #"); display.print(sensor_status);
  display.display();
  delay(2000);
  return;
  }
  do
  {
  do
  {
    sensor_status=finger.getImage();
  }
  while(sensor_status==FINGERPRINT_NOFINGER);
  if(sensor_status==FINGERPRINT_OK) break;
  //______ERROR_indicate
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("ERROR");
  display.setCursor(0,10);
  display.print("Place again");
  display.display();
  delay(1000);
  }
  while(sensor_status!=FINGERPRINT_OK);
  //________GET_FINGERPRINT_SUCCESS______
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Image taken");
  display.display();
  delay(600);
  //________PROCESS_FINGERPRINT___________
  sensor_status=finger.image2Tz(1);
  display.clearDisplay();
  display.setCursor(0,0);
  if(sensor_status!=FINGERPRINT_OK)
  {
    display.print("Can't process image");
    display.display();
    delay(1500);
    return;// Can't do anything else
  }
  //________PROCESS_SUCCESS_______________
  display.print("Image processed");
  display.display();
  delay(800);
  sensor_status=finger.getImage();
  while(sensor_status!=FINGERPRINT_NOFINGER)
  {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Remove finger");
    display.display();
    sensor_status=finger.getImage();
    delay(50);
  }
  //________RETAKE_IMAGE___________________
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Place again");
    display.display();
    sensor_status=finger.getImage();
    while(sensor_status==FINGERPRINT_NOFINGER)
    {
      sensor_status=finger.getImage();
      delay(50);
    }
    if(sensor_status!=FINGERPRINT_OK)
    {
      display.clearDisplay();
      display.setCursor(0,0);
      display.print("ERROR!");
      display.display();
      delay(1500);
      return;
    }
    sensor_status=finger.image2Tz(2);
    sensor_status=finger.createModel();
    //____________ERROR
    if(sensor_status!=FINGERPRINT_OK)
    {
      display.clearDisplay();
      display.setCursor(0,0);
      display.print("ERROR!");
      display.display();
      delay(1500);
      return;
    }
    //__________image_processed_____
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Fingerprint ready");
    display.display();
    delay(800);
    //__________store_id_____________
    sensor_status=finger.storeModel(id);
    if(sensor_status==FINGERPRINT_OK)
    {
      display.clearDisplay();
      display.setCursor(0,0);
      display.print("Fingerprint is stored");
      display.setCursor(0,10);
      display.print("#"); display.print(id);
      display.display();
      delay(2000);
      return;
    }
    else
    {
      display.clearDisplay();
      display.setCursor(0,0);
      display.print("ERROR!");
      display.display();
      delay(2000);
      return;
    }
}
void delete_mode()
{
  int sensor_status=0;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Delete finger");
  display.display();
  int id=get_id_to_process(1);
  while(id==master_id)
  {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("You can't delete master finger!");
    display.display();
    delay(2000);
    id=get_id_to_process(1);
  }
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Deleting...");
  display.display();
  sensor_status=finger.deleteModel(id);
  delay(500);
  display.clearDisplay();
  display.setCursor(0,0);
  if(sensor_status!=FINGERPRINT_OK)
  {
    display.print("ERROR");
    display.display();
    delay(2000);
    return;
  }
  else
  {
    display.print("Deleted #"); display.print(id);
    display.display();
    delay(1500);
    return;
  }
}
void set_master_finger()
{
 display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Set master finger");
  display.display();
  while(1);
}
void set_unlock_time()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Change unlock time");
  display.display();
  while(1);
}
int8_t get_id_to_process(int mode)
{
  interupt_event=0;
  static int id=1;
  while(interupt_event!=INTERUPT_ENTER)
  {
  if(interupt_event==INTERUPT_UP)
  {
    interupt_event=0;
    if(id!=127) id++;
  }
  if(interupt_event==INTERUPT_DOWN)
  {
    interupt_event=0;
    if(id!=1) id--;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(mode==0?"Choose id you want to save":"Choose id you want to delete");
  display.setCursor(0,20);
  display.setTextSize(2);
  display.print("#"); display.print(id);
  display.display();
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Choosed ID:"); display.print(id);
  display.display();
  delay(500);
  return id;
}







