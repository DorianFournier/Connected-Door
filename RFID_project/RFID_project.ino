  /* Dorian Fournier -- RFID Projet for "connected door" */
  
  #include <SPI.h>                                  // bibliothéque pour communication avec appareils SPI (Serial Peripheral Interface)
  #include <MFRC522.h>                              // bibliothéque pour capteur RFID
  #include <Servo.h>                                // bibliothéque pour servomoteur
  #include <LiquidCrystal.h>                        // bibliothéque pour l'écran LCD
  #include <Keypad.h>                               // bibliothéque pour la membrane
  
  #define RST_PIN 5                                 
  #define SS_PIN 53
  
  #define VERT 38                                   
  #define ROUGE 39
  #define BUZZER 48
  #define VERT2 A1
  #define ROUGE2 A2
  
  Servo mon_servo;
  LiquidCrystal lcd(2,3,6,7,11,12);                 //définition des pins utilisés pour l'écran LCD
  MFRC522 capteur_rfid(SS_PIN, RST_PIN);    
  
  byte sept_seg_digits[6] = {   B11111100,  // = 0   
                                B01100000,  // = 1    
                                B11011010,  // = 2
                                B11110010,  // = 3
                                B01100110,  // = 4
                                B10110110,  // = 5
                             };
                               
  int latchPin = 9;                                   // connecté au pin ST_CP du 74HC595
  int clockPin = 10;                                  // connecté au pin SH_CP du 74HC595  74hc595
  int dataPin = 8;                                    // connecté au pin DS du 74HC595
  
  int valeurRouge;                                    
  int valeurVerte;
  
  int essais_restant = 3;                          
  
  const int tailleCombinaison = 6;
  int combinaisonCode[tailleCombinaison] = {49, 50, 51, 52, 53, 54};    // combinaison écrite en caractères ASCII
  
  int nouvelleCombinaisonCode[tailleCombinaison];
  char nombre[tailleCombinaison];
  int right = 0;
  
  int nouvelleCombinaison = 0;
  byte fulllcd[8] = {B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111,};
  int nouvelleCombinaisonFinale = 0;
  int kposition = 0;
  
  const byte LIGNES = 4;                      // 4 lignes composent notre membrane
  const byte COLONNES = 4;                    // 4 colonnes composent notre membrane
  
  char keys[LIGNES][COLONNES] =               // emplacement des touches en fonctions des colonnes et lignes declarés auparavant
  {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
  };
  
  byte rowPins[LIGNES] = {30, 31, 32, 33};          // connection des pins des lignes sur l'Elegoo
  byte colPins[COLONNES] = {34, 35, 36, 37};        // connection des pins des colonnes sur l'Elegoo
  
  Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, LIGNES, COLONNES );
  
 
/*========================================= void setup() START =========================================*/

void setup() {

/*-- RFID INIT --*/
  SPI.begin();                        // début de la communication SPI
  capteur_rfid.PCD_Init();            // initialisation du capteur RFID
/*-- RFID INIT END --*/

/*-- SERVO INIT --*/  
  mon_servo.attach(13);               // servo moteur connecté au pin PWM num4
  mon_servo.write(180);               // bouger le servo moteur en position initiale : 180 degrés
/*-- SERVO INIT END --*/ 

/*-- LED RGB INIT --*/
  pinMode(ROUGE, OUTPUT);            
  pinMode(VERT, OUTPUT);
  digitalWrite(ROUGE, LOW);           
  digitalWrite(VERT, LOW);
  pinMode(VERT2,OUTPUT);
  pinMode(ROUGE2,OUTPUT);
/*-- LED RGB INIT END --*/

/*-- BUZZER INIT -- */  
  pinMode(BUZZER, OUTPUT);           
/*-- BUZZER INITnEND -- */  

/*-- 7 SEGMENTS INIT -- */
  pinMode(latchPin, OUTPUT);          
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
/*-- 7 SEGMENTS INIT END -- */

}

/*========================================= void setup() END =========================================*/

/*========================================= void loop() STRAT =========================================*/

void loop() 
{
label:
 
  message_accueil();      
  
  valeurRouge = 255;
  valeurVerte = 0;
  analogWrite(VERT, valeurVerte);         
  analogWrite(ROUGE, valeurRouge);    

  if (essais_restant == 0)              // s'il y a plus d'essais alors : 
  {

    for (int u = 0;  u<5 ; u++)         // affiche le "blocage systeme" en clignotement
      {
        lcd.clear();
        delay(300);
        lcd.print("     BLOCAGE   ");
        lcd.setCursor(0,1);
        lcd.print("     SYSTEME   ");
        delay(1000);
      }
   
   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("Combinaison :");

  while (right != tailleCombinaison)                 // tant que la combinaison ne correspond pas au code 
    {
     char k = keypad.getKey();
     
     if(k != 0)                                      
      { 
        if(k == 68)
          {
            kposition = 0;
          }
          else
          {
            nombre[kposition] = k;                    // sauvegarder le nombre dans la bonne position
            kposition++;                              // rajouter un a la position suivante pour le prochain nombre
            lcd.setCursor(kposition - 1,1);
            lcd.print("*");                           // écrire "*" à chaque touche appuyé
            //delay(50);                          
            
            if(kposition == tailleCombinaison)                        // regarder s'il y a bien eu 6 caractéres rentrés 
              { 
                kposition = 0;                                    
                for(int g = 0; g<tailleCombinaison; g++)          
                  {
                  if(nombre[g] == combinaisonCode[g]){ right++;}      // regarder si les caractères rentrés sont bon
                  }
                if(right == tailleCombinaison)
                {
                  ledValid();                                        
                  rightlcd();                                      
                  right = 0;
                  goto label;                                         
                }
                else
                {
                  ledNonValid();                                     
                  wronglcd();                                   
                  right = 0;
                }
              delay(100);
              }
           }
       }
    }

  }
   
  septSegs(5);
  
  if ( ! capteur_rfid.PICC_IsNewCardPresent() || ! capteur_rfid.PICC_ReadCardSerial() ) 
  {
    delay(50);
    return;
  }

  String UID = "";
  for (byte i = 0; i < capteur_rfid.uid.size; i++)
  {
   UID.concat(String(capteur_rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
   UID.concat(String(capteur_rfid.uid.uidByte[i], HEX));
  }
  
  UID.toUpperCase();

  if (UID.substring(1) == "A7 5B DC 79" || UID.substring(1) == "CC E6 F1 22")     //si un badge autorisé est detecté
  {
    lcd.clear();
    lcd.print("    BON BADGE   ");
    delay(1500);
    lcd.clear();
    
    if (UID.substring(1) == "A7 5B DC 79")
    {
      lcd.print("   M.FONTAINE   ");                             // affiche M.FONTAINE si c'est le badge de M.FONTAINE
    }
    else lcd.print("   M.ANTOINE   ");

    autorisation();                                              // appel fonction autorisation
  }

  else non_autorise();                                           // appel fonction non_autorise
    
}

/*========================================= void loop() END =========================================*/

/*========================================= Création de fonction =========================================*/

void septSegs(byte digit)
{
  digitalWrite(latchPin, LOW);                                    
  shiftOut(dataPin, clockPin, LSBFIRST, sept_seg_digits[digit]); 
  digitalWrite(latchPin, HIGH);                                   
}

void message_accueil()                   
{
  lcd.begin(16, 2);
  lcd.print("Scannez votre");
  lcd.setCursor(0,1);
  lcd.print("badge svp !");
}

void autorisation()
{
  lcd.setCursor(0,1);
  lcd.print("Ouverture porte ");     
  tone(BUZZER, 1000);
  delay(200);
  noTone(BUZZER);
  valeurRouge = 0;
  valeurVerte = 255;
  analogWrite(VERT, valeurVerte);
  analogWrite(ROUGE, valeurRouge);
  
  mon_servo.write(90);              
  
  for (byte digit = 6; digit > 0; --digit)      // actionne le compte à rebours affiché sur le 7 segments
  {
    delay(1000);
    septSegs(digit - 1); 
  }
  
  mon_servo.write(180);                         // retour du servo à sa position initiale soit ici 180 degrés
  delay(500);

  essais_restant = 3;
}

void non_autorise()     
{
  lcd.clear();
  essais_restant -= 1;                          // enleve un essais des que l'intrus passe sont badge
  for (int j = 0; j<5; j++)
  {
    lcd.print("Mauvais badge !");               // affiche du texte sur l'écran LCD
    lcd.setCursor(0,1);
    lcd.print("Essais restant:");
    lcd.print(essais_restant);
    
    valeurRouge = 255;
    valeurVerte = 0;
    analogWrite(VERT, valeurVerte);
    analogWrite(ROUGE, valeurRouge);
    tone(BUZZER,1000);
    
    delay(200);                                 //ici les lignes servent à faire clignoter la LED RGB et allumer / éteindre le buzzer
    
    valeurRouge = 0;
    valeurVerte = 0;
    analogWrite(VERT, valeurVerte);
    analogWrite(ROUGE, valeurRouge);
    noTone(BUZZER);
    lcd.clear();
    
    delay(200);
  }
  
  lcd.clear();
}

void lcdstartup()                       
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Combinaison :");
  essais_restant = 3;                           // remet le nombre d'essais_restant à 3 possibilités
}

void lcdenter()                                 
{
  lcd.clear();
  lcd.print("Entrer code :");
}

void lcdgranted()                             
{
  lcd.clear();
  lcd.print("Acces Autorise");
  lcd.setCursor(0,1);
  lcd.print("Appuyer sur 'D'");
}

void rightlcd()                                 // gére les évenements qu'il y a lorsque la combinaison est bonne
{
 lcdgranted();
 
 while(nouvelleCombinaisonFinale == 0)
 {
  delay(10);
  char o = keypad.getKey();
  if(o != 0){
    if(o == 67)
    {
      digitalWrite(VERT2,LOW);
      lcdenter();
      
      while(nouvelleCombinaison == 0)
      {
        enternewcode();
      }
      nouvelleCombinaison = 0;
    }
    
    if(o == 68)
    {
      nouvelleCombinaisonFinale = 1;
      digitalWrite(VERT2,LOW);
    }
  }
  }
  nouvelleCombinaisonFinale = 0;
 lcd.clear();
 lcdstartup();
}

void enternewcode()                             // s'occupe de la création du nouveau code
{
  char newk = keypad.getKey();
  if(newk != 0)
  {
    nouvelleCombinaisonCode[kposition] = newk;
    kposition++;
    lcd.setCursor(kposition,1);
    lcd.print(newk);
    
    if(newk == 68)
    {
      lcd.setCursor(0,1);
      lcd.print("        ");
      kposition = 0;
    }
    
    if(kposition == tailleCombinaison)
    {
      lcd.setCursor(kposition + 1,1);
      lcd.write(byte(0));
      delay(1000);
      kposition = 0;
      checknewcode();
    }
  }
}

void checknewcode()                               
{
  lcd.clear();
  lcd.print("Etes vous sur ?");
  lcd.setCursor(0,1);
  lcd.print("A: OUI / C: NON");
  
  while(nouvelleCombinaison == 0)
  {
    char newc = keypad.getKey();
    if(newc != 0)
    {
      if(newc == 65)
      {
        lcd.clear();
        lcd.print("Nouveau code");
        lcd.setCursor(0,1);
        lcd.print("Accepte");
        nouvelleCombinaison = 1;
        nouvelleCombinaisonFinale = 1;
        for(int j=0;j<tailleCombinaison;j++)
        {
          combinaisonCode[j] = nouvelleCombinaisonCode[j];
        }
          kposition = 0;
          delay(1500);
      }
      if(newc == 67)
      {
        delay(10);
        lcdgranted();
        nouvelleCombinaison = 1;
      }
    }
  }
}

void wronglcd()                                 // gére les évenements qu'il y a lorsque la combinaison est fausse
{
  lcd.clear();
  lcd.print("Acces Refuse ");
  lcd.setCursor(0,1);
  lcd.print("Appuyer sur 'D'");
  while(nouvelleCombinaisonFinale == 0)
  {
  delay(10);
  char o = keypad.getKey();
  if(o != 0)
  {
    if(o == 68)
    {
      nouvelleCombinaisonFinale = 1;
    }
  }
  }
  nouvelleCombinaisonFinale = 0;
  lcd.clear();
  lcdstartup();
  digitalWrite(ROUGE2,LOW);
}

void ledValid()                                // allume LED verte si combinaison correct 
{
  digitalWrite(VERT2, HIGH);
  digitalWrite(ROUGE2, LOW);
}
void ledNonValid()                             // allume LED rouge si combinaison fausse
{
  digitalWrite(ROUGE2, HIGH);
  digitalWrite(VERT2, LOW);
}
