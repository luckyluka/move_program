#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "rs232.h"

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 600

SDL_Renderer *renderer = NULL;
SDL_Window *window = NULL;
SDL_Texture *texture = NULL;
SDL_Color textColor = {255, 255, 255, 255};
SDL_Event event;
SDL_Point touchLocation = {-1, -1};
SDL_Rect fillRect = {0, 0, 0, 0};

TTF_Font *textFont = NULL;
struct editbox
{
  int x;
  int y;
  int w;
  int h;
  int posX;
  int posY;
  int hasFocus;
  char value[5];
};

int start = 0;
int cycleCounter = 0;
int cycleCheck = 0;

int timestamp = 0;
int oldtimestamp = 0;
int rows = 24;
int columns = 32;

char buffRows[20];
char buffColumns[20];


struct editbox eb[1000];

int editBox_id_counter;

int curX = 0;
int curY = 0;

char buffX[20];
char buffY[20];
char buffCount[20];
int textureWidth = 0;
int textureHeight = 0;

int i = 0;
int move_count = 0;
int pattern_move_count = 1;
int pattern2_move_count = 1;

int dir_move;

int holes;

int cport_nr=16;        /* /dev/ttyS0 (COM1 on windows) */
int bdrate=115200;       /* 9600 baud */
int i;
int n1;
int n2;
int n3;
int received1 = 0;
int received2 = 0;
int received3 = 0;
char mode[]={'8','N','1',0};
char str[3][512];
unsigned char buf1[4096];
unsigned char buf2[4096];
unsigned char buf3[4096];

int init()
{  
  if((SDL_Init(SDL_INIT_VIDEO||SDL_INIT_EVENTS)) != 0)
  {
    SDL_Log("Unable to initialize SDL:%s ", SDL_GetError());
    return 1;                                                                               
  }  

  window = SDL_CreateWindow("IT-Elektronika", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
  SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  
  if (window == NULL)
  {
    return 1;
  }		  

  renderer = SDL_CreateRenderer(window, - 1, SDL_RENDERER_SOFTWARE);
  if(renderer == NULL)
  {
    printf("RENDERER IS NULL\n");
  }

  if(TTF_Init() == -1)
  {
    printf("TTF ERROR\n");
  }
  textFont = TTF_OpenFont("BebasNeue Bold.ttf", 60);
  
  return 15;
}

void freeTexture(void)
{
  if(texture != NULL)
  {
    SDL_DestroyTexture(texture);	  
    texture = NULL;
    textureWidth = 0;
    textureHeight = 0;
  }
}

void draw(void)
{
  SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
  SDL_RenderClear(renderer);
}

void render(int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip)
{
  SDL_Rect renderQuad;
  renderQuad.x = x;
  renderQuad.y = y;
  renderQuad.w = textureWidth;
  renderQuad.h = textureHeight;

  if(clip != NULL)
  {
    renderQuad.w = clip -> w;
    renderQuad.h = clip -> h;
  }
  SDL_RenderCopyEx(renderer, texture, clip, &renderQuad, angle, center, flip);
}

void drawTextBox(int i, int x, int y, int w, int h)
{
  if(eb[i].hasFocus == 0)
  {
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderDrawLine(renderer, x, y, (x+w), y);
    SDL_RenderDrawLine(renderer, (x+w), y, (x+w), (y+h)); 
    SDL_RenderDrawLine(renderer, (x+w), (y+h), x, (y+h));
    SDL_RenderDrawLine(renderer, x, (y+h), x, y);
  }
  if(eb[i].posX == curX && eb[i].posY == curY)
  {
    SDL_Rect fillRect = {x, y, w, h};	  
    SDL_SetRenderDrawColor(renderer, 110, 70, 200, 70);
    SDL_RenderFillRect(renderer, &fillRect);
    SDL_RenderDrawLine(renderer, x, y, (x+w), y);
    SDL_RenderDrawLine(renderer, (x+w), y, (x+w), (y+h)); 
    SDL_RenderDrawLine(renderer, (x+w), (y+h), x, (y+h));
    SDL_RenderDrawLine(renderer, x, (y+h), x, y);
  }  
}


void drawEbGrid(void)
{
  int i;
  
  
  for(i = 0; i < holes; ++i)
  {
    drawTextBox(i, eb[i].x, eb[i].y, eb[i].w, eb[i].h);
  }
}

int writeText(const char *text, SDL_Color textColor)
{
 	
  SDL_Surface* textSurface;
  
  textSurface = TTF_RenderText_Solid(textFont, text, textColor);

  freeTexture();

  if(textSurface == NULL)
  {
    printf("Unable to render text surface! SDL Error: %s\n", SDL_GetError());
  }
  else
  {
    texture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if(texture == NULL)
    {
      printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
    }
    else
    {
      textureWidth = textSurface -> w;
      textureHeight = textSurface -> h;
    }
    SDL_FreeSurface(textSurface);
  }
  return texture != NULL;
}

void drawButton(int x,  int y, int w, int h)
{
  SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderDrawLine(renderer, x, y, (x+w), y);
  SDL_RenderDrawLine(renderer, (x+w), y, (x+w), (y+h)); 
  SDL_RenderDrawLine(renderer, (x+w), (y+h), x, (y+h));
  SDL_RenderDrawLine(renderer, x, (y+h), x, y);
  writeText("START: ", textColor);
  render(x+((w/2)-(textureWidth/2)), y + ((h/2)-(textureHeight/2)), NULL, 0.0, NULL, SDL_FLIP_NONE); 
  if(touchLocation.x > x && touchLocation.x < x+w && touchLocation.y > y && touchLocation.y < y + h && timestamp > oldtimestamp)
  {
    start = 1;
  }
}

void eventUpdate()
{
  while(SDL_PollEvent(&event) != 0 )
  {
   // printf("X:%f Y:%f\n", event.tfinger.x, event.tfinger.y); 
    if(event.type == SDL_FINGERDOWN)
    {
      
      timestamp = event.tfinger.timestamp;
      touchLocation.x = event.tfinger.x;
      touchLocation.y = event.tfinger.y;
    } 
    if(event.type == SDL_QUIT)
    {
      start = 1;
    }
  }
}

void plusRow()
{
  writeText("+", textColor);
  render(950, 50, NULL, 0.0, NULL, SDL_FLIP_NONE); 
  if(touchLocation.x > 900 && touchLocation.x < 1000 && touchLocation.y > 0 && touchLocation.y < 100 && timestamp > oldtimestamp && cycleCheck != cycleCounter)
  {
    cycleCheck = cycleCounter;
    rows++;
  } 
}

void minusRow()
{
  writeText("-", textColor);
  render(750, 50, NULL, 0.0, NULL, SDL_FLIP_NONE); 
  if(touchLocation.x > 700 && touchLocation.x < 800 && touchLocation.y > 0 && touchLocation.y < 100 && timestamp > oldtimestamp && cycleCheck != cycleCounter)
  {
    cycleCheck = cycleCounter;
    rows--;
  } 
}

void plusColumn()
{
  writeText("+", textColor);
  render(950, 150, NULL, 0.0, NULL, SDL_FLIP_NONE); 
  if(touchLocation.x > 900 && touchLocation.x < 1000 && touchLocation.y > 100 && touchLocation.y < 200 && timestamp > oldtimestamp && cycleCheck != cycleCounter)
  {
    cycleCheck = cycleCounter;
    columns++;
  }
}

void minusColumn()
{
  writeText("-", textColor);
  render(750, 150, NULL, 0.0, NULL, SDL_FLIP_NONE); 
  if(touchLocation.x > 700 && touchLocation.x < 800 && touchLocation.y > 100 && touchLocation.y < 200 && timestamp > oldtimestamp && cycleCheck != cycleCounter)
  {
    cycleCheck = cycleCounter;
    columns--;
  }
}


void initVars()
{
  int x = 50;
  int y = 500;
  int w = 10;
  int h = 10;
  int drawRows = 0;
  int drawColumns = 0;
  int counter = 1;
  
  editBox_id_counter = 0;

  for(drawRows = 0; drawRows < rows ; ++drawRows)
  {
    for(drawColumns = 0; drawColumns < columns; ++drawColumns)
    {
      eb[editBox_id_counter].x = x;
      eb[editBox_id_counter].y = y;
      eb[editBox_id_counter].w = w;
      eb[editBox_id_counter].h = h;
      eb[editBox_id_counter].hasFocus = 0;
      eb[editBox_id_counter].posX = drawColumns;
      eb[editBox_id_counter].posY = drawRows;
      editBox_id_counter++;
      x = x + w;
      counter++;
    }
    x = 50;
    y = y - h;  
  }
}


void up()
{
  curY++;	
  printf("%d:UP\n", move_count);
}
void down()
{
  curY--;	
  printf("%d:DOWN\n", move_count);
}
void left()
{
  curX--;	
  printf("%d:LEFT\n", move_count);
}
void right()
{
  curX++;
  printf("%d:RIGHT\n", move_count);
  //RS232_cputs(cport_nr, str[1]);
  //printf("sent: %s\n", str[1]);
  //usleep(1000000);
  
  //n2 = RS232_PollComport(cport_nr, buf2, 4095);
  //while(received2 == 0)
  //{
  //  if(n2 > 0)
  //  {
  //    buf2[n2] = 0;   /* always put a "null" at the end of a string! */

    //  for(i=0; i < n2; i++)
     // {
      //  if(buf2[i] < 32)  /* replace unreadable control-codes by dots */
       // {
        //  buf2[i] = '.';
       // }
     // }
    //  printf("received %i bytes: %s\n", n2, (char *)buf2);
    //  received2= 1;
   // }
 // }

 // RS232_cputs(cport_nr, str[2]);
  //printf("sent: %s\n", str[2]);
 /// usleep(1000000);
 
//  n3 = RS232_PollComport(cport_nr, buf3, 4095);
//  while(received3 == 0)
 // {
  //  if(n3 > 0)
  //  {
    //  buf3[n3] = 0;   /* always put a "null" at the end of a string! */

     // for(i=0; i < n3; i++)
     // {
      //  if(buf3[i] < 32)  /* replace unreadable control-codes by dots */
       // {
        //  buf3[i] = '.';
       // }
     // }
   //   printf("received %i bytes: %s\n", n3, (char *)buf3);
     // received3 = 1;
   // }
 // }
}

void diagonal()
{
  curX++;
  curY++;
  printf("%d:DIAGONAL\n", move_count);
}

void ll_grid()
{ 
  dir_move = 1;
  
  for(move_count=0;move_count<holes+1;move_count++)
  {
    printf("%d\n", move_count);
    draw();	
    drawEbGrid();
    
    sprintf(buffCount, "N:%d", move_count);
    writeText(buffCount, textColor);
    render(500, 350, NULL, 0.0, NULL, SDL_FLIP_NONE);

    sprintf(buffX, "X:%d", curX);
    writeText(buffX, textColor);
    render(500, 400, NULL, 0.0, NULL, SDL_FLIP_NONE);

    sprintf(buffY, "Y:%d", curY);
    writeText(buffY, textColor);
    render(500, 450, NULL, 0.0, NULL, SDL_FLIP_NONE);
    
    SDL_RenderPresent(renderer);
    SDL_RenderClear(renderer);
    SDL_Delay(1);
    if(move_count == 0)
    {
      diagonal();
    }
    else if(move_count < ((columns*2)-2) && move_count != 0)
    {
      switch(pattern_move_count)
      {
        case 1:
          down();
          break;
        case 2:
          right();
          break;
        case 3:
          up();
          
          break;
        case 4:
          right();
          break;
      }
      if(pattern_move_count < 4) 
      {
        pattern_move_count++;
      }
      else
      {
        pattern_move_count = 1; 
      }
    }
    else if(move_count >= ((columns*2)-2) && move_count < ((columns*2)-1))
    {
      up();
    }
    else if(move_count >= ((columns*2)-1) && move_count < ((holes - rows)))
    {
      if(dir_move == 1 && pattern2_move_count < columns-1)
      {
        left();
        pattern2_move_count++;
      }   
      else if(dir_move == 0 && pattern2_move_count < columns-1)
      {
        right();
        pattern2_move_count++;
      }
      else if(dir_move == 0 && pattern2_move_count == columns-1 && move_count != (holes-rows))
      {
        up();      
        dir_move = 1;
        pattern2_move_count = 1;
      }
      else if(dir_move == 1 && pattern2_move_count == columns-1  && move_count != (holes-rows))
      {
        up();
        dir_move = 0;
        pattern2_move_count = 1;
      }
    }
    else if(move_count == ((holes - rows)))
    {
      left();    
    }
    else if(move_count > (holes - rows) && move_count <= holes) 
    {
      printf("%d <= %d \n", move_count, holes);
      down();
    }
    else
    {
      printf("---------------------------------%d------NO MOVE ---------------------------------------------\n", move_count);
    }

  }
}

void ss_grid()
{ 
  dir_move = 0;
  printf("SS GRID\n");

  for(move_count=0;move_count<holes+1;++move_count)
  {
    //printf("SS GRID - for loop beggining)\n");
    printf("**************\n");
    printf("%d\n", move_count);
    draw();	
    drawEbGrid();
    
    sprintf(buffCount, "N:%d", move_count);
    writeText(buffCount, textColor);
    render(500, 350, NULL, 0.0, NULL, SDL_FLIP_NONE);

    sprintf(buffX, "X:%d", curX);
    writeText(buffX, textColor);
    render(500, 400, NULL, 0.0, NULL, SDL_FLIP_NONE);

    sprintf(buffY, "Y:%d", curY);
    writeText(buffY, textColor);
    render(500, 450, NULL, 0.0, NULL, SDL_FLIP_NONE);
    
    SDL_RenderPresent(renderer);
    SDL_RenderClear(renderer);
    SDL_Delay(1);
    if(move_count < columns-1)
    {
      right();
    }
    else if(move_count == columns-1)
    {
      up();
    }
    else if(move_count > columns-1 && move_count < ((holes - rows)))
    {
     // printf("SS GRID - first else if: move_count:%d  pattern_move_count: %d  columns: %d dir_move: %d\n", move_count, pattern2_move_count, columns, dir_move);

      if(dir_move == 0 && pattern2_move_count < columns-1)
      {
        left();
        pattern2_move_count++;
      }   
      else if(dir_move == 1 && pattern2_move_count < columns-1)
      {
        right();
        pattern2_move_count++;
      }
      else if(dir_move == 0 && pattern2_move_count == columns-1 && move_count != (holes - rows))
      {
        up();      
        dir_move = 1;
        pattern2_move_count = 1;
      }
      else if(dir_move == 1 && pattern2_move_count == columns-1 && move_count != (holes - rows))
      {
        up();
        dir_move = 0;
        pattern2_move_count = 1;
      }
    }
    else if(move_count == ((holes - rows)))
    {
 //     printf("SS GRID - second else if\n");
      left();    
    }
    else if(move_count > (holes - rows) && move_count <= holes) 
    {
   //   printf("SS GRID - third else if\n");
      down();
    }
  }
}

int main()
{
  printf("**************\n");
  printf("MOVE PROGRAM\n");
  SDL_Delay(2000);

  strcpy(str[0], "I00HT*");

  strcpy(str[1], "I00CY005000.000000000040011110001001*");
 
  strcpy(str[2], "I00SY*");

  if(RS232_OpenComport(cport_nr, bdrate, mode))
  {
    printf("Can not open comport\n");

    return(0);
  }
 
  RS232_cputs(cport_nr, str[0]);
  
  printf("sent: %s\n", str[0]);
  usleep(1000000);
 
  n1 = RS232_PollComport(cport_nr, buf1, 4095);
  
  while(received1 == 0)
  {
    if(n1 > 0)
    {
      buf1[n1] = 0;   /* always put a "null" at the end of a string! */

      for(i=0; i < n1; i++)
      {
        if(buf1[i] < 32)  /* replace unreadable control-codes by dots */
        {
          buf1[i] = '.';
        }
      }
      printf("received %i bytes: %s\n", n1, (char *)buf1);
      received1 = 1;
    }
  }


  init();
  
  while(!start)
  {
    draw();
   
    eventUpdate(); 
    writeText("VRSTICE: ", textColor);
    render(500, 50, NULL, 0.0, NULL, SDL_FLIP_NONE); 
    
    plusRow();
    minusRow();
    plusColumn();
    minusColumn();

    sprintf(buffRows, "%d", rows);
    writeText(buffRows, textColor);
    render(830, 50, NULL, 0.0, NULL, SDL_FLIP_NONE); 

    sprintf(buffColumns, "%d", columns);
    writeText(buffColumns, textColor);
    render(830, 150, NULL, 0.0, NULL, SDL_FLIP_NONE);

    writeText("STOLPCI: ", textColor);
    render(500, 150, NULL, 0.0, NULL, SDL_FLIP_NONE); 
  
    drawButton(500, 300, 500, 100);
   
    initVars();
    holes = rows * columns;
    drawEbGrid();
    SDL_RenderPresent(renderer);	
    cycleCounter++;
    oldtimestamp = timestamp;
  }	  	  
  holes = rows * columns;
  if(rows%2==0 && columns%2==0)
  {
    printf("SODO\n");
    ss_grid();
  }
  else
  {
    printf("LIHO\n");
    ll_grid();
  }
  SDL_Delay(5000);

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit(); 
  return 1;
}
