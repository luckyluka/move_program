#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "rs232.h"
#include "func_init.h"

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 600
#define DROWSAPP "71395"

SDL_Renderer *renderer = NULL;
SDL_Window *window = NULL;
SDL_Texture *texture = NULL;
SDL_Color textColor = {255, 255, 255, 255};
SDL_Event event;
SDL_Point touchLocation = {-1, -1};
SDL_Rect fillRect = {0, 0, 0, 0};
TTF_Font *textFont = NULL;

FILE *fp;

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

int PiControlHandle_g = -1;
int start = 0;
int cycleCounter = 0;
int cycleCheck = 0;

int timestamp = 0;
int oldtimestamp = 0;
int rows = 24;
int columns = 32;
int program;

char passText[5];

char buffRows[20];
char buffColumns[20];
char numBuff[10];

struct editbox eb[10000];
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
int n;
int received = 0;
char mode[]={'8','N','1',0};
char str[13][512];
unsigned char buf[4096];

int pageNumber;

int flags;
int innited;

int init()    /* things needed to start sdl2 properly */
{  
  flags = IMG_INIT_JPG|IMG_INIT_PNG;
  innited = IMG_Init(flags);
  	
  if((SDL_Init(SDL_INIT_VIDEO||SDL_INIT_EVENTS)) != 0)
  {
    SDL_Log("Unable to initialize SDL:%s ", SDL_GetError());
    return 1;                                                                               
  }  

  window = SDL_CreateWindow("IT-Elektronika", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
  SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_BORDERLESS);
  
  if (window == NULL)
  {
    return 1;
  }		  

  renderer = SDL_CreateRenderer(window, - 1, SDL_RENDERER_SOFTWARE);
  if(renderer == NULL)
  {
    printf("RENDERER IS NULL\n");
  }

  if((innited&flags) != flags)
  {
    printf("IMG_INIT: Failed to init required jpg an png support!\n");
    printf("IMG_INIT: %s\n", IMG_GetError());
  }

  if(TTF_Init() == -1)
  {
    printf("TTF ERROR\n");
  }
  textFont = TTF_OpenFont("BebasNeue Bold.ttf", 60);
  
  return 15;
}

void comm_init()
{
  strcpy(str[0], "I00HT*");

  strcpy(str[1], "I00CX005000.000000000120011110001001*");     /* X plus */

  strcpy(str[2], "I00CX005000.000000000120001110001001*");     /* X minus */
  
  strcpy(str[3], "I00CY005000.000000000120011110001001*");     /* Y plus */
  
  strcpy(str[4], "I00CY005000.000000000120001110001001*");     /* Y minus */
  
  strcpy(str[5], "I00CX000500.000100000120001110001001*");    /* HOME X */

  strcpy(str[6], "I00CY000500.000100000120001110001001*");    /* HOME Y */
  
  strcpy(str[7], "I00CX000500.000100000120001110001001*");    /* PARK X - to be defined */  
  
  strcpy(str[8], "I00CY000500.000100000120001110001001*");    /* PARK Y - to be defined*/
  
  strcpy(str[9], "I00SX*");
  
  strcpy(str[10], "I00SY*");
  
  strcpy(str[11], "I00KX1*");
  
  strcpy(str[12], "I00KY1*");

  strcpy(str[13], "I00KX0*");
  
  strcpy(str[14], "I00KY0*");

  if(RS232_OpenComport(cport_nr, bdrate, mode))
  {
    printf("Can not open comport\n");

    return(0);
  }
}

void freeTexture(void)  /* taking care of memory */
{
  if(texture != NULL)
  {
    SDL_DestroyTexture(texture);	  
    texture = NULL;
    textureWidth = 0;
    textureHeight = 0;
  }
}

void draw(void)     /* drawing background */
{
  SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
  SDL_RenderClear(renderer);
}

void render(int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip) /* render something(object) to screen */
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

void drawTextBox(int i, int x, int y, int w, int h)  /*drawing one text box */
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


void drawEbGrid(void)   /* drawing many textboxes */
{
  int i;
  
  for(i = 0; i < holes; ++i)
  {
    drawTextBox(i, eb[i].x, eb[i].y, eb[i].w, eb[i].h);
  }
}

int writeText(const char *text, SDL_Color textColor)  /* text to be written, after the call to this function a render function call must be made */
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

void drawButton(int x,  int y, int w, int h)    /* button to start movement - obsolete - will use physical button */
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

void eventUpdate()   /* handling touch events */
{
  while(SDL_PollEvent(&event) != 0 )
  {
    
    if(event.type == SDL_FINGERDOWN)  
    {
      
      timestamp = event.tfinger.timestamp;
      touchLocation.x = event.tfinger.x;
      touchLocation.y = event.tfinger.y;
    }
    /* 
    if(event.type == SDL_QUIT)
    {
      start = 1;
    }
    
    
    if(event.type == SDL_MOUSEBUTTONDOWN)  
    {
      
      timestamp = event.button.timestamp;
      touchLocation.x = event.button.x;
      touchLocation.y = event.button.y;
    } 

    if(event.type == SDL_QUIT)
    {
      start = 1;
    }
    */
  }
}

void plusRow()  /* plus button to increment value of rows */
{
  writeText("+", textColor);
  render(950, 100, NULL, 0.0, NULL, SDL_FLIP_NONE); 
  if(touchLocation.x > 900 && touchLocation.x < 1000 && touchLocation.y > 50 && touchLocation.y < 150 && timestamp > oldtimestamp && cycleCheck != cycleCounter)
  {
    cycleCheck = cycleCounter;
    rows++;
  } 
}

void minusRow()   /* minus button to decrement value of rows */
{
  writeText("-", textColor);
  render(650, 100, NULL, 0.0, NULL, SDL_FLIP_NONE); 
  if(touchLocation.x > 600 && touchLocation.x < 700 && touchLocation.y > 50 && touchLocation.y < 150 && timestamp > oldtimestamp && cycleCheck != cycleCounter)
  {
    cycleCheck = cycleCounter;
    rows--;
  } 
}

void plusColumn() /* plus button to increment value of columns */
{
  writeText("+", textColor);
  render(950, 200, NULL, 0.0, NULL, SDL_FLIP_NONE); 
  if(touchLocation.x > 900 && touchLocation.x < 1000 && touchLocation.y > 150 && touchLocation.y < 250 && timestamp > oldtimestamp && cycleCheck != cycleCounter)
  {
    cycleCheck = cycleCounter;
    columns++;
  }
}

void minusColumn() /* minus button to decrement value of rows */
{
  writeText("-", textColor);
  render(650, 200, NULL, 0.0, NULL, SDL_FLIP_NONE); 
  if(touchLocation.x > 600 && touchLocation.x < 700 && touchLocation.y > 150 && touchLocation.y < 250 && timestamp > oldtimestamp && cycleCheck != cycleCounter)
  {
    cycleCheck = cycleCounter;
    columns--;
  }
}

void initVars(int x, int y, int w, int h)  /* handling struct for drawing grids */
{
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

void command(int number)  /*  sending serial communication command */
{
  RS232_cputs(cport_nr, str[number]);
  received = 0;
  printf("sent: %s\n", str[number]);
  usleep(100000);
  
  n = RS232_PollComport(cport_nr, buf, 4095);
  while(received == 0)
  {
    if(n > 0)
    {
      buf[n] = 0;   /* always put a "null" at the end of a string! */

      for(i=0; i < n; i++)
      {
        if(buf[i] < 32)  /* replace unreadable control-codes by dots */
        {
          buf[i] = '.';
        }
      }
      printf("received %i bytes: %s\n", n, (char *)buf);
      received = 1;
    }
  }
}


void up()                     /*movement up */
{
  curY++;	
  printf("%d:UP\n", move_count);
  
  command(3);
  command(10);
  /*
  handle_spring();
  */
}
void down()                    /*movement down */
{
  curY--;	
  printf("%d:DOWN\n", move_count);
  
  command(4);
  command(10);
  /*
  handle_spring();
  */
}
void left()                    /*movement left */
{
  curX--;	
  printf("%d:LEFT\n", move_count);

  command(2);
  command(9);
  /*
  handle_spring();
  */
}
void right()                   /*movement right */
{
  curX++;
  printf("%d:RIGHT\n", move_count);
  
  command(1);
  command(9);
  /*
  handle_spring();
  */
}

void diagonal()              /*moving diagonally */
{
  curX++;
  curY++;
  printf("%d:DIAGONAL\n", move_count);
  /*
  command(1);
  command(3);
  command(6);*/
  /*
  handle_sping();
  */
}

void home()                /*moving to home position */
{
  printf("HOME\n");
  /*
  command(11);
  command(12);
   
  command(5);
  command(6);
  */
}

void park_from_home()                /*movement to park position */
{
/*
  printf("PARK\n");
  command(11);
  command(12);
  command(7);
  command(8);
  command(13);
  command(14);
*/
}

void park_from_work()
{
  /*
  command(7);
  command(8);
  */
}

void start_procedure()
{
  home();
  while(readVariableValue("I_2") == 0 && readVariableValue("I_3") == 0)
  {
    printf("GOING HOME\n"); 
  }
}

void up_button(int x,  int y)    /* drawing button to go up */
{
  SDL_Surface *imageSurface;
  freeTexture();
  imageSurface = IMG_Load("/home/pi/move_program/up.png");

  if(imageSurface == NULL)
  {
    printf("Unable to render image surface! SDL_ImageError: %s\n", IMG_GetError());
  }
  else
  {
    texture = SDL_CreateTextureFromSurface(renderer, imageSurface);
    if(texture == NULL)
    {
      printf("Unable to create texture from rendered image! SDL_ImageError: %s\n", SDL_GetError());
    }
    else
    {
      textureWidth = imageSurface -> w;
      textureHeight = imageSurface -> h;
    }
    SDL_FreeSurface(imageSurface);
  }

  render(x, y, NULL, 0.0, NULL, SDL_FLIP_NONE);
  
  if(touchLocation.x > x && touchLocation.x < x+100 && touchLocation.y > y && touchLocation.y < y + 100 && timestamp > oldtimestamp)
  {
    up();
  }
}

void down_button(int x,  int y)       /* drawing button to go down */
{
  SDL_Surface *imageSurface;
  freeTexture();
  imageSurface = IMG_Load("/home/pi/move_program/down.png");

  if(imageSurface == NULL)
  {
    printf("Unable to render image surface! SDL_ImageError: %s\n", IMG_GetError());
  }
  else
  {
    texture = SDL_CreateTextureFromSurface(renderer, imageSurface);
    if(texture == NULL)
    {
      printf("Unable to create texture from rendered image! SDL_ImageError: %s\n", SDL_GetError());
    }
    else
    {
      textureWidth = imageSurface -> w;
      textureHeight = imageSurface -> h;
    }
    SDL_FreeSurface(imageSurface);
  }	
  render(x, y, NULL, 0.0, NULL, SDL_FLIP_NONE);

  if(touchLocation.x > x && touchLocation.x < x+100 && touchLocation.y > y && touchLocation.y < y + 100 && timestamp > oldtimestamp)
  {
    down();
  }
}

void left_button(int x,  int y)              /* drawing button to go left */
{
  SDL_Surface *imageSurface;
  freeTexture();
  imageSurface = IMG_Load("/home/pi/move_program/left.png");

  if(imageSurface == NULL)
  {
    printf("Unable to render image surface! SDL_ImageError: %s\n", IMG_GetError());
  }
  else
  {
    texture = SDL_CreateTextureFromSurface(renderer, imageSurface);
    if(texture == NULL)
    {
      printf("Unable to create texture from rendered image! SDL_ImageError: %s\n", SDL_GetError());
    }
    else
    {
      textureWidth = imageSurface -> w;
      textureHeight = imageSurface -> h;
    }
    SDL_FreeSurface(imageSurface);
  }
  
  render(x, y, NULL, 0.0, NULL, SDL_FLIP_NONE);

  if(touchLocation.x > x && touchLocation.x < x+100 && touchLocation.y > y && touchLocation.y < y + 100 && timestamp > oldtimestamp)
  {
    left();
  }
}

void right_button(int x,  int y)       /* drawing button to go right */
{
  SDL_Surface *imageSurface;
  freeTexture();
  imageSurface = IMG_Load("/home/pi/move_program/right.png");

  if(imageSurface == NULL)
  {
    printf("Unable to render image surface! SDL_ImageError: %s\n", IMG_GetError());
  }
  else
  {
    texture = SDL_CreateTextureFromSurface(renderer, imageSurface);
    if(texture == NULL)
    {
      printf("Unable to create texture from rendered image! SDL_ImageError: %s\n", SDL_GetError());
    }
    else
    {
      textureWidth = imageSurface -> w;
      textureHeight = imageSurface -> h;
    }
    SDL_FreeSurface(imageSurface);
  }
  render(x, y, NULL, 0.0, NULL, SDL_FLIP_NONE);

  if(touchLocation.x > x && touchLocation.x < x+100 && touchLocation.y > y && touchLocation.y < y + 100 && timestamp > oldtimestamp)
  {
    right();
  }
}

void button(int x, int y, int w, int h, const char* text, int gotoNum) /* menu button - move to another page (settings/manual) */
{
  SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderDrawLine(renderer, x, y, (x+w), y);
  SDL_RenderDrawLine(renderer, (x+w), y, (x+w), (y+h)); 
  SDL_RenderDrawLine(renderer, (x+w), (y+h), x, (y+h));
  SDL_RenderDrawLine(renderer, x, (y+h), x, y);
  writeText(text, textColor);
  render(x+((w/2)-(textureWidth/2)), y + ((h/2)-(textureHeight/2)), NULL, 0.0, NULL, SDL_FLIP_NONE); 
  if(touchLocation.x > x && touchLocation.x < x+w && touchLocation.y > y && touchLocation.y < y + h && timestamp > oldtimestamp)
  {
    pageNumber = gotoNum;
  }
}

void button_save(int x, int y, int w, int h)  /* save row/column data button */
{
  SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderDrawLine(renderer, x, y, (x+w), y);
  SDL_RenderDrawLine(renderer, (x+w), y, (x+w), (y+h)); 
  SDL_RenderDrawLine(renderer, (x+w), (y+h), x, (y+h));
  SDL_RenderDrawLine(renderer, x, (y+h), x, y);
  writeText("SHRANI", textColor);
  render(x+((w/2)-(textureWidth/2)), y + ((h/2)-(textureHeight/2)), NULL, 0.0, NULL, SDL_FLIP_NONE); 
  if(touchLocation.x > x && touchLocation.x < x+w && touchLocation.y > y && touchLocation.y < y + h && timestamp > oldtimestamp)
  {
    system("rm param.txt");	  
    fp = fopen("/home/pi/move_program/param.txt", "w");
    fprintf(fp,"%d\n", rows);
    fprintf(fp, "%d\n", columns);  
    fclose(fp);
  }
  
}

void keypad(int x, int y, int w, int h)  /* drawing keypad for entering password */
{
  int i = 1;
  int j = 1;
  int k = 0;
  int nums = 1;
  int origX = x;
  int origY = y;
  
  SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderDrawLine(renderer, x, y-h, (x+w*3), y-h);
  SDL_RenderDrawLine(renderer, (x+w*3), y-h, (x+w*3), y); 
  SDL_RenderDrawLine(renderer, (x+w*3), y, x, y);
  SDL_RenderDrawLine(renderer, x, y, x, y-h);
  writeText(passText, textColor);
  render(x, y - h , NULL, 0.0, NULL, SDL_FLIP_NONE); 
  
  for(k = 0; k < 5; ++k)
  {
    printf("pass text: %c\n", passText[k]);
  } 

  for(j = 1; j < 4; ++j)
  {  
    for(i = 1; i < 4; ++i)
    {
      SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
      SDL_RenderDrawLine(renderer, x, y, (x+w), y);
      SDL_RenderDrawLine(renderer, (x+w), y, (x+w), (y+h)); 
      SDL_RenderDrawLine(renderer, (x+w), (y+h), x, (y+h));
      SDL_RenderDrawLine(renderer, x, (y+h), x, y);
      sprintf(numBuff, "%d", nums);
      writeText(numBuff, textColor);
      render(x+((w/2)-(textureWidth/2)), y + ((h/2)-(textureHeight/2)), NULL, 0.0, NULL, SDL_FLIP_NONE); 
      if(touchLocation.x > x && touchLocation.x < x+w && touchLocation.y > y && touchLocation.y < y + h && timestamp > oldtimestamp && cycleCheck != cycleCounter)
      {
        cycleCheck = cycleCounter;
        if(passText[4] == '\0')
        {
          strcat(passText, numBuff);
        }
      }
      if(passText[4] != '\0')
      {
        int ret;
        ret = strcmp(DROWSAPP, passText);
        if(ret == 0)
        {
          printf("RET: %d\n", ret);
          printf("%s equal to %s\n", DROWSAPP, passText);
          printf("ACCESS GRANTED\n");
          pageNumber = 3;
        }
        memset(&passText[0], 0, 5);
      }

      x = x + w;
      nums++;
    }
    x = origX;
    y = y + h;
  }
  y = origY;
}




void admin(int x, int y, int w, int h, int gotoNum) /* move to admin area button */
{
  SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderDrawLine(renderer, x, y, (x+w), y);
  SDL_RenderDrawLine(renderer, (x+w), y, (x+w), (y+h)); 
  SDL_RenderDrawLine(renderer, (x+w), (y+h), x, (y+h));
  SDL_RenderDrawLine(renderer, x, (y+h), x, y);
  writeText(" . . . ", textColor);
  render(x+((w/2)-(textureWidth/2)), y + ((h/2)-(textureHeight/2)), NULL, 0.0, NULL, SDL_FLIP_NONE); 

  if(touchLocation.x > x && touchLocation.x < x+w && touchLocation.y > y && touchLocation.y < y + h && timestamp > oldtimestamp && cycleCounter != cycleCheck)
  {
    cycleCheck = cycleCounter;
    pageNumber = gotoNum;
    memset(&passText[0], 0, 5);
  }
}

void handle_spring()
{
  while(readVariableValue("I_1") == 0 && move_count != holes)  // do not wait at the last hole. it was filled in the beggining //
  {
    printf("WAITING FOR SPRING\n");
  }
  writeVariableValue("O_1", 1);
  SDL_Delay(10);
  writeVariableValue("O_1", 0); 
}

void ll_grid()  /* start movement procedure for odd value grid */
{ 
  dir_move = 1;
  pattern_move_count = 1;
  pattern2_move_count = 1;
  for(move_count=0;move_count<holes;move_count++)
  {
    printf("%d\n", readVariableValue("I_1"));
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
    SDL_Delay(300);
  }
  /*
  park();
  */
}

void ss_grid()/* start movement procedure for even value grid */
{ 
  dir_move = 0;
  pattern2_move_count = 1;
  printf("SS GRID\n");
  
  for(move_count=0;move_count<holes;++move_count)
  {
    printf("**************\n");
    printf("%d\n", move_count);
    draw();	
    drawEbGrid();
    
    sprintf(buffCount, "N:%d", move_count);
    writeText(buffCount, textColor);
    render(750, 400, NULL, 0.0, NULL, SDL_FLIP_NONE);

    sprintf(buffX, "X:%d", curX);
    writeText(buffX, textColor);
    render(750, 450, NULL, 0.0, NULL, SDL_FLIP_NONE);

    sprintf(buffY, "Y:%d", curY);
    writeText(buffY, textColor);
    render(750, 500, NULL, 0.0, NULL, SDL_FLIP_NONE);
    
    sprintf(buffRows, "VRSTICE:%d", rows);
    writeText(buffRows, textColor);
    render(750, 100, NULL, 0.0, NULL, SDL_FLIP_NONE); 
    
    sprintf(buffColumns, "STOLPCI:%d", columns);
    writeText(buffColumns, textColor);
    render(750, 200, NULL, 0.0, NULL, SDL_FLIP_NONE);
   
    SDL_RenderPresent(renderer);
    SDL_RenderClear(renderer);
    
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
      left();    
    }
    else if(move_count > (holes - rows) && move_count <= holes) 
    {
      down();
    }
    else
    {
      printf("---------------------------------%d------NO MOVE ---------------------------------------------\n", move_count);
    }
    printf("X: %d  Y: %d DIRECTION: %d \n", curX, curY, dir_move);
    SDL_Delay(300);
  }
/*
  park();
  */
}

void ls_grid()               /* start movement procedure for odd and even value grid */
{ 
  dir_move = 0;
  pattern2_move_count = 1;
  printf("SS GRID\n");

  for(move_count=0;move_count<holes;++move_count)
  {
    printf("**************\n");
    printf("%d\n", move_count);
    draw();	
    drawEbGrid();
    
    sprintf(buffCount, "N:%d", move_count);
    writeText(buffCount, textColor);
    render(750, 400, NULL, 0.0, NULL, SDL_FLIP_NONE);

    sprintf(buffX, "X:%d", curX);
    writeText(buffX, textColor);
    render(750, 450, NULL, 0.0, NULL, SDL_FLIP_NONE);

    sprintf(buffY, "Y:%d", curY);
    writeText(buffY, textColor);
    render(750, 500, NULL, 0.0, NULL, SDL_FLIP_NONE);
    
    sprintf(buffRows, "VRSTICE:%d", rows);
    writeText(buffRows, textColor);
    render(750, 100, NULL, 0.0, NULL, SDL_FLIP_NONE); 
    
    sprintf(buffColumns, "STOLPCI:%d", columns);
    writeText(buffColumns, textColor);
    render(750, 200, NULL, 0.0, NULL, SDL_FLIP_NONE);
   
    SDL_RenderPresent(renderer);
    SDL_RenderClear(renderer);
    
    if(move_count < rows-1)
    {
      up();
    }
    else if(move_count == rows-1)
    {
      right();
    }
    else if(move_count > rows-1 && move_count < ((holes - columns)))
    {
      if(dir_move == 0 && pattern2_move_count < rows-1)
      {
        down();
        pattern2_move_count++;
      }   
      else if(dir_move == 1 && pattern2_move_count < rows-1)
      {
        up();
        pattern2_move_count++;
      }
      else if(dir_move == 0 && pattern2_move_count == rows-1 && move_count != (holes - columns))
      {
        right();      
        dir_move = 1;
        pattern2_move_count = 1;
      }
      else if(dir_move == 1 && pattern2_move_count == rows-1 && move_count != (holes - columns))
      {
        right();
        dir_move = 0;
        pattern2_move_count = 1;
      }
    }
    else if(move_count == ((holes - columns)))
    {
      down();    
    }
    else if(move_count > (holes - columns) && move_count < holes) 
    {
      left();
    }
    else
    {
      printf("---------------------------------%d------NO MOVE ---------------------------------------------\n", move_count);
    }
    printf("X: %d  Y: %d DIRECTION: %d  HOLES: %d\n", curX, curY, dir_move, holes);
    
    SDL_Delay(300);
  }
  /*
  park();
  */
}

int page_main()   /* setting up main page */
{
  printf("PAGE MAIN\n");
  
  while(!start && pageNumber == 1)
  {
    draw();
    eventUpdate(); 
    initVars(50, 550, 15, 15);
    holes = rows * columns;
    drawEbGrid();
    
    admin(945, 0, 75, 50, 2);
    
    sprintf(buffCount, "N:%d", move_count);
    writeText(buffCount, textColor);
    render(750, 400, NULL, 0.0, NULL, SDL_FLIP_NONE);

    sprintf(buffX, "X:%d", curX);
    writeText(buffX, textColor);
    render(750, 450, NULL, 0.0, NULL, SDL_FLIP_NONE);

    sprintf(buffY, "Y:%d", curY);
    writeText(buffY, textColor);
    render(750, 500, NULL, 0.0, NULL, SDL_FLIP_NONE); 
    
    sprintf(buffRows, "VRSTICE:%d", rows);
    writeText(buffRows, textColor);
    render(750, 100, NULL, 0.0, NULL, SDL_FLIP_NONE); 
    
    sprintf(buffColumns, "STOLPCI:%d", columns);
    writeText(buffColumns, textColor);
    render(750, 200, NULL, 0.0, NULL, SDL_FLIP_NONE);

    SDL_RenderPresent(renderer);
    cycleCounter++;
    oldtimestamp = timestamp;
    
    if(readVariableValue("I_1")==1)      /* check if start button is pressed */
    {
      start = 1;
    }
  }
  
  if(pageNumber == 1)                   /* check grid setup */
  {
    park_from_home();                   /* go from home to starting position */
    
    while(readVariableValue("I_1") == 0)  /*handle first spring*/
    {
       printf("WAITING FOR SPRING\n");
    }
    SDL_Delay(100);
    
    holes = rows * columns;
    if(rows%2==0 && columns%2==0)
    {
      printf("SODO\n");
      ss_grid();
    }
    else if(rows%2 == 0&& columns%2!=0)
    {
      printf("LIHO\n");
      ss_grid();
    }
    else if(rows%2!=0 && columns%2==0)
    {
      printf("SODO\n");
      ls_grid();
    }	    
    else
    {
      printf("LIHO\n");
      ll_grid();
    }
    start = 0;
    return 1;
  }
  SDL_RenderClear(renderer);
  start = 0;
  return 1;
}

void page_pass() /* setting up password page */
{
  draw();
  eventUpdate(); 
  keypad(400, 200, 100, 100);
  admin(945, 0, 75, 50, 1);
  SDL_RenderPresent(renderer);
  SDL_RenderClear(renderer);
  cycleCounter++;
  oldtimestamp = timestamp;
}

void page_select() /* setting up selection page */
{
  draw();
  eventUpdate();
  admin(945, 0, 75, 50, 1);
  button(200, 200, 300, 100, "SETTINGS", 4); 
  button(600, 200, 300, 100, "MANUAL MODE", 5); 
  SDL_RenderPresent(renderer);
  SDL_RenderClear(renderer);
  cycleCounter++;
  oldtimestamp = timestamp;
}

void page_settings() /*setting up settings page*/
{
  draw();
  eventUpdate();
  admin(945, 0, 75, 50, 3);

  initVars(50, 550, 15, 15);
  holes = rows * columns;
  drawEbGrid();
    
  admin(945, 0, 75, 50, 2);
    
  sprintf(buffRows, "VRSTICE:%d", rows);
  writeText(buffRows, textColor);
  render(700, 100, NULL, 0.0, NULL, SDL_FLIP_NONE); 
    
  sprintf(buffColumns, "STOLPCI:%d", columns);
  writeText(buffColumns, textColor);
  render(700, 200, NULL, 0.0, NULL, SDL_FLIP_NONE);


  plusRow();
  minusRow();
  plusColumn();
  minusColumn();

  button_save(650, 300, 300, 100);


  SDL_RenderPresent(renderer);
  SDL_RenderClear(renderer);
  cycleCounter++;
  oldtimestamp = timestamp;
}

void page_manual() /*setting up manual page*/
{
  draw();
  eventUpdate();
  admin(945, 0, 75, 50, 3);
  up_button(500, 100);
  down_button(500, 250);
  left_button(300, 250);
  right_button(700, 250);

  SDL_RenderPresent(renderer);
  SDL_RenderClear(renderer);
  cycleCounter++;
  oldtimestamp = timestamp;
}

void load_page(int pageNumber) /*handling loading pages */
{
  switch(pageNumber)
  {
    case 1:
      page_main();
      break;

    case 2:
      page_pass();
      break;
  
    case 3:
      page_select();
      break;

    case 4:
      page_settings();
      break;

    case 5:
      page_manual();
      break;
  }
}

int main()
{
  FILE *fp;  
  char *line;
  size_t len;
  int i;

  i = 0;
  line = NULL;
  len = 0;
     
  fp = fopen("/home/pi/move_program/param.txt", "r");
  
  printf("**************\n");
  printf("MOVE PROGRAM\n");


  for(i = 0; i < 2; ++i)
  {
    getline(&line, &len, fp);
    printf("%s", line);
    if(i == 0)
    {
      rows = atoi(line);
    }
    else
    {
      columns = atoi(line);	    
    }
  }
  pageNumber = 1;
  passText[0] = '\0';
  init();
  comm_init();
  program = 1;
 
  
  /*
  home();
  */
 
  while(program == 1)
  {
    load_page(pageNumber);
  }
  
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit(); 
  return 1;
}
