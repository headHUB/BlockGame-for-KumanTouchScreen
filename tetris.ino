#include <SPI.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>

//--------------------------------------------------------------------------------
MCUFRIEND_kbv tft;

#define YP A2
#define XM A3
#define YM 8
#define XP 9
#define MINPRESSURE 10
#define TS_MINX 130
#define TS_MAXX 905
#define TS_MINY 75
#define TS_MAXY 930
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define BLACK   0x0000
#define WHITE   0xFFFF
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define YELLOWGREEN 0xCFE0
#define ORANGE  0xFDE0

#define XSIZE 10
#define YSIZE 20
#define YMARGIN_SIZE 5
#define BLOCK_PIXEL 15
#define XSTART 90
#define YSTART 20

//--------------------------------------------------------------------------------

int board[XSIZE][YSIZE + YMARGIN_SIZE];

struct Position
{
  int x;
  int y;
};

struct Mino
{
  int color;
  int rotate_pattern_num;
  Position blocks[4];
};

const Mino minos[7] = {
  {CYAN       , 2,  { {0, 0}, {0, -1}, { +0, +1}, { +0, +2} } }, //I
  {YELLOW     , 1,  { {0, 0}, {0, +1}, { +1, +0}, { +1, +1} } }, //O
  {YELLOWGREEN, 2,  { {0, 0}, {0, -1}, { +1, +0}, { +1, +1} } }, //S
  {RED        , 2,  { {0, 0}, {0, -1}, { -1, +0}, { -1, +1} } }, //Z
  {BLUE       , 4,  { {0, 0}, {0, -1}, { +0, +1}, { -1, +1} } }, //J
  {ORANGE     , 4,  { {0, 0}, {0, -1}, { +0, +1}, { +1, +1} } }, //L
  {MAGENTA    , 4,  { {0, 0}, {0, -1}, { +1, +0}, { +0, +1} } }, //T
};

struct CurrentStatus {
  int mino_type;
  int x;
  int y;
  int rotate;
};
CurrentStatus current, next_mino;

enum GameState {
  NEW,
  FALL,
  DELETE,
  NEXT,
  GAMEOVER,
};

int key = 0;
int state = NEW;
long score = 0;
long high_score = 0;

//--------------------------------------------------------------------------------
// Utility

void calc_blockpositions(int type, int rotate, Position out[4])
{
  rotate = rotate % minos[type].rotate_pattern_num;
  for (int i = 0; i < 4; ++i) {
    int x = minos[type].blocks[i].x;
    int y = minos[type].blocks[i].y;
    switch (rotate) {
      case 0:
        out[i].x = x;
        out[i].y = y;
        break;
      case 1:
        out[i].x = -y;
        out[i].y = x;
        break;
      case 2:
        out[i].x = -x;
        out[i].y = -y;
        break;
      case 3:
        out[i].x = y;
        out[i].y = -x;
        break;
    }

  }
}

bool check_boundary()
{
  if (current.x < 0 || current.x >= XSIZE || current.y < 0) return false;

  Position block[4];
  calc_blockpositions(current.mino_type, current.rotate, block);
  for (int i = 0; i < 4; ++i) {
    if (current.x + block[i].x < 0 || current.x + block[i].x >= XSIZE || current.y + block[i].y < 0) {
      return false;
    }
    if (board[current.x + block[i].x][current.y + block[i].y]) return false;
  }
  return true;
}

CurrentStatus generate_mino()
{
  CurrentStatus mino;
  mino.x = 4;
  mino.y = 20;
  mino.mino_type = random(7);
  mino.rotate = 0;
  return mino;
}

Position conv_board2display(int x, int y)
{
  Position p = { 0, 0 };
  p.x = XSTART + (XSIZE - x - 1) * BLOCK_PIXEL;
  p.y = YSTART + (YSIZE - y - 1) * BLOCK_PIXEL;
  return p;
}

//--------------------------------------------------------------------------------
// Display

void clear_current_mino()
{
  Position block[4];
  calc_blockpositions(current.mino_type, current.rotate, block);
  for (int i = 0; i < 4; ++i) {
    Position p = conv_board2display(current.x + block[i].x, current.y + block[i].y);
    tft.fillRect(p.x, p.y, BLOCK_PIXEL, BLOCK_PIXEL, BLACK);
  }
}

void clear_next()
{
  tft.fillRect(2, 32, 78, 78, BLACK);
}

void clear_screen()
{
  tft.fillScreen(BLACK);
}

void show_next()
{    
  tft.setTextSize(2);
  tft.setCursor(0, 10);
  tft.setTextColor(YELLOW, BLACK);
  tft.print("Next");

  tft.drawRect(0, 30, 80, 80, WHITE);
  const int color = minos[next_mino.mino_type].color;
  Position block[4];
  calc_blockpositions(next_mino.mino_type, 0, block);
  for (int i = 0; i < 4; ++i) {
    tft.fillRect(30 + block[i].x*BLOCK_PIXEL, 55 + block[i].y*BLOCK_PIXEL, BLOCK_PIXEL, BLOCK_PIXEL, color);
  }
}

void show_score()
{
  tft.setTextSize(2);
  tft.setCursor(0, 120);
  tft.setTextColor(YELLOW, BLACK);
  tft.println("Score");
  tft.println(score);
  tft.println("HiScore");
  tft.println(high_score);
}

void draw()
{
  show_score();
  show_next();
  
  // Draw board
  for (int y = 0; y < YSIZE; ++y) {
    for (int x = 0; x < XSIZE; ++x) {
      Position p = conv_board2display(x, y);
      if (board[x][y]) {
        tft.fillRect(p.x, p.y, BLOCK_PIXEL, BLOCK_PIXEL, board[x][y]);
      } else {
        tft.drawRect(p.x, p.y, BLOCK_PIXEL, BLOCK_PIXEL, WHITE);
      }
    }
  }
  // Draw current mino
  const int color = minos[current.mino_type].color;
  Position block[4];
  calc_blockpositions(current.mino_type, current.rotate, block);
  for (int i = 0; i < 4; ++i) { 
    Position p = conv_board2display(current.x + block[i].x, current.y + block[i].y);
    tft.fillRect(p.x, p.y, BLOCK_PIXEL, BLOCK_PIXEL, color);
  }
}

//--------------------------------------------------------------------------------
// Game Control

void move_mino()
{
  switch (key) {
    case 'h':
      current.x += 1;
      if (!check_boundary()) current.x -= 1;
      break;
    case 'l':
      current.x -= 1;
      if (!check_boundary()) current.x += 1;
      break;
    case 'j':
      current.rotate += 1;
      if (!check_boundary()) current.rotate -= 1;
      break;
    case 'k':
      for (int k = 0; k < YSIZE; ++k) {
        current.y -= 1;
        if (!check_boundary()) {
          current.y += 1;
          break;
        }
      }
      break;
  }
  key = 0;
}

void drop_mino()
{
  current.y -= 1;
  Position block[4];
  calc_blockpositions(current.mino_type, current.rotate, block);
  for (int i = 0; i < 4; ++i) {
    if (board[current.x + block[i].x][current.y + block[i].y] || current.y + block[i].y < 0) {
      current.y += 1;
      state = DELETE;
      return;
    }
  }
}

void fix_mino()
{
  const int color = minos[current.mino_type].color;
  Position block[4];
  calc_blockpositions(current.mino_type, current.rotate, block);
  for (int i = 0; i < 4; ++i) {
    board[current.x + block[i].x][current.y + block[i].y] = color;
  }
}

void delete_mino()
{
  for (int y = 0; y < YSIZE; ++y) {
    bool clear_flag = true;
    for (int x = 0; x < XSIZE; ++x) {
      if (!board[x][y]) clear_flag = false;
    }
    if (clear_flag) {
      for (int tmp_y = y; tmp_y < YSIZE; ++tmp_y) {
        for (int x = 0; x < XSIZE; ++x) {
          board[x][tmp_y] = board[x][tmp_y + 1];
        }
      }
      y--;
      clear_screen();
      score += 1000;
      if (score > high_score) high_score = score;
    }
  }
}

void judge()
{
  for (int x = 0; x < XSIZE; ++x) {
    if (board[x][YSIZE - 1]) {
      state = GAMEOVER;
      return;
    }
  }
  state = NEXT;
}

//--------------------------------------------------------------------------------
// H/W input and state change

void setup() {
  randomSeed(analogRead(0));
  Serial.begin(9600);
  tft.reset();
  uint16_t identifier = tft.readID();
  tft.begin(identifier);

  key = 0;
  state = NEW;
  score = 0;
  high_score = 0;
}

void update_state()
{
  switch (state)
  {
    case NEW:
      tft.fillScreen(BLACK);
      memset(board, 0, sizeof(int)*XSIZE * (YSIZE + YMARGIN_SIZE));
      state = NEXT;
      next_mino = generate_mino();
      break;
    case FALL:
      clear_current_mino();
      move_mino();
      drop_mino();
      draw();
      break;
    case DELETE:
      fix_mino();
      delete_mino();
      judge();
      break;
    case NEXT:
      current = next_mino;
      next_mino = generate_mino();
      clear_next();
      state = FALL;
      break;
    case GAMEOVER:
      tft.fillScreen(BLACK);
      tft.fillScreen(RED);
      tft.fillScreen(BLACK);
      tft.fillScreen(RED);
      score = 0;
      state = NEW;
      break;
    default:
      // Error
      tft.fillScreen(BLACK);
      tft.fillScreen(YELLOW);
      tft.fillScreen(BLACK);
      tft.fillScreen(YELLOW);
      break;
  }
}

void key_input()
{
  static int prev = 0;
  if (state != FALL) {
    key = 0;
    return;
  }
  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  if (prev > ts.pressureThreshhold && p.z > ts.pressureThreshhold) {
    // need calibration
    if (p.x < 250) {
      key = 'k';
    } else if (p.x > 750) {
      key = 'j';
    } else if (p.y < 400) {
      key = 'h';
    } else {
      key = 'l';
    }
  }
  prev = p.z;
}

void loop() {
  for (int i = 0; i < 10; ++i) {
    key_input();
    delay(16);
  }
  update_state();
}

