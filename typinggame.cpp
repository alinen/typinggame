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

const int TIME_OFFSET = 5;
int num_explosion_fade = 4;
int num_trail = 4;
Vec2 dim_explosion(3,5);
Vec2 dim_bee_sprite(4,8); //(6,12);
Vec2 dim_sky(11,52);
const char* trail = ".*oO";
const char* explosion1[] = {
" % % ",
"% % %",
" % % "};
const char* explosion2[] = {
" * * ",
"* * *",
" * * "};
const char* explosion3[] = {
" . . ",
". . .",
" . . "};
const char* explosion4[] = {
"     ",
"     ",
"     "};

const char* bee_sprite1[] = {
"     __     ",
"   _(__)_   ",
"  /-------@/",
"<(--------|D",
"  \\-------@\\",
"    (__)    "};

const char* bee_sprite[] = {
"   __   ",
"  (__\\_ ",
"-{{_{|8)",
"  (__/  "};

const char* sky1[] = {
"....................................................",
"..........(`      )...................._............",
".........(  _^ ^_  )................:(`  )`.........",
"........_(   )u(   '`...........:(   .    ).........",
".....=(`(      .     )......--..`.  (    ) )........",
"...((    (..__.:'-'.......+(   )...` _`  ) )........",
"...`(       ) ).........(   .  ).....(   )  ........",
".....` __.:'   ).......(   (   )).....`-'.-(`    )..",
"............--'.........`- __.'.........:(   ^ ^  ))",
".......................................`(    )V  )).",
"...................................................."};      

const char* sky[] = {
"           __----_                                  ",
"          (`......)                    _            ",
"         (. _^ ^_ .)                :(`..)`         ",
".       _(...)u( ..'`           :(........)         ",
"     =(`(..... ......)      --  `.  (. ..).)        ",
"   ((....(..__.:'-'       +(...)   ` _`..).)        ",
"   `(.......).)         (.. ...)     (...)          ",
"     ` __.:. ..)       (...(...)      `-'.-(`....)  ",
"            --'         `-.__.'         :(.. ^ ^ .))",
"                                       `(....)V .)) ",
"                                         ...        "};      


const int flair[15] = {0,-1,-1,-1,-1,-1,-1,0,1,1,1,1,1,0,0};
const int flairSize = 15; 

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
      init_pair(3, COLOR_RED, COLOR_BLACK);
      init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
      init_pair(5, COLOR_YELLOW, COLOR_BLACK);
      init_pair(6, COLOR_GREEN, COLOR_BLACK);
      init_pair(7, COLOR_BLUE, COLOR_BLACK);
      init_pair(8, COLOR_CYAN, COLOR_BLACK);

      ycursor_offset = 0;
      current = 0;
      dt = 0;
      elapsedTime = 0;
      startcol = ((int) COLS*0.5) - 19; // hard-coded for injust.txt
      startrow = LINES;
      startscreen = 3;

      attron(COLOR_PAIR(3));
      mvaddstr(LINES-1,0,"----------------"); 
      attroff(COLOR_PAIR(3));

      attron(COLOR_PAIR(4));
      mvaddstr(LINES-2,0,"----------------"); 
      attroff(COLOR_PAIR(4));
 
      attron(COLOR_PAIR(5));
      mvaddstr(LINES-3,0,"----------------"); 
      attroff(COLOR_PAIR(5)); 

      attron(COLOR_PAIR(6));
      mvaddstr(LINES-4,0,"----------------"); 
      attroff(COLOR_PAIR(6)); 

      attron(COLOR_PAIR(7));
      mvaddstr(LINES-5,0,"----------------"); 
      attroff(COLOR_PAIR(7));
   }

   void clear()
   {
      pos.clear();
      state.clear();
      vel.clear();
      dim.clear();
      words.clear();
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
            time += 2.0 + rand() % 5; // next text appears between 3 and 8 seconds later
         }
         textfile.close();
         //for (int i = 0; i < words.size(); i++)
         //{
         //   cout << words[i] << " " << pos[i] << " " << spawn[i] << " " << dim[i] << endl;
         //}
         return true;
      }
      return false;
   }

   void addLine(const string& line, float spawn_time)
   {
      char buffer[2056];
      strncpy(buffer, line.c_str(), 2056);

      int x = startrow;
      int num_spaces = 0;
      int y = startcol + rand() % 10 - 5;
      char* token = findword(buffer, ' ', num_spaces);
      while (token)
      {
         string word = token;

         words.push_back(word);
         dim.push_back(Vec2(1,word.size()));
         pos.push_back(Vec2(x, y));
         vel.push_back(Vec2(-3,0));
         spawn.push_back(spawn_time);
         state.push_back(WS_HIDDEN);
         tmpx.push_back(x);
         tmpy.push_back(y);

         y += word.size()+num_spaces;
         token = findword(NULL, ' ', num_spaces);
      }
   }

   void start()
   {
      move(0, 0); addstr("Press ESC to exit.\n");

      bee.startpos = Vec2(LINES*0.5, -dim_bee_sprite.y);
      bee.vel = Vec2(0,0);
      init_bee();

      explosion_animation.push_back(explosion1);
      explosion_animation.push_back(explosion2);
      explosion_animation.push_back(explosion3);
      explosion_animation.push_back(explosion4);

      drawSky(dim_sky, sky);
   }

   void eraseWord(int word_id)
   {
      // erase old word position and update pos
      for (int i = 0; i < words[word_id].size(); i++)
      {
          mvaddch(pos[word_id].x, pos[word_id].y+i, ' '); 
      }      
   }

   void drawInProgress(int word_id)
   {
      attron(COLOR_PAIR(WS_INPROGRESS));
      attron(A_BOLD);
      for (int i = 0; i <= ycursor_offset && i < words[word_id].size(); i++) 
      {
         mvaddch(pos[word_id].x, pos[word_id].y+i, words[word_id][i]); 
      }
      attroff(A_BOLD);
      attroff(COLOR_PAIR(WS_INPROGRESS));	
      for (int i = ycursor_offset+1; i < words[word_id].size(); i++) 
      {
         mvaddch(pos[word_id].x, pos[word_id].y+i, words[word_id][i]); 
      }            
   }

   void drawError(int word_id)
   {
      attron(COLOR_PAIR(WS_ERROR));	
      attron(A_BOLD);
      move(pos[word_id].x, pos[word_id].y); 
      addstr(words[word_id].c_str());
      attroff(A_BOLD);
      attroff(COLOR_PAIR(WS_ERROR));         
   }

   void drawNormal(int word_id)
   {
       move(pos[word_id].x, pos[word_id].y); 
       addstr(words[word_id].c_str());      
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
               mvaddch(i+startscreen, ypos+j, line[j]);
               //if (line[j] == '.') attroff(COLOR_PAIR(8));
            }
         }
         ypos += dim.y;
      }
   }

   void drawFail(int word_id)
   {
      attron(COLOR_PAIR(WS_ERROR));	
      for (int i = 0; i < words[word_id].size(); i++)
      {
         mvaddch(pos[word_id].x, pos[word_id].y+i, words[word_id][i]);
      }
      attroff(COLOR_PAIR(WS_ERROR));
   }

   bool intersection(int word_id)
   {
      for (int i = 0; i < words[word_id].size(); i++)
      {
         int x = pos[word_id].x;
         int y = pos[word_id].y+i;
         if (x >= startscreen+1 && x < dim_sky.x+startscreen+1)
         {
            int c = mvgetch(x, y);
            if (c != ' ')
            {
               create_explosion(Vec2(x,y),WS_ERROR);
               addch('$');
               return true;
            }
         }
      }
      return false;
   }

   void create_explosion(const Vec2& p, int c)
   {
      explosions.pos.push_back(p-dim_explosion*0.5);
      explosions.stage.push_back(0);
      explosions.color.push_back(c);
   }

   void draw_explosions()
   {
      for (int e = 0; e < explosions.pos.size(); e++)
      {
         Vec2 p = explosions.pos[e];
         int stage = explosions.stage[e];
         int c = explosions.color[e];
         attron(COLOR_PAIR(c));	
         for (int i = 0; i < dim_explosion.x; i++)
         {
            int x = p.x+i;
            if (x > LINES) continue;
            for (int j = 0; j < dim_explosion.y; j++)
            {
               int y = p.y+j;
               if (y > COLS) continue;
               mvaddch(x, y, explosion_animation[stage][i][j]);
            }
         }
         attroff(COLOR_PAIR(c));	
         explosions.stage[e]++;
      }

      // remove finished explosions
      while (explosions.stage[0] >= explosion_animation.size())
      {
         explosions.pos.pop_front();
         explosions.stage.pop_front();
         explosions.color.pop_front();
      }
   }

   void init_bee()
   {
      bee.flair_offset = rand() % flairSize;
      bee.vel = Vec2(0,1);
      bee.pos = bee.startpos;
   }

   void draw_bee(bool erase)
   {
      attron(COLOR_PAIR(5));
      for (int i = 0; i < dim_bee_sprite.x; i++)
      {
         int x = bee.pos.x+i;
         if (x < 0 || x > LINES) continue;
         for (int j = 0; j < dim_bee_sprite.y; j++)
         {
            int y = bee.pos.y+j;
            if (y < 0 || y > COLS) continue;
            if (erase) mvaddch(x, y, ' ');
            else
            {
               mvaddch(x, y, bee_sprite[i][j]);
            }
         }
      }
      attroff(COLOR_PAIR(5));
   }

   void updateAndDraw(float _dt, char c)
   {
      if (current >= words.size()) // all done!
      {
         move(1,0); addstr("You win!");
         return;
      }

      static float gXvel = -2.0;
      elapsedTime += _dt;
      dt += _dt;
      float tmp = gXvel * dt;
      int numUnits = (int) tmp;
      if (fabs(numUnits) > 0)
      {
         dt = 0; 
         for (int k = current; k < words.size(); k++)
         {
            if (state[k] == WS_HIDDEN && elapsedTime > spawn[k])
            {
               state[k] = WS_INIT;
               if (k-1 >= 0 && spawn[k] - spawn[k-1] > 3 && bee.pos.y >= COLS)
               {
                  init_bee();
               }
            }

            if (state[k] != WS_COMPLETE && state[k] != WS_FAIL && state[k] != WS_HIDDEN)
            {
               // update position
               eraseWord(k);
               tmpx[k] = tmpx[k] + numUnits;
               tmpy[k] = tmpy[k];
               pos[k].x = pos[k].x + numUnits; 

               if (pos[k].x < startscreen || intersection(k)) // beyond bounds
               {
                  state[k] = WS_FAIL;
               }
               pos[k].x = min(LINES-1, pos[k].x);
            }
         }

      }

      static float beev = 12.0;
      static float beedt = 0;
      beedt += _dt;
      tmp = beev * beedt;
      numUnits = (int) tmp;
      if (fabs(numUnits) > 0)
      {
         beedt = 0;
         draw_bee(true);
         bee.pos.y = bee.pos.y + numUnits;
         bee.flair_offset = (bee.flair_offset + numUnits) % flairSize;
         bee.pos.x = bee.startpos.x + flair[bee.flair_offset];
         draw_bee(false);
      }

      if (words[current][ycursor_offset] == c) // correct 
      {
         state[current] = WS_INPROGRESS;
         ycursor_offset++;
      }
      else if (c != -1 && c != ' ') // mistake
      {
         state[current] = WS_ERROR;
         ycursor_offset = 0; //restart word
      }

      // update based on user update
      for (int k = current; k < words.size(); k++)
      {
         assert (state[k] != WS_COMPLETE); // completed word, don't draw, shouldn't need this

         if (state[k] == WS_HIDDEN)
         {
            continue; // don't draw it yet
         }
         else if (state[k] == WS_INIT)
         {
            drawNormal(k);
         }
         else if (state[k] == WS_ERROR)
         {
            drawError(k);  
         }
         else if (state[k] == WS_INPROGRESS)
         {
            drawInProgress(k);
         }         
         else if (state[k] == WS_FAIL)
         {
            drawFail(k);
            if(k == current)
            {
               ycursor_offset = words[current].size(); // inc cursor, e.g. complete word automatically
            }
         }
      }
      
      if (ycursor_offset >= words[current].size()) //complete
      {
         state[current] = WS_COMPLETE;
         create_explosion(pos[current]+dim[current]*0.5, WS_INPROGRESS);
         eraseWord(current);
         ycursor_offset = 0;
         current++;
      }

      draw_explosions();
      move(pos[current].x, pos[current].y + ycursor_offset); 
      wrefresh(stdscr);
   }

private:
   vector<Vec2> pos;
   vector<Vec2> vel;
   vector<Vec2> dim;
   vector<float> spawn;
   vector<string> words;
   vector<float> tmpx;
   vector<float> tmpy;
   enum WordState {WS_INIT, WS_INPROGRESS, WS_ERROR, WS_COMPLETE, WS_FAIL, WS_HIDDEN};
   vector<WordState> state;

   enum RainbowColors { RB_1 = 3, RB_2, RB_3, RB_4, RB_5, RB_6 };

   struct Explosions
   {
      deque<Vec2> pos;
      deque<int> stage;
      deque<int> color;
   } explosions;
   vector<const char**> explosion_animation;

   struct Bee
   {
      Vec2 startpos;
      Vec2 pos;
      Vec2 vel;
      int flair_offset;
   } bee;

   float dt;
   float elapsedTime;
   int current;
   int ycursor_offset; // y offset of cursor cursorpos;
   int startrow,startcol;
   int startscreen;
};

int main(int argc, char **argv)
{
   try
   {
      float elapsedTime = 0;
      struct timespec then, now;
      clock_gettime(CLOCK_MONOTONIC, &then);

      TypingGame game;
      if (!game.loadFile("injust.txt")) // load default text
      {
         game.addLine("The quick brown fox jumps over the lazy dog.", 0);
      }      

      game.start();
      while (true)
      {
         char c = getch();
         if (c == 27) break;

         clock_gettime(CLOCK_MONOTONIC, &now);
         float dt = float(now.tv_sec - then.tv_sec + (now.tv_nsec - then.tv_nsec)/1000000000.0);
         elapsedTime += dt;
         then = now;

         //cout << now << " " << dt << endl;
         char buff[32];
         sprintf(buff, "%.2f,%.2f", dt,elapsedTime);
         mvaddstr(1,0,buff);


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

