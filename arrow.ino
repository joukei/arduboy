
/*
  Simple Animation example of a sprite
  Written By Joeri http://www.JO3RI.be , 12 June 2015
*/

#include <Arduino.h>
#include <Arduboy2.h>
#include <ArduboyTones.h>

#include "bitmap.h"

struct Arrows
{
  public:
    byte x;
    byte y;
    bool fMove;
    bool fHit;
    bool fSuspend;
};

struct Targets
{
  public:
    int type;
    int x;
    int y;
    int w;
    int h;
    int point;
};

// type
#define TARGET_NORMAL 0
#define TARGET_APPLE  1
#define TARGET_MEN    2
#define TARGET_PEN    3
#define TARGET_PANDA  4
#define TARGET_DEAD   5
#define TARGET_UFO    6

struct Players
{
  public:
    int score;
    int ArrowCount;
    int level;
    int clearScore;
    int totalScore;
    bool fGameOver;
    bool fClearStage;
    bool fSound;
};

Arrows Arrow;
Targets Target;
Players Player;

Arduboy2 arduboy;
Sprites sprites;
ArduboyTones sound(arduboy.audio.enabled);

#define MODE_TITLE   0
#define MODE_PLAY    1
#define MODE_NEXT_LV 2

byte mode = MODE_TITLE;
byte suspendFrame = 0;

void setup() {
  arduboy.boot();
  arduboy.audio.begin();
  arduboy.bootLogoSpritesSelfMasked();
  arduboy.setFrameRate(60);
  arduboy.initRandomSeed();

  Arrow.x = 1;
  Arrow.y = 20;
  Arrow.fMove = false;
  Arrow.fHit = false;
  Arrow.fSuspend = false;

  Target.type = TARGET_NORMAL;
  Target.x = 128 - 32 - 1;
  Target.y = 64;
  Target.w = 32;
  Target.h = 32;
  Target.point = 10;

  Player.fSound = false;
}

void loop() {
  if (!(arduboy.nextFrame())) return;
  arduboy.pollButtons();
  arduboy.clear();

  if ( mode == MODE_TITLE ) {
    Player.level = 1;
    Player.clearScore = 10;
    Player.totalScore = 0;
    sprites.drawSelfMasked(  10, 3, titleSprite, 0);
    sprites.drawSelfMasked(  128 - 32 - 3 , 64 - 32 - 20, titleIconSprite, 0);
    arduboy.setCursor(10, 47);
    arduboy.print("Press B Start");
    if (arduboy.justPressed(B_BUTTON))  mode = MODE_NEXT_LV;

    // sound
    if (arduboy.justPressed(UP_BUTTON) || arduboy.justPressed(DOWN_BUTTON)) {
      Player.fSound = !Player.fSound;
    }
    arduboy.setCursor(25, 56);
    if ( Player.fSound == false ) {
      arduboy.print("Sound:Off");
      arduboy.audio.off();
    } else {
      arduboy.print("Sound:On");
      arduboy.audio.on();
    }

  } else if ( mode == MODE_NEXT_LV) {
    char Buf[6] = { 0x30, 0x30, 0x30, 0x30, 0x30, '\0' };
    ltoa( Player.clearScore, Buf, 10);
    arduboy.setCursor(20, 50);
    arduboy.print("Clear Score:");
    arduboy.setCursor(95, 50);
    arduboy.print(Buf);

    Target.type = TARGET_NORMAL;
    Player.score = 0;
    Player.ArrowCount = 5;
    Player.fGameOver = false;
    Player.fClearStage = false;

    sprites.drawSelfMasked( 25, 5, StageSprite, 0);
    sprites.drawSelfMasked( 55, 27, numbersSprite, Player.level);

    if (arduboy.justPressed(B_BUTTON))  mode = MODE_PLAY;
  } else if ( mode == MODE_PLAY ) {
    drawScore();
    judgeNextLevel();
    drawTarget();
    drawArrow();
    judgeHit();
  }

  arduboy.display();
}

void drawTarget()
{
  if ( Player.fGameOver == true ) {
    Target.type = TARGET_DEAD;
  } else if ( Player.fClearStage == true ) {
    Target.type = TARGET_PANDA;
  }

  sprites.drawSelfMasked(  Target.x, Target.y, targetSprite, Target.type);

  if ( Arrow.fHit == true && suspendFrame < 100 ) {
    Arrow.fSuspend = true;
    suspendFrame++;
    showPoint(Target.point);
  } else {
    Arrow.fSuspend = false;
    Target.y -= 1;
    if ( Target.y <= -60 ) {
      Target.y = 64;
      suspendFrame = 0;
      Target.type = getTargetType(Player.level);
    }
  }
}

void drawArrow()
{

  if ( Arrow.fMove == false  && Arrow.fHit == false && Player.ArrowCount > 0) {
    // 矢の発射
    if (arduboy.justPressed(B_BUTTON)) {
      Arrow.fMove = true;
      Target.point = 0;
      sound.tone(NOTE_A4, 50);
    }
  }

  if ( Arrow.fMove == false  && Arrow.fHit == false && Player.ArrowCount > 0) {
    if (arduboy.pressed(UP_BUTTON)) Arrow.y--;
    if (arduboy.pressed(DOWN_BUTTON)) Arrow.y++;
    if ( Arrow.y <= 1 ) Arrow.y = 1;
    if ( Arrow.y >= 45 ) Arrow.y = 45;
  }

  if ( Arrow.y <= 0 && Arrow.fHit == true) {
    // 矢が的に当たって上端に到着 -> 初期位置へ戻す
    Arrow.x = 2;
    Arrow.y = 20;
    Arrow.fHit = false;
  }

  if ( Arrow.x > 128 && Arrow.fMove == true) {
    // 矢が的に当たらなかった -> 初期位置へ戻す
    Arrow.x = 2;
    Player.ArrowCount--;
    Arrow.fMove = false;
  }

  if (Arrow.fHit == true && Arrow.fSuspend == false) {
    // 矢が的に当たった -> 的に刺さる
    Arrow.y--;
  }

  if ( Arrow.fMove == true ) {
    Arrow.x += 2; // speed
  }

  if (  Player.ArrowCount > 0 || Arrow.fHit == true) {
    sprites.drawSelfMasked(Arrow.x, Arrow.y, arrowSprite, 0);
  }
}

const uint16_t penSound[] PROGMEM = {
  NOTE_C1, 100, NOTE_REST, 10,
  NOTE_C1, 300, NOTE_REST, 10,
  TONES_END
};

const uint16_t centorSound[] PROGMEM = {
  NOTE_B6, 100, NOTE_REST, 10, NOTE_G7, 100, NOTE_REST, 10,
  NOTE_B6, 100, NOTE_REST, 10, NOTE_G7, 100, NOTE_REST, 10,
  NOTE_B6, 100, NOTE_REST, 10, NOTE_G7, 100, NOTE_REST, 10,
  TONES_END
};

const uint16_t hitSound[] PROGMEM = {
  NOTE_B6, 100, NOTE_REST, 10, NOTE_G7, 100, NOTE_REST, 10,
  TONES_END
};

void judgeHit()
{
  Rect TargetRect = {.x = Target.x + 15, .y = Target.y, .width = 2, .height = 32};
  Rect ArrowRect = {.x = Arrow.x + 15, .y = Arrow.y + 2, .width = 1, .height = 1};

  if (Target.type == TARGET_UFO) {
    TargetRect.y += 8;
    TargetRect.height = 14;
  }

  if (arduboy.collide(TargetRect, ArrowRect) && Arrow.fHit == false) {
    int pos = 0;
    //矢が的に刺さった
    Arrow.fMove = false;
    Arrow.fHit = true;
    Player.ArrowCount--;
    pos = abs(32 / 2 - 2 - (ArrowRect.y - TargetRect.y));
    Target.point = getPoint(Target.type, pos);
    Player.score += Target.point;
    Player.totalScore += Target.point;

    if ( Target.type == TARGET_PEN ) {
      sound.tones(penSound);
    } else if ( pos == 0 ) {
      sound.tones(centorSound);
    } else {
      sound.tones(hitSound);
    }
  }
}

void drawScore()
{
  char Buf[6] = { 0x30, 0x30, 0x30, 0x30, 0x30, '\0' };

  //level
  ltoa( Player.level, Buf, 10);
  arduboy.setCursor(1, 56);
  arduboy.print(F("LV:"));
  arduboy.setCursor(18, 56);
  arduboy.print(Buf);

  // reamain arrow
  sprites.drawSelfMasked(30, 56, remainArrowSprite, 0);
  arduboy.setCursor(46, 56);
  arduboy.print(F(":"));
  arduboy.setCursor(52, 56);
  ltoa( Player.ArrowCount, Buf, 10);
  arduboy.print(Buf);

  //score
  ltoa( Player.score, Buf, 10);
  arduboy.setCursor(65, 56);
  arduboy.print(F("SCORE:"));
  arduboy.setCursor(65 + 36, 56);
  arduboy.print(Buf);
}

void judgeNextLevel()
{
  if ( Player.ArrowCount <= 0 && Arrow.fHit == false ) {
    if ( Player.score >= Player.clearScore ) {
      Player.fClearStage = true;
      arduboy.setCursor(10, 20);
      arduboy.print("Stage Clear");
      arduboy.setCursor(10, 30);
      arduboy.print("Press B Button.");
      if (arduboy.justPressed(B_BUTTON)) {
        Player.level++;
        Player.clearScore += 10;
        Player.score = 0;
        mode = MODE_NEXT_LV;
      }
    } else {
      Player.fGameOver = true;
      arduboy.setCursor(2, 10);
      arduboy.print("Game Over");
      {
        char Buf[6] = { 0x30, 0x30, 0x30, 0x30, 0x30, '\0' };

        ltoa( Player.totalScore, Buf, 10);
        arduboy.setCursor(2, 25);
        arduboy.print(F("TOTAL SCORE:"));
        arduboy.setCursor(75, 25);
        arduboy.print(Buf);
      }
      arduboy.setCursor(2, 40);
      arduboy.print("Press B Button.");
      
      if (arduboy.justPressed(B_BUTTON)) {
        mode = MODE_TITLE;
      }
    }
  }
}

// pos：中心からの距離
int getPoint( int type, int pos )
{
  int point;
  switch ( type ) {
    case TARGET_NORMAL:
      point = 10 - (pos / 2);
      break;
    case TARGET_APPLE:
      point = 10 - (pos / 2);
      break;
    case TARGET_MEN:
      point = 12 - (pos / 2);
      break;
    case TARGET_UFO:
      point = 16 - (pos / 2);
      break;
    case TARGET_PEN:
      point = -20 + (pos / 2);
      break;
    default:
      break;
  }
  return point;
}

void showPoint(int point)
{
  if ( point < 0 ) {
    sprites.drawSelfMasked(  30, 20, numbersSprite, 11);
  } else {
    sprites.drawSelfMasked(  30, 20, numbersSprite, 10);
  }

  if (abs(point) < 10 ) {
    sprites.drawSelfMasked(  46, 20, numbersSprite, abs(point));
  } else {
    sprites.drawSelfMasked(  46, 20, numbersSprite, abs(point) / 10);
    sprites.drawSelfMasked(  46 + 16, 20, numbersSprite, abs(point) % 10);
  }
}


int getTargetType( int level )
{
  int type = TARGET_NORMAL;
  int num;
  num = rand();
  if ( level == 1 ) {
    type = TARGET_NORMAL;
  } else if ( level == 2 ) {
    if ( num % 10 == 0 ) type = TARGET_UFO;
    if ( num % 10 == 1 ) type = TARGET_PEN;
  } else if ( level == 3 ) {
    if ( num % 5 == 0 ) type = TARGET_UFO;
    if ( num % 5 == 1 ) type = TARGET_PEN;
  } else {
    if ( num % 3 == 0 ) type = TARGET_PEN;
    if ( num % 3 == 1 ) type = TARGET_UFO;
  }
  return type;
}

