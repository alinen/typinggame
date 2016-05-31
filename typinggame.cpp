// build: g++ typinggame.cpp -o game -lncurses
// run: game < input.txt
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <deque>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <curses.h>
#include <windows.h>
#include <time.h>
#include <assert.h>

using namespace std;

class Vec2
{
public:
   union {
      struct {
         int x;
         int y;
      };
      int n[2];
   };

   Vec2() {}
   Vec2(int i, int j) { n[0] = i; n[1] = j; }
   Vec2(const Vec2& v) : x(v.x), y(v.y) {}
   Vec2& operator=(const Vec2& v) { if (&v == this) return *this; x = v.x; y = v.y; }
   int operator[] (int i) const { return n[i]; }
   int operator[] (int i) { return n[i]; }
   friend Vec2 operator* (const Vec2& v, float a) { return Vec2((int) v.x*a, (int) v.y*a); }
   friend Vec2 operator* (float a, const Vec2& v) { return Vec2((int) v.x*a, (int) v.y*a); }
   friend Vec2 operator+ (const Vec2& a, const Vec2& b) { return Vec2(a.x+b.x, a.y+b.y); }
   friend Vec2 operator- (const Vec2& a, const Vec2& b) { return Vec2(a.x-b.x, a.y-b.y); }
   friend ostream& operator<< (ostream& os, const Vec2& v) { os << "(" << v.x << ", " << v.y << ")"; }
};

// findword is like strtok but returns the number of characters between tokens
char* findword(char* buff, char delim, int& num_spaces)
{
   static int idx = 0;
   static int len = 0;
   static char* ptr = 0;
   if (buff)
   {
      ptr = buff;
      idx = 0;
      len = strlen(buff);
   }

   if (idx >= len)
   {
      return NULL;
   }

   num_spaces = 1;
   while (idx < len && ptr[idx] == delim) // leading delim chars
   {
      num_spaces++;
      idx++;
   }
   int start = idx;
   while (idx < len && ptr[idx] != delim) {idx++;} // word itself
   if (idx < len) ptr[idx] = '\0';
   idx++;
   return &(ptr[start]);
}

//------------------------
//   graphics
//------------------------
const int gNumTrail = 4;
const char* gTrail = ".*oO";

const int gNumExplosionFade = 4;
const Vec2 gDimExplosion(3,5);
const char* gExplosion1[] = {
" % % ",
"% % %",
" % % "};
const char* gExplosion2[] = {
" * * ",
"* * *",
" * * "};
const char* gExplosion3[] = {
" . . ",
". . .",
" . . "};
const char* gExplosion4[] = {
"     ",
"     ",
"     "};

const Vec2 gDimBeeSprite(4,8); 
const char* gBeeSprite[] = {
"   __   ",
"  (__\\_ ",
"-{{_{|8)",
"  (__/  "};

const Vec2 gDimSky(11,52);
const char* gSkySprite[] = {
"           __----_                                  ",
"          (`......)                    _            ",
"         (. _^ ^_ .)                :(`..)`         ",
"        _(...)u( ..'`           :(........)         ",
"     =(`(..... ......)      --  `.  (. ..).)        ",
"   ((....(..__.:'-'       +(...)   ` _`..).)        ",
"   `(.......).)         (.. ...)     (...)          ",
"     ` __.:. ..)       (...(...)      `-'.-(`....)  ",
"            --'         `-.__.'         :(.. ^ ^ .))",
"                                       `(....)V .)) ",
"                                         ...        "};      

const int gFlair[15] = {0,-1,-1,-1,-1,-1,-1,0,1,1,1,1,1,0,0};
const int gNumFlair = 15; 

//---------------------------------
// game
//---------------------------------
class TypingGame
{
public:
   TypingGame()
   {
      init();
   }

   ~TypingGame()
   {
      endwin();
   }

   void init()
   {
      initscr();
      if(has_colors() == FALSE) 
      {
         endwin();
         throw runtime_error("typinggame needs colors");
      }
   
      noecho(); // don't show keypresses in console
      keypad(stdscr, TRUE); // enable special keys
      nodelay(stdscr, TRUE); // don't block on getch
      cbreak(); // all input sent immediately to getch()
      curs_set(2);
      bkgdset(COLOR_BLACK); // set background color to black

      start_color();
	   init_pair(WS_INPROGRESS, COLOR_MAGENTA, COLOR_BLACK);
	   init_pair(WS_ERROR, COLOR_RED, COLOR_BLACK);
      init_pair(RB_1, COLOR_RED, COLOR_BLACK);
      init_pair(RB_2, COLOR_MAGENTA, COLOR_BLACK);
      init_pair(RB_3, COLOR_YELLOW, COLOR_BLACK);
      init_pair(RB_4, COLOR_GREEN, COLOR_BLACK);
      init_pair(RB_5, COLOR_BLUE, COLOR_BLACK);
      init_pair(RB_6, COLOR_CYAN, COLOR_BLACK);

      mScore = 0;
      mElapsedTime = 0;
      // NOTE: Need to init text BEFORE loading text!!
      mTxt.init(this, Vec2(-3,0), Vec2(LINES, ((int) COLS*0.5) - 19)); // hard-coded for injust.txt
      mBee.init(Vec2(0,10), Vec2(gDimSky.x + SCREEN_START + 2, -gDimBeeSprite.y), RB_3);
      mBeeSpawn = 0;
      mExplosions.init();
      move(0, 0); addstr("Press ESC to exit.\n");
      drawSky(gDimSky, gSkySprite);      


   }

   bool loadFile(const string& filename)
   {
      ifstream textfile(filename.c_str());
      if (textfile.is_open())
      {
         float time = 0;
         string line;
         while (getline(textfile, line))
         {
            addLine(line, time);
            time += MIN_TIME_OFFSET + rand() % VAR_TIME_OFFSET; // next text appears between 3 and 8 seconds later
         }
         textfile.close();
         return true;
      }
      return false;
   }

   void addLine(const string& line, float spawnTime)
   {
      mTxt.addLine(line, spawnTime);
   }

   void drawSky(const Vec2& dim, const char* sprite[])
   {
      int ypos = 0;
      while (ypos < COLS)
      {
         for (int i = 0; i < dim.x; i++)
         {
            const char* line = sprite[i];
            for (int j = 0; j < dim.y && j < COLS; j++)
            {
               //if (line[j] == '.') attron(COLOR_PAIR(8));
               mvaddch(i+SCREEN_START, ypos+j, line[j]);
               //if (line[j] == '.') attroff(COLOR_PAIR(8));
            }
         }
         ypos += dim.y;
      }
   }

   void createExplosion(const Vec2& p, int c)
   {
      mExplosions.create(p, c);
   }

   void updateAndDraw(float dt, char c)
   {
      if (mTxt.finished())
      {
         move(LINES*0.5,COLS*0.5); printw("Finished! Score: %d", mScore);
         return;
      }

      mvprintw(1,0, "Score: %10d", mScore);
      mElapsedTime += dt;

      drawSky(gDimSky, gSkySprite);  
      mTxt.update(dt, mElapsedTime);
      mTxt.processUserInput(c);

      mBee.updateAndDraw(dt);
      mExplosions.updateAndDraw(dt);
      mTxt.draw();

      /*
      if (mBee.finished() && mBee.inMotion())
      {
         mBee.stop();
         mBeeSpawn = mElapsedTime + MIN_TIME_OFFSET + rand() % VAR_TIME_OFFSET;
      }

      if (mElapsedTime > mBeeSpawn && !mBee.inMotion())
      {
         mBee.start();
      }*/

      wrefresh(stdscr);
   }

private:


   enum RainbowColors { RB_1 = 3, RB_2, RB_3, RB_4, RB_5, RB_6 };
   int mScore;
   float mElapsedTime;
   float mBeeSpawn;
   static const int SCREEN_START = 3;
   static const int VAR_TIME_OFFSET = 5;
   static const float MIN_TIME_OFFSET = 2.0;   

   //----------------------------------------------
   // Scrolling text logic
   //----------------------------------------------
   enum WordState {WS_INIT, WS_INPROGRESS, WS_ERROR, WS_COMPLETE, WS_FAIL, WS_HIDDEN};
   class ScrollText
   {
   public:
      void init(TypingGame* g, const Vec2& _vel, const Vec2& _startpos)
      {
         mGame = g;
         mYcursorOffset = 0;
         mCurrent = 0;
         mDt = 0;
         mVel = _vel;
         mStartpos = _startpos; 
      }

      void addLine(const string& line, float spawnTime)
      {
         char buffer[2056];
         strncpy(buffer, line.c_str(), 2056);

         int x = mStartpos.x;
         int y = mStartpos.y + rand() % 10 - 5;
         int num_spaces = 0;
         char* token = findword(buffer, ' ', num_spaces);
         while (token)
         {
            string word = token;

            mWords.push_back(word);
            mDim.push_back(Vec2(1,word.size()));
            mPos.push_back(Vec2(x, y));
            mSpawn.push_back(spawnTime);
            mState.push_back(WS_HIDDEN);

            y += word.size()+num_spaces;
            token = findword(NULL, ' ', num_spaces);
         }
      }

      bool finished()
      {
         return (mCurrent >= mWords.size()); // all done!
      }

      int numWords() const
      {
         return mWords.size(); 
      }

      void eraseWord(int word_id)
      {
         // erase old word position and update pos
         for (int i = 0; i < mWords[word_id].size(); i++)
         {
            mvaddch(mPos[word_id].x, mPos[word_id].y+i, ' '); 
         }      
      }

      void drawInProgress(int word_id)
      {
         attron(COLOR_PAIR(WS_INPROGRESS));
         attron(A_BOLD);
         for (int i = 0; i <= mYcursorOffset && i < mWords[word_id].size(); i++) 
         {
            mvaddch(mPos[word_id].x, mPos[word_id].y+i, mWords[word_id][i]); 
         }
         attroff(A_BOLD);
         attroff(COLOR_PAIR(WS_INPROGRESS));	
         for (int i = mYcursorOffset+1; i < mWords[word_id].size(); i++) 
         {
            mvaddch(mPos[word_id].x, mPos[word_id].y+i, mWords[word_id][i]); 
         }            
      }
   
      void drawError(int word_id)
      {
         attron(COLOR_PAIR(WS_ERROR));	
         attron(A_BOLD);
         move(mPos[word_id].x, mPos[word_id].y); 
         addstr(mWords[word_id].c_str());
         attroff(A_BOLD);
         attroff(COLOR_PAIR(WS_ERROR));         
      }

      void drawNormal(int word_id)
      {
         move(mPos[word_id].x, mPos[word_id].y); 
         addstr(mWords[word_id].c_str());      
      }
      
      void drawFail(int word_id)
      {
         attron(COLOR_PAIR(WS_ERROR));	
         for (int i = 0; i < mWords[word_id].size(); i++)
         {
            mvaddch(mPos[word_id].x, mPos[word_id].y+i, mWords[word_id][i]);
         }
         attroff(COLOR_PAIR(WS_ERROR));
      }

      bool intersection(int word_id)
      {
         for (int i = 0; i < mWords[word_id].size(); i++)
         {
            int x = mPos[word_id].x - TypingGame::SCREEN_START;
            int y = mPos[word_id].y+i;
            int y_tile = y % gDimSky.y;
            if (x >= 0 && x < gDimSky.x && gSkySprite[x][y_tile] != ' ')
            {
               return true;
            }
         }
         return false;
      }
         

      void update(float _dt, float elapsedTime)
      {
         mDt += _dt;
         Vec2 numUnits = mVel * mDt;
         if (fabs(numUnits.x) == 0 && fabs(numUnits.y) == 0) return;

         mDt = 0; 
         float advance = 0;
         if (mSpawn[mCurrent] > elapsedTime && mState[mCurrent] == WS_HIDDEN) // waiting, start it early
         {
             mState[mCurrent] = WS_INIT;
             advance = mSpawn[mCurrent] - elapsedTime;
         }         

         int maxRow = -1;
         for (int k = mCurrent; k < mWords.size(); k++)
         {
            if (k != mCurrent)
            {
               mSpawn[k] -= advance;
            }

            if (mState[k] == WS_HIDDEN && elapsedTime > mSpawn[k])
            {
               mState[k] = WS_INIT;
            }

            if (mState[k] != WS_COMPLETE && mState[k] != WS_FAIL && mState[k] != WS_HIDDEN)
            {
               // update position
               Vec2 newpos = mPos[k] + numUnits; 

               // special case: don't move ahead of the bee
               // starting with mCurrent, have all text pile behind the bee row
               // 
               if (k == mCurrent && mGame->mBee.inMotion() && newpos.x <= mGame->mBee.trajectoryHeight())
               {
                  maxRow = mPos[k].x;
                  newpos.x = mPos[k].x;
               }
               else if (k != mCurrent && maxRow > 0 && newpos.x <= maxRow)
               {
                  maxRow = mPos[k].x;
                  newpos.x = mPos[k].x;
               }

               eraseWord(k);
               mPos[k] = newpos;
               if (mPos[k].x < TypingGame::SCREEN_START || mPos[k].x > LINES-1 || intersection(k)) 
               {
                  int x = mPos[k].x+mDim[k].x*0.5;
                  int y = mPos[k].y;
                  mGame->createExplosion(Vec2(x,y),WS_ERROR);
                  mState[k] = WS_FAIL; 
               }       
            }
         }
      }

      void processUserInput(char c)
      {
         if (mWords[mCurrent][mYcursorOffset] == c) // correct 
         {
            mState[mCurrent] = WS_INPROGRESS;
            mYcursorOffset++;
         }
         else if (c != -1 && c != ' ') // mistake
         {
            mState[mCurrent] = WS_ERROR;
            mYcursorOffset = 0; //restart word
         }

         if (mYcursorOffset >= mWords[mCurrent].size()) //complete
         {
            if (mState[mCurrent] == WS_INPROGRESS) // success!
            {
               int multiplier = 1; 
               if (mPos[mCurrent].x > LINES*0.75) multiplier = 10;
               else if (mPos[mCurrent].x > LINES*0.5) multiplier = 5;
               else if (mPos[mCurrent].x > LINES*0.25) multiplier = 2;
               mGame->mScore += mDim[mCurrent].y * multiplier;
               mGame->createExplosion(mPos[mCurrent]+mDim[mCurrent]*0.5, WS_INPROGRESS);
            }
            mState[mCurrent] = WS_COMPLETE;
            eraseWord(mCurrent);
            mYcursorOffset = 0;
            mCurrent++;
         }         
      }

      void draw()
      {
         // update based on user update
         for (int k = mCurrent; k < mWords.size(); k++)
         {
            assert (mState[k] != WS_COMPLETE); // completed word, don't draw, shouldn't need this

            if (mState[k] == WS_HIDDEN)
            {
               continue; // don't draw it yet
            }
            else if (mState[k] == WS_INIT)
            {
               drawNormal(k);
            }
            else if (mState[k] == WS_ERROR)
            {
               drawError(k);  
            }
            else if (mState[k] == WS_INPROGRESS)
            {
               drawInProgress(k);
            }         
            else if (mState[k] == WS_FAIL)
            {
               drawFail(k);
               if (k == mCurrent) // increment cursor
               {
                  mYcursorOffset = mWords[mCurrent].size();
               }
            }
         }
         move(mPos[mCurrent].x, mPos[mCurrent].y + mYcursorOffset); 
      }

   private:
      TypingGame* mGame; // owner
      vector<Vec2> mPos;
      vector<Vec2> mDim;
      vector<float> mSpawn;
      vector<string> mWords;
      vector<WordState> mState;
      Vec2 mVel; // all text has same vel -> easier to read
      float mDt; // time since last update
      int mCurrent; // current text to type
      int mYcursorOffset; // y offset of cursor cursorpos;
      Vec2 mStartpos; // position of first line of text
   } mTxt;


   //----------------------------------------------
   // Explosions
   //----------------------------------------------
   class Explosions
   {
   public:
      void init()
      {
         mElapsedTime = 0;
         mExplosionAnimation.push_back(gExplosion1);
         mExplosionAnimation.push_back(gExplosion2);
         mExplosionAnimation.push_back(gExplosion3);
         mExplosionAnimation.push_back(gExplosion4);
      }

      void create(const Vec2& p, int c)
      {
         mPos.push_back(p-gDimExplosion*0.5);
         mStage.push_back(0);
         mColor.push_back(c);
      }

      void updateAndDraw(float dt)
      {
         mElapsedTime += dt;
         bool inc = false;
         if (mElapsedTime > RATE)
         {
            mElapsedTime = 0;
            inc = true;
         }

         for (int e = 0; e < mPos.size(); e++)
         {
            Vec2 p = mPos[e];
            int stage = mStage[e];
            int c = mColor[e];
            attron(COLOR_PAIR(c));	
            for (int i = 0; i < gDimExplosion.x; i++)
            {
               int x = p.x+i;
               if (x > LINES) continue;
               for (int j = 0; j < gDimExplosion.y; j++)
               {
                  int y = p.y+j;
                  if (y > COLS) continue;
                  mvaddch(x, y, mExplosionAnimation[stage][i][j]);
               }
            }
            attroff(COLOR_PAIR(c));	
            if (inc) mStage[e]++;
         }
   
         // remove finished explosions
         while (mStage[0] >= mExplosionAnimation.size())
         {
            mPos.pop_front();
            mStage.pop_front();
            mColor.pop_front();
         }
      }

   private:
      deque<Vec2> mPos;
      deque<int> mStage;
      deque<int> mColor;
      float mElapsedTime;
      vector<const char**> mExplosionAnimation;
      static const float RATE = 0.1;
   } mExplosions;

   //----------------------------------------------
   // Bee
   //----------------------------------------------
   struct Bee
   {
   public:
      void init(const Vec2& v, const Vec2& sp, int c)
      {
         mFlairOffset = rand() % gNumFlair;
         mVel = v;
         mStartpos = sp;
         mPos = sp;
         mColor = c;
         mPause = true;
      }

      void start()
      {
         init(mVel, mStartpos, mColor);
         mPause = false;
      }

      void stop()
      {
         mPause = true;
         if (mPause) draw(true);
      }

      bool inMotion()
      {
         return !mPause;
      }

      bool finished()
      {
         return ((mPos.x < -gDimBeeSprite.x || mPos.x > LINES-1) || 
                 (mPos.y < -gDimBeeSprite.y || mPos.y > COLS-1));
      }

      void draw(bool erase)
      {
         attron(COLOR_PAIR(mColor));
         for (int i = 0; i < gDimBeeSprite.x; i++)
         {
            int x = mPos.x+i;
            if (x < 0 || x > LINES) continue;
            for (int j = 0; j < gDimBeeSprite.y; j++)
            {
               int y = mPos.y+j;
               if (y < 0 || y > COLS) continue;
               if (erase)
               {
                  mvaddch(x, y, ' ');
               }
               else
               {
                  mvaddch(x, y, gBeeSprite[i][j]);
               }
            }
         }
         attroff(COLOR_PAIR(mColor));

         /*
         if (!erase)
         {
             int c = 3 + i%5;
             attron(COLOR_PAIR(c));
             attroff(COLOR_PAIR(c));
         }*/
      }
      
      void updateAndDraw(float _dt)
      {
         if (mPause) return;

         mDt += _dt;
         Vec2 numUnits = mVel * mDt;
         if (fabs(numUnits.x) == 0 && fabs(numUnits.y) == 0) return;
         
         mDt = 0;
         draw(true);
         mPos = mPos + numUnits;
         mFlairOffset = (mFlairOffset+1) % gNumFlair;
         mPos.x = mStartpos.x + gFlair[mFlairOffset];
         draw(false);
      }

      int trajectoryHeight() const
      {
         return mStartpos.x+2+gDimBeeSprite.x;
      }

   private:
      Vec2 mStartpos;
      Vec2 mPos;
      Vec2 mVel;
      int mFlairOffset;
      float mDt; // time since last update
      int mColor;
      bool mPause;
   } mBee;

};

int main(int argc, char **argv)
{
   try
   {
      float elapsedTime = 0; // todo: move to game class
      struct timespec then, now;
      clock_gettime(CLOCK_MONOTONIC, &then);

      TypingGame game;
      if (!game.loadFile("injust.txt")) // load default text
      {
         game.addLine("The quick brown fox jumps over the lazy dog.", 0);
      }      

      while (true)
      {
         char c = getch();
         if (c == 27) break;

         clock_gettime(CLOCK_MONOTONIC, &now);
         float dt = float(now.tv_sec - then.tv_sec + (now.tv_nsec - then.tv_nsec)/1000000000.0);
         elapsedTime += dt;
         then = now;

         //cout << now << " " << dt << endl;
         //char buff[32];
         //sprintf(buff, "%.2f,%.2f", dt,elapsedTime);
         //mvaddstr(1,0,buff);


         game.updateAndDraw(dt, c);
         Sleep(100);
      }
   }
   catch (exception& e)
   {
      cout << "Cannot init game. " << e.what() << endl;
   }

   return 0;             
}

