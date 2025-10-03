#include <Arduino.h>
#include <LiquidCrystal.h>

// Configuración de pines LCD
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Pines para botones
#define BTN_JUMP   22
#define BTN_START  23
#define BTN_DUCK   24  // Botón para agacharse

// Buzzer
int buzzerPin = 3;

// Variables del juego
bool gameStarted = false;
bool gameOver = false;
int score = 0;
int highScore = 0;

// Sistema del dinosaurio
int dinoPos = 1;  // 0=Arriba, 1=Normal
bool isJumping = false;
bool isDucking = false;  // Estado de agachado
int jumpFrame = 0;
const int JUMP_DURATION = 15;

// Sistema de obstáculos
int obstaclePos = 25;
int obstacleType = 0;
int gameSpeed = 8;
unsigned long lastMoveTime = 0;
int obstacleGap = 0;

// Sistema para cactus dobles
bool doubleCactus = false;
int secondCactusPos = -5;

// ✅ SIMPLIFICADO: SISTEMA DE ESTRELLAS SIMPLE
int transitionPhase = 0;
bool starsActive = false;
int starTimer = 0;
bool isNight = false;

// Variables de animación
unsigned long lastUpdate = 0;
bool dinoFrame = false;

// MELODÍA DE INICIO
const int START_MELODY_NOTES = 8;
int startMelody[START_MELODY_NOTES] = {
  523, 659, 784, 1047, 784, 659, 523, 392
};
int startMelodyDurations[START_MELODY_NOTES] = {
  200, 200, 200, 300, 200, 200, 200, 300
};

// GRÁFICOS ORIGINALES
byte dinoRun1[8] = {
  B00000,
  B00111,
  B00111,
  B00110,
  B10111,
  B11111,
  B01110,
  B01100
};

byte dinoRun2[8] = {
  B00000,
  B00111,
  B00111,
  B00110,
  B10111,
  B11111,
  B01110,
  B00100
};

byte dinoJump[8] = {
  B00000,
  B00111,
  B00111,
  B00110,
  B10111,
  B11111,
  B01110,
  B01100
};

byte dinoDuck1[8] = {
  B00000,
  B00000,
  B00000,
  B00111,
  B10111,
  B11111,
  B11111,
  B01110
};

byte dinoDuck2[8] = {
  B00000,
  B00000,
  B00000,
  B00111,
  B10111,
  B11111,
  B11111,
  B00100
};

byte cactusSmall[8] = {
  B00100,
  B00100,
  B10101,
  B10101,
  B10101,
  B10101,
  B00100,
  B00100
};

byte cactusBig[8] = {
  B00100,
  B00101,
  B10101,
  B10101,
  B10101,
  B10101,
  B10101,
  B00100
};

byte birdLow[8] = {
  B01100,
  B11110,
  B01100,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

byte groundLine[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111
};

// Variables para controlar reinicio de gráficos
unsigned long lastGraphicsReset = 0;
const unsigned long GRAPHICS_RESET_INTERVAL = 30000;

// FUNCIÓN PARA REPRODUCIR MELODÍA DE INICIO
void playStartMelody() {
  for (int i = 0; i < START_MELODY_NOTES; i++) {
    tone(buzzerPin, startMelody[i], startMelodyDurations[i]);
    delay(startMelodyDurations[i] + 50);
    noTone(buzzerPin);
  }
}

// FUNCIÓN PARA TONO DE SALTO
void playJumpSound() {
  tone(buzzerPin, 523, 80);
}

// FUNCIÓN PARA TONO DE AGACHARSE
void playDuckSound() {
  tone(buzzerPin, 350, 60);
}

// FUNCIÓN MEJORADA PARA LEER BOTONES
bool isButtonPressed(int buttonPin) {
  return digitalRead(buttonPin) == LOW;
}

// CONFIGURAR GRÁFICOS
void setupGraphics() {
  lcd.createChar(0, dinoRun1);
  lcd.createChar(1, dinoRun2);  
  lcd.createChar(2, dinoJump);
  lcd.createChar(3, cactusSmall);
  lcd.createChar(4, cactusBig);
  lcd.createChar(5, birdLow);
  lcd.createChar(6, groundLine);
  lcd.createChar(7, dinoDuck1);
  
  lastGraphicsReset = millis();
}

// VERIFICAR Y REINICIAR GRÁFICOS
void checkAndResetGraphics() {
  if (millis() - lastGraphicsReset > GRAPHICS_RESET_INTERVAL) {
    setupGraphics();
    lastGraphicsReset = millis();
  }
}

// ✅ ACTUALIZADO: TRANSICIÓN CÍCLICA SOL-LUNA
void updateSunMoonTransition() {
  int cycleScore = score % 1000; // Ciclo cada 1000 puntos
  
  if (cycleScore < 200) {
    // Día completo (0-199 puntos)
    isNight = false;
    transitionPhase = 0;
  } else if (cycleScore < 500) {
    // Transición día a noche (200-499 puntos)
    isNight = false;
    transitionPhase = map(cycleScore, 200, 500, 0, 10);
    transitionPhase = constrain(transitionPhase, 0, 10);
  } else if (cycleScore < 700) {
    // Noche completa (500-699 puntos)
    isNight = true;
    transitionPhase = 10;
  } else {
    // Transición noche a día (700-999 puntos)
    isNight = true;
    transitionPhase = map(cycleScore, 700, 1000, 10, 0);
    transitionPhase = constrain(transitionPhase, 0, 10);
  }
}

// ✅ CORREGIDO: ACTUALIZAR ESTRELLAS
void updateStars() {
  starTimer++;
  
  if (starTimer > 10) { // Cada 11 frames (más frecuente)
    starTimer = 0;
    
    // ✅ ACTIVAR ESTRELLAS SOLO DURANTE NOCHE Y TRANSICIONES
    if (isNight || transitionPhase > 0) {
      starsActive = true;
    } else {
      starsActive = false;
    }
  }
}

// Dibujar suelo
void drawGround() {
  lcd.setCursor(0, 1);
  for(int i = 0; i < 16; i++) {
    lcd.write(6);
  }
}

// ✅ ACTUALIZADO: DIBUJAR SOL/LUNA CON TRANSICIONES CÍCLICAS
void drawSunMoon() {
  lcd.setCursor(0, 0);
  
  if (transitionPhase == 0) {
    lcd.print("O"); // Sol completo (día)
  } else if (transitionPhase <= 3) {
    lcd.print("D"); // Sol 3/4
  } else if (transitionPhase <= 6) {
    lcd.print(")"); // Media luna temprana
  } else if (transitionPhase <= 9) {
    lcd.print(")"); // Media luna avanzada
  } else {
    lcd.print(")"); // Luna completa (noche - espacio vacío)
  }
}

// ✅ CORREGIDO: DIBUJAR ESTRELLAS COMO PUNTOS SIMPLES
void drawStars() {
  if (!starsActive) return;
  
  // Array de posiciones posibles para estrellas (evitando sol/luna y puntuación)
  int starPositions[] = {4, 6, 8, 11, 13, 15};
  int numPositions = 6;
  
  // Dibujar algunas estrellas aleatorias
  for(int i = 0; i < numPositions; i++) {
    if(random(0, 100) < 40) { // 40% de probabilidad por posición
      lcd.setCursor(starPositions[i], 0);
      lcd.print("."); // Usar asterisco para mejor visibilidad
    }
  }
  
  // ✅ ESTRELLA ESPECIAL ENCIMA DE LA CABEZA DEL DINOSAURIO
  if(random(0, 100) < 60) { // 60% de probabilidad
    lcd.setCursor(2, 0); // Posición encima del dinosaurio (columna 2, fila 0)
    lcd.print(".");
  }
}

// ✅ CORREGIDO: DIBUJAR JUEGO SIN INTERFERIR CON ELEMENTOS
void drawGame() {
  lcd.clear();
  
  checkAndResetGraphics();
  
  // DIBUJAR SOL/LUNA PRIMERO
  drawSunMoon();
  
  // ✅ DIBUJAR ESTRELLAS DESPUÉS (solo durante noche y transiciones)
  if (starsActive) {
    drawStars();
  }
  
  // DIBUJAR PUNTUACIÓN
  lcd.setCursor(10, 0);
  lcd.print("S:");
  lcd.print(score);
  
  // DIBUJAR SUELO
  drawGround();
  
  // DIBUJAR DINOSAURIO
  lcd.setCursor(2, dinoPos);
  if (isJumping) {
    lcd.write(2);
  } else if (isDucking) {
    lcd.write(7);
  } else {
    lcd.write(dinoFrame ? 0 : 1);
  }
  
  // DIBUJAR OBSTÁCULO PRINCIPAL
  if(obstaclePos >= 0 && obstaclePos < 16) {
    if (obstacleType == 2) { // Pájaro
      lcd.setCursor(obstaclePos, 1);
      lcd.write(5);
    } else { // Cactus
      lcd.setCursor(obstaclePos, 1);
      switch(obstacleType) {
        case 0: lcd.write(3); break;
        case 1: lcd.write(4); break;
      }
    }
  }
  
  // DIBUJAR SEGUNDO CACTUS SI ESTÁ ACTIVO
  if (doubleCactus && secondCactusPos >= 0 && secondCactusPos < 16) {
    lcd.setCursor(secondCactusPos, 1);
    lcd.write(3);
  }
}

// SISTEMA DE SALTO
void handleJump() {
  static unsigned long lastJumpTime = 0;
  
  if (isButtonPressed(BTN_JUMP) && !isJumping && !isDucking && dinoPos == 1) {
    if (millis() - lastJumpTime > 200) {
      isJumping = true;
      jumpFrame = 0;
      dinoPos = 0;
      playJumpSound();
      lastJumpTime = millis();
    }
  }
  
  if (isJumping) {
    jumpFrame++;
    if (jumpFrame >= JUMP_DURATION) {
      isJumping = false;
      dinoPos = 1;
    }
  }
}

// SISTEMA PARA AGACHARSE
void handleDuck() {
  bool duckPressed = isButtonPressed(BTN_DUCK);
  
  if (duckPressed && !isJumping) {
    if (!isDucking) {
      playDuckSound();
    }
    isDucking = true;
  } else if (!duckPressed) {
    isDucking = false;
  }
}

// ACTUALIZAR ANIMACIÓN SOLO CUANDO NO ESTÁ AGACHADO
void updateAnimation() {
  if (millis() - lastUpdate > 200) {
    if (!isJumping && !isDucking) {
      dinoFrame = !dinoFrame;
    }
    lastUpdate = millis();
  }
}

// GENERAR OBSTÁCULOS CON CACTUS DOBLES ALEATORIOS
void generateObstacle() {
  if (obstaclePos < -3) {
    obstacleGap++;
    
    int minGap, maxGap;
    
    if (score < 20) {
      minGap = 2;
      maxGap = 5;
    } else if (score < 50) {
      minGap = 1;
      maxGap = 4;
    } else if (score < 100) {
      minGap = 0;
      maxGap = 3;
    } else {
      minGap = 0;
      maxGap = 2;
    }
    
    if (obstacleGap > random(minGap, maxGap)) {
      obstaclePos = 16;
      
      int obstacleRandom = random(0, 100);
      
      if (score < 15) {
        if (obstacleRandom < 60) {
          obstacleType = 0;
        } else if (obstacleRandom < 85) {
          obstacleType = 1;
        } else {
          obstacleType = 2;
        }
      } else if (score < 40) {
        if (obstacleRandom < 50) {
          obstacleType = 0;
        } else if (obstacleRandom < 80) {
          obstacleType = 1;
        } else {
          obstacleType = 2;
        }
      } else if (score < 80) {
        if (obstacleRandom < 40) {
          obstacleType = 0;
        } else if (obstacleRandom < 75) {
          obstacleType = 1;
        } else {
          obstacleType = 2;
        }
      } else {
        if (obstacleRandom < 35) {
          obstacleType = 0;
        } else if (obstacleRandom < 65) {
          obstacleType = 1;
        } else {
          obstacleType = 2;
        }
      }
      
      if ((obstacleType == 0 || obstacleType == 1) && random(0, 100) < 30) {
        doubleCactus = true;
        secondCactusPos = obstaclePos + 2;
      } else {
        doubleCactus = false;
        secondCactusPos = -5;
      }
      
      if (random(0, 100) < 50) {
        if (random(0, 100) < 80) {
          obstacleGap = random(0, 1);
        } else {
          obstacleGap = random(minGap + 1, maxGap + 2);
        }
      } else {
        obstacleGap = 0;
      }
      
      if (score > 0 && score % 20 == 0 && gameSpeed < 18) {
        gameSpeed++;
      }
    }
  }
}

// MOVER OBSTÁCULOS INCLUYENDO CACTUS DOBLES
void moveObstacles() {
  unsigned long currentTime = millis();
  
  int moveInterval = (1500 / gameSpeed);
  
  if (currentTime - lastMoveTime > moveInterval) {
    obstaclePos--;
    
    if (doubleCactus) {
      secondCactusPos--;
      if (secondCactusPos < -3) {
        doubleCactus = false;
      }
    }
    
    score++;
    if (score > highScore) {
      highScore = score;
    }
    
    lastMoveTime = currentTime;
  }
}

// DETECTAR COLISIONES CON CACTUS DOBLES
bool checkCollision() {
  if (obstaclePos == 2) {
    if (obstacleType == 2) {
      return (!isDucking && !isJumping);
    } else {
      return (!isJumping);
    }
  }
  
  if (doubleCactus && secondCactusPos == 2) {
    return (!isJumping);
  }
  
  return false;
}

// Pantalla de inicio
void showStartScreen() {
  lcd.clear();
  
  lcd.setCursor(3, 0);
  lcd.print("DINO RUN");
  lcd.setCursor(1, 1);
  lcd.print("PRESS START");
  
  static unsigned long lastAnimTime = 0;
  if (millis() - lastAnimTime > 500) {
    lcd.setCursor(7, 0);
    lcd.write(dinoFrame ? 0 : 1);
    dinoFrame = !dinoFrame;
    lastAnimTime = millis();
  }
}

// Pantalla de Game Over
void showGameOver() {
  lcd.clear();
  
  lcd.setCursor(4, 0);
  lcd.print("GAME OVER");
  lcd.setCursor(3, 1);
  lcd.print("Score: ");
  lcd.print(score);
  
  tone(buzzerPin, 200, 300);
  delay(400);
  tone(buzzerPin, 150, 500);
  delay(2000);
}

// REINICIAR JUEGO
void resetGame() {
  gameOver = false;
  score = 0;
  dinoPos = 1;
  isJumping = false;
  isDucking = false;
  jumpFrame = 0;
  obstaclePos = 25;
  obstacleType = 0;
  gameSpeed = 8;
  obstacleGap = 0;
  doubleCactus = false;
  secondCactusPos = -5;
  transitionPhase = 0;
  starsActive = false;
  starTimer = 0;
  isNight = false;
  
  lastMoveTime = millis();
  lastUpdate = millis();
}

// CONFIGURACIÓN
void setup() {
  pinMode(BTN_JUMP, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_DUCK, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  
  lcd.begin(16, 2);
  
  delay(100);
  lcd.clear();
  
  setupGraphics();
  
  randomSeed(analogRead(0));
}

// LOOP PRINCIPAL
void loop() {
  if(!gameStarted) {
    showStartScreen();
    
    if(isButtonPressed(BTN_START)) {
      playStartMelody();
      gameStarted = true;
      resetGame();
      delay(300);
    }
    
  } else if(!gameOver) {
    updateSunMoonTransition();
    updateStars();
    updateAnimation();
    handleJump();
    handleDuck();
    moveObstacles();
    generateObstacle();
    
    if(checkCollision()) {
      gameOver = true;
      showGameOver();
      gameStarted = false;
    } else {
      drawGame();
    }
    
  } else {
    if(isButtonPressed(BTN_START)) {
      playStartMelody();
      gameStarted = true;
      resetGame();
      delay(300);
    }
  }
  
  delay(50);
}