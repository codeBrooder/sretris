/* sretris.c
 * created: 27.10.2013
 * version 0.1 - 17.11.2013 - playable tetris with following features:
 * pause; start again; block and background images rollback; side panel with nice control icons; scores with bonuses; speed levels;
 *
 *
 * TODO - highscores, options
 * TODO - visual WOWs
 * TODO - version 0.2 - sounds with rollback
 * TODO - level music
 * 
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>

typedef enum { running, stopped, paused, gameOver } gameState;
gameState state; 
typedef enum { none, down, right, left } blockMoveDirection;

typedef enum {false = 0, true = 1} bool;

void initNewGame( void );
void updateScreen( const blockMoveDirection move );
bool movePossible( const blockMoveDirection move );
void checkLine( void );
void gameStopped( void );
void blitPause( void );
void blitGameOver( void );
void blitScore ( void );
void blitSpeedLevel( void );
inline void printState( void );
inline bool blockInPlane( void );
void clean( void );

#define WIDTH 12
#define HEIGHT 16
const int width = WIDTH;
const int height = HEIGHT;
int empty[WIDTH][HEIGHT]; /* array representing currnet state of tiles - empty or with block */

const int blockPixelSize = 40;
const int iconPixelSize = 32;

int screenWidth;
int screenHeight;
const int colorDepth = 32;
const int sidePanelWidth = 150;

SDL_Renderer* sdlRenderer = NULL;
SDL_Window* screen = NULL;

SDL_Surface* background_s = NULL;
SDL_Texture* background = NULL;
SDL_Surface* block_s  = NULL;
SDL_Texture* block = NULL;

SDL_Surface* digits_s  = NULL;
SDL_Texture* digits = NULL;
SDL_Surface* keyEsc_s  = NULL;
SDL_Texture* keyEsc = NULL;
SDL_Surface* keyQ_s  = NULL;
SDL_Texture* keyQ = NULL;
SDL_Surface* iconExit_s  = NULL;
SDL_Texture* iconExit = NULL;
SDL_Surface* keyP_s  = NULL;
SDL_Texture* keyP = NULL;
SDL_Surface* iconPause_s  = NULL;
SDL_Texture* iconPause = NULL;
SDL_Surface* iconPlay_s  = NULL;
SDL_Texture* iconPlay = NULL;
SDL_Surface* keyS_s  = NULL;
SDL_Texture* keyS = NULL;

struct Pozition {
	int x,y;
};

struct Block4 {
	struct Pozition poz[4];
	int next;
};

#define BLOCK_VARIANTS 28
/* array with starting positions of tetris blocks */
const struct Block4 blocks[BLOCK_VARIANTS] = {
	/* some block positions are repeated to achive the same occuarence probablility */
	{5,0,6,0,5,-1,6,-1, 1},  /*         */
	{5,0,6,0,5,-1,6,-1, 2},  /*   ##    */
	{5,0,6,0,5,-1,6,-1, 3},  /*   ##    */
	{5,0,6,0,5,-1,6,-1, 0},  /*         * repeated 4 times */

	{4,0,5,0,6,0,7,0, 5},	 /*   #         */
	{6,0,6,-1,6,-2,6,-3, 6}, /*   #         */
	{4,0,5,0,6,0,7,0, 7},	 /*   #   ####  */
	{6,0,6,-1,6,-2,6,-3, 4}, /*   #         * repeated 2 times */

	{5,0,6,0,7,0,6,-1, 9},	  /*         #         #   */
	{6,0,6,-1,6,-2,5,-1, 10}, /*   #    ##   ###   ##  */
	{7,-1,6,-1,5,-1,6,0, 11}, /*  ###    #    #    #   */
	{5,-2,5,-1,5,0,6,-1, 8},  /*                       */

	{5,0,6,0,6,-1,6,-2, 13},  /*   #         ##       */
	{7,0,7,-1,6,-1,5,-1, 14}, /*   #   ###   #   #    */
	{6,-2,5,-2,5,-1,5,0, 15}, /*  ##     #   #   ###  */
	{5,-1,5,0,6,0,7,0, 12},   /*                      */

	{6,0,5,0,5,-1,5,-2, 17},  /*  #         ##        */
	{7,-1,7,0,6,0,5,0, 18},   /*  #     #    #   ###  */
	{5,-2,6,-2,6,-1,6,0, 19}, /*  ##  ###    #   #    */
	{5,0,5,-1,6,-1,7,-1, 16}, /*                      */

	{5,0,5,-1,6,-1,6,-2, 21}, /*    #         */
	{7,0,6,0,6,-1,5,-1, 22},  /*   ##   ##    */
	{5,0,5,-1,6,-1,6,-2, 23}, /*   #     ##   */
	{7,0,6,0,6,-1,5,-1, 20},  /*              * repated 2 times */

	{6,0,6,-1,5,-1,5,-2, 25}, /*   #          */
	{7,-1,6,-1,6,0,5,0, 26},  /*   ##    ##   */ 
	{6,0,6,-1,5,-1,5,-2, 27}, /*    #   ##    */
	{7,-1,6,-1,6,0,5,0, 24}   /*              * repated 2 times */

};

/* current active tetris block */
struct Block4 block4;
int blockVar;

/* step = auto step down = when block is stepping down without user interaction */
const int stepDelay = 500;		/* set wisely because pps should be >=1 */
const int eventPollDelay = 20; /* set wisely because pps should be >= 1 */
int pps; /* polls per step */
int polls2step; /* How many EventPolls remain to step down block */

unsigned int score;
int lastSuccesses;
int allLines;

int speedLevel;
int maxSpeedLevel;
const int linesPerSpeedLevel = 10; /* should be > 4 */
int linesToNextLevel;


int main(int argc, char** argv) {

	screenWidth = width * blockPixelSize;
	screenHeight = height * blockPixelSize;

	SDL_Init( SDL_INIT_VIDEO );
	if( SDL_CreateWindowAndRenderer(
		   screenWidth + sidePanelWidth, screenHeight,
		   SDL_WINDOW_SHOWN, &screen, &sdlRenderer)
	  )	{
			printf("Window or renderer creation failed - exiting.");
			return 1;
		}
		
	/* Load control icons images */
	keyEsc_s = SDL_LoadBMP( "images/Esc.bmp" );
	printf("keyEsc_s = %p \n", keyEsc_s );
	if(keyEsc_s == NULL) {
		printf( SDL_GetError() );
		printf( "\n" );
	}
	keyQ_s = SDL_LoadBMP( "images/Q.bmp" );
	printf("keyQ_s = %p \n", keyQ_s );
	if(keyQ_s == NULL) {
		printf( SDL_GetError() );
		printf( "\n" );
	}
	iconExit_s = SDL_LoadBMP( "images/exit.bmp" );
	keyP_s = SDL_LoadBMP( "images/P.bmp" );
	iconPause_s = SDL_LoadBMP( "images/pause.bmp" );
	iconPlay_s = SDL_LoadBMP( "images/play.bmp" );
	keyS_s = SDL_LoadBMP( "images/S.bmp" );
	digits_s = SDL_LoadBMP( "images/digits_green.bmp" );

	keyEsc = SDL_CreateTextureFromSurface( sdlRenderer, keyEsc_s );
	keyQ = SDL_CreateTextureFromSurface( sdlRenderer, keyQ_s );
	iconExit = SDL_CreateTextureFromSurface( sdlRenderer, iconExit_s );
	keyP = SDL_CreateTextureFromSurface( sdlRenderer, keyP_s );
	iconPause = SDL_CreateTextureFromSurface( sdlRenderer, iconPause_s );
	iconPlay = SDL_CreateTextureFromSurface( sdlRenderer, iconPlay_s );
	keyS = SDL_CreateTextureFromSurface( sdlRenderer, keyS_s );
	digits = SDL_CreateTextureFromSurface( sdlRenderer, digits_s );
	
	if(keyEsc == NULL || keyQ == NULL || iconExit == NULL || keyP == NULL || iconPause == NULL || iconPlay == NULL || keyS == NULL ||
			digits == NULL) {
		printf("Loading images Error - exiting\n");
		return 1;
	}
	
	/* load background and block images
	 * if loading fails there are default colors defined */
	if(	( background_s = SDL_LoadBMP( "images/vinales-valley.bmp" ) ) == NULL ) {
		background_s = SDL_CreateRGBSurface( SDL_SWSURFACE, screenWidth, screenHeight, colorDepth,0,0,0,0 );
		SDL_FillRect( background_s, NULL , 0x222222 );
	}
	background = SDL_CreateTextureFromSurface( sdlRenderer, background_s );
	if( ( block_s = SDL_LoadBMP( "images/kloc40x40.bmp" ) ) == NULL ) {
		block_s = SDL_CreateRGBSurface( SDL_SWSURFACE, blockPixelSize, blockPixelSize, colorDepth, 0,0,0,0 );
		SDL_FillRect( block_s, NULL , 0x000000 );
		SDL_Rect rect =  { 1,1, blockPixelSize-1, blockPixelSize-1 };
		SDL_FillRect( block_s, &rect, 0x30ff90 );
	}
	block = SDL_CreateTextureFromSurface( sdlRenderer, block_s );

	/* free all surfaces - now Textures will be used instead */
	SDL_FreeSurface( keyEsc_s );
	SDL_FreeSurface( keyQ_s );
	SDL_FreeSurface( iconExit_s );
	SDL_FreeSurface( keyP_s );
	SDL_FreeSurface( iconPause_s );
	SDL_FreeSurface( iconPlay_s );
	SDL_FreeSurface( keyS_s );
	SDL_FreeSurface( digits_s );

	SDL_FreeSurface( background_s );
	SDL_FreeSurface( block_s );

	srand ( (unsigned) time(NULL) ); /* randomize rand seed */

	SDL_Event event;
	/* No SDL_EnableKeyRepeat in SDL2. Leaving commented yet. */
	/* SDL_EnableKeyRepeat(1, 150); */

	initNewGame();

	while( state == running ) {

		SDL_Delay( eventPollDelay ); /* after stepping if necessary and polling and handling events -> wait */

		if( --polls2step == 0 ) { /* is it time to auto step down? if not goto event polling */
			polls2step = pps - speedLevel;
			if( movePossible( down ) ) { /* if block is not at bottom and next pozition is empty -> step down */
				int i;
				for(i=0; i<4; i++)
					++(block4.poz[i].y);
				updateScreen( down );
			}
			else if( blockInPlane() ) { /* not the highest raw and pozition under is not empty ->
					1)leave current block in place; 2)check full raw condition and remove raw(s) if needed; 3)place new block on "battlefield" */
				int i;
				for(i=0; i<4; i++)
					empty[block4.poz[i].x][block4.poz[i].y] = 0;

				checkLine();

				blockVar = rand() % BLOCK_VARIANTS;
				block4 = blocks[blockVar];
				updateScreen( none );
			}
			else { /* block is in highest raw and pozition under the block is not empty - game over */
				state = gameOver;
				gameStopped();
			}

			/*printState();*/
		}

		while( SDL_PollEvent( &event ) ) {
			switch( event.type ) {
				case SDL_KEYDOWN:
					switch( event.key.keysym.sym ) {
						case SDLK_ESCAPE: /* quit game */
						case SDLK_q:
							state = stopped;
							break;
						case SDLK_p: /* pause game */
							state = paused;
							gameStopped();
							break;
						case SDLK_UP: /* rotate block if possible */
							{
							int i, temp=0;
							struct Pozition deltaPoz[4];
							for(i=0; i<4; i++) {
								deltaPoz[i].x = blocks[block4.next].poz[i].x - blocks[blockVar].poz[i].x;
								deltaPoz[i].y = blocks[block4.next].poz[i].y - blocks[blockVar].poz[i].y;
									/* if not go outside left or right border */
								if( deltaPoz[i].x + block4.poz[i].x >= 0 && deltaPoz[i].x + block4.poz[i].x < width ) 
									if( deltaPoz[i].y + block4.poz[i].y < 0 ) /* if this part (tile) of block is above the screen do not check empty */
										temp++;
									else
										temp+=empty[deltaPoz[i].x + block4.poz[i].x][deltaPoz[i].y + block4.poz[i].y];
							}
							if(temp == 4) { /* possible to rotate */
								SDL_Rect tempRect;
								tempRect.w = tempRect.h = blockPixelSize;
								for(i=0; i<4; i++) {
									if( block4.poz[i].y >= 0 ) {
										tempRect.x = block4.poz[i].x * blockPixelSize;
										tempRect.y = block4.poz[i].y * blockPixelSize;
										SDL_RenderCopy( sdlRenderer, background, &tempRect, &tempRect );
									}
									block4.poz[i].x += deltaPoz[i].x;
									block4.poz[i].y += deltaPoz[i].y;
								}
								for(i=0; i<4; i++)
									if( block4.poz[i].y >= 0 ) {
										tempRect.x = block4.poz[i].x * blockPixelSize;
										tempRect.y = block4.poz[i].y * blockPixelSize;
										SDL_RenderCopy( sdlRenderer, block, NULL, &tempRect );
									}
								blockVar = block4.next;
								block4.next = blocks[block4.next].next;
								SDL_RenderPresent( sdlRenderer );
								/*printState();*/
							}
							}
							break;
						case SDLK_DOWN: /* the same procedure as auto step down */
							if( movePossible( down ) ) {
								int i;
								for(i=0; i<4; i++)
									++(block4.poz[i].y);
								updateScreen( down );
							}
							else if( blockInPlane() ) {
								int i;
								for(i=0; i<4; i++)
									empty[block4.poz[i].x][block4.poz[i].y] = 0;

								checkLine();

								blockVar = rand() % BLOCK_VARIANTS;
								block4 = blocks[blockVar];
								updateScreen( none );
							}
							else {
								state = gameOver;
								gameStopped(); /* game over */
							}

							/*printState();*/
							break;
						case SDLK_LEFT:
							if( movePossible( left ) ) {
								int i;
								for(i=0; i<4; i++)
									--(block4.poz[i].x);
								updateScreen( left );
								/*printState();*/
							}
							break;
						case SDLK_RIGHT:
							if( movePossible( right ) ) {
								int i;
								for(i=0; i<4; i++)
									++(block4.poz[i].x);
								updateScreen( right );
								/*printState();*/
							}
							break;
					}
					break;/*SDL_KEYDOWN*/
				case SDL_QUIT:
					state = stopped;
			} /*switch event.type*/
		}/*while pollEvent*/
	}/*while running*/

	/* clean before you go out the toilet */
	clean();

	SDL_Quit();

	return 0;
}

void initNewGame( void ) {

	speedLevel = 1;
	linesToNextLevel = linesPerSpeedLevel;
	score = allLines = lastSuccesses = 0;

	pps = stepDelay/eventPollDelay; /* polls per step */
	polls2step = pps - 1;

	maxSpeedLevel = pps - 10;

	int c,h;
	for(c=0; c<width; c++)
		for(h=0; h<height; h++)
			empty[c][h] = 1;

	blockVar = rand() % BLOCK_VARIANTS;
	block4 = blocks[ blockVar ];
	
	SDL_RenderCopy( sdlRenderer, background, NULL, NULL );
	SDL_Rect tempRect;
	tempRect.w = tempRect.h = blockPixelSize;
	for(c=0; c<4; c++) {
		tempRect.x = block4.poz[c].x * blockPixelSize;
		tempRect.y = block4.poz[c].y * blockPixelSize;
		SDL_RenderCopy( sdlRenderer, block, NULL, &tempRect );
	}
	
	SDL_Rect rect = { screenWidth, 0, sidePanelWidth, screenHeight };
	SDL_SetRenderDrawColor( sdlRenderer, 0x10, 0x10, 0x10, 0xFF );
	SDL_RenderFillRect( sdlRenderer , &rect ); /* side panel background */

	rect.w = rect.h = iconPixelSize;
	rect.x += 4;
	rect.y += screenHeight - 40;
	SDL_RenderCopy( sdlRenderer, keyEsc, NULL, &rect );

	rect.x += 36;
	SDL_RenderCopy( sdlRenderer, keyQ, NULL, &rect );

	rect.x += 40;
	SDL_RenderCopy( sdlRenderer, iconExit, NULL, &rect );

	rect.x -= 76;
	rect.y -= 40;
	SDL_RenderCopy( sdlRenderer, keyP, NULL, &rect );

	rect.x += 40;
	SDL_RenderCopy( sdlRenderer, iconPause, NULL, &rect );

	blitScore();
	blitSpeedLevel();

	SDL_RenderPresent( sdlRenderer );

	state = running;
}

/* if active block changed pozition it is necessary to redraw some parts of background
 */
void updateScreen( const blockMoveDirection move ) {
	SDL_Rect rect;
	rect.w = rect.h = blockPixelSize;
	int i;

	if( move == down ) {
		for(i=0; i<4; i++) {
			rect.y = ( block4.poz[i].y - 1 ) * blockPixelSize;
			if( rect.y >= 0 ) {
				rect.x = block4.poz[i].x * blockPixelSize;
				SDL_RenderCopy( sdlRenderer, background, &rect, &rect );
			}
		}
	}
	else if( move == left ) {
		for(i=0; i<4; i++) {
			rect.y = block4.poz[i].y * blockPixelSize;
			if( rect.y >= 0 ) {
				rect.x = ( block4.poz[i].x + 1 ) * blockPixelSize;
				SDL_RenderCopy( sdlRenderer, background, &rect, &rect );
			}
		}
	}
	else if( move == right ) {
		for(i=0; i<4; i++) {
			rect.y = block4.poz[i].y * blockPixelSize;
			if( rect.y >= 0 ) {
				rect.x = ( block4.poz[i].x - 1 ) * blockPixelSize;
				SDL_RenderCopy( sdlRenderer, background, &rect, &rect );
			}
		}
	}

	for(i=0; i<4; i++) {
		rect.y = block4.poz[i].y * blockPixelSize;
		if( rect.y >= 0 ) {
			rect.x = block4.poz[i].x * blockPixelSize;
			SDL_RenderCopy( sdlRenderer, block, NULL, &rect );
		}
	}

	SDL_RenderPresent( sdlRenderer );
}

/* function called when pause or game over occured */
void gameStopped( void ) {

	/**
	 * ********************************
	 * ********************************
	 */
	printf("gameStopped score=%d\n", score);

	if( state == paused )
		blitPause();
	else
		blitGameOver();

	SDL_RenderPresent( sdlRenderer );
	SDL_Event event;

	bool runWhile = true;
	while ( runWhile ) {

		SDL_Delay(100); /* keyboard lag is not critical when paused or game over*/ /* or SDL_Delay( eventPollDelay ); */

		while ( SDL_PollEvent( &event ) )
			if( event.type == SDL_QUIT || event.type == SDL_KEYDOWN && (
						event.key.keysym.sym == SDLK_q || event.key.keysym.sym == SDLK_ESCAPE /* quit game */
							|| (event.key.keysym.sym == SDLK_p && state == paused) /* resume from paused stated */
							|| (event.key.keysym.sym == SDLK_s && state == gameOver) /* start new game from gameOver state */
						)
			  ) {
				runWhile = false;
				break; /* must be break, so while(PollEvent) do not set event to other than SDL_KEYDOWN
						  remember that next events in queue will be handled in main loop */
			}
	}
	
	/* if game was resumed redraw etire screen */
	if( state == paused && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_p ) {
		int i,j;
		SDL_Rect rect;
		rect.w = rect.h = blockPixelSize;

		for(j=0; j<height; j++)
			for(i=0; i<width; i++) {
				rect.x = i*blockPixelSize;
				rect.y = j*blockPixelSize;
				if( empty[i][j] )
					SDL_RenderCopy( sdlRenderer, background, &rect, &rect );
				else
					SDL_RenderCopy( sdlRenderer, block, NULL, &rect );
			}
		for(i=0; i<4; i++)
			if( block4.poz[i].y >=0 ) {
				rect.x = block4.poz[i].x * blockPixelSize;
				rect.y = block4.poz[i].y * blockPixelSize;
				SDL_RenderCopy( sdlRenderer, block, NULL, &rect );
			}

		/* draw pause icon instead of play icon in side panel */
		rect.w = rect.h = iconPixelSize;
		rect.x = screenWidth + 44;
		rect.y = screenHeight - 80;
		SDL_RenderCopy( sdlRenderer, iconPause, NULL, &rect );

		SDL_RenderPresent( sdlRenderer );
		state = running;
	}
	else if( state == gameOver && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_s )
		initNewGame();
	else
		state = stopped; /* if user decided to quit, instruct main loop to quit the game */
}

void blitScore ( void ) {
	char scoreStr[16];
	int numDigits = snprintf(scoreStr, 32, "%d", score );
	
	SDL_Rect rect, rectDst;
	rect.y = rectDst.y = 0;
	rect.w = rectDst.w = 20;
	rect.h = rectDst.h = 40;

	int i = 0;
	for( i=numDigits-1; i>=0; i--) {
		rect.x = ( ( (int) scoreStr[i] ) - 48 ) * 20;
		rectDst.x = screenWidth + sidePanelWidth - (numDigits - i)*20;
		SDL_RenderCopy( sdlRenderer, digits, &rect, &rectDst );
	}
}

void blitSpeedLevel( void ) {
	char speedLevelStr[4];
	int numDigits = snprintf(speedLevelStr, 4, "%d", speedLevel );

	SDL_Rect rect, rectDst;
	rectDst.y = 50;
	rect.y = 0;
	rect.w = rectDst.w = 20;
	rect.h = rectDst.h = 40;

	int i;
	for( i=0; i<numDigits; i++ ) {
		rect.x = ( ( (int) speedLevelStr[i] ) - 48 ) * 20;
		rectDst.x = screenWidth + (numDigits + i)*20 + 1;
		SDL_RenderCopy( sdlRenderer, digits, &rect, &rectDst );
	}

}

void blitGameOver( void ) {
	
	SDL_Rect rect = {screenWidth/2 - blockPixelSize*2.5 + blockPixelSize/4,
		screenHeight/2 - blockPixelSize*3 + blockPixelSize/4,
		blockPixelSize*5, blockPixelSize*5};
	SDL_SetRenderDrawColor( sdlRenderer, 0x10, 0x10, 0x10, 0xFF );
	SDL_RenderFillRect( sdlRenderer, &rect );

	rect.x -= blockPixelSize/4;
	rect.y -= blockPixelSize/4;
	SDL_SetRenderDrawColor( sdlRenderer, 0x00, 0x00, 0x88, 0xFF );
	SDL_RenderFillRect( sdlRenderer, &rect );

	rect.x += 2;
	rect.y += 2;
	rect.w -= 4;
	rect.h -= 4;
	SDL_SetRenderDrawColor( sdlRenderer, 0x99, 0xAA, 0xAA, 0xFF );
	SDL_RenderFillRect( sdlRenderer, &rect );

	rect.x = screenWidth/2 - blockPixelSize*0.5;
	rect.y = screenHeight/2 - blockPixelSize*2.5;
	rect.w = blockPixelSize;
	rect.h = blockPixelSize*2.5;
	SDL_SetRenderDrawColor( sdlRenderer, 0xAA, 0x00, 0x00, 0xFF );
	SDL_RenderFillRect( sdlRenderer, &rect );

	rect.y += blockPixelSize*3;
	rect.h = blockPixelSize;
	SDL_RenderFillRect( sdlRenderer, &rect );

	/* draw "start new game" control instead of pause control */
	rect.w = rect.h = iconPixelSize;
	rect.y = screenHeight - 80;
	rect.x = screenWidth + 4;
	SDL_RenderCopy( sdlRenderer, keyS, NULL, &rect );
	rect.x += 40;
	SDL_RenderCopy( sdlRenderer, iconPlay, NULL, &rect );

}

void blitPause( void ) {

	SDL_Rect rect = {screenWidth/2 - blockPixelSize*2.5 + blockPixelSize/4,
		screenHeight/2 - blockPixelSize*3 + blockPixelSize/4,
		blockPixelSize*5, blockPixelSize*5};
	SDL_SetRenderDrawColor( sdlRenderer, 0x10, 0x10, 0x10, 0xFF );
	SDL_RenderFillRect( sdlRenderer, &rect );

	rect.x -= blockPixelSize/4;
	rect.y -= blockPixelSize/4;
	SDL_SetRenderDrawColor( sdlRenderer, 0x00, 0x00, 0x88, 0xFF );
	SDL_RenderFillRect( sdlRenderer, &rect );

	rect.x += 2;
	rect.y += 2;
	rect.w -= 4;
	rect.h -= 4;
	SDL_SetRenderDrawColor( sdlRenderer, 0xAA, 0xAA, 0xAA, 0xFF );
	SDL_RenderFillRect( sdlRenderer, &rect );

	rect.x = screenWidth/2 - blockPixelSize*1.5;
	rect.y = screenHeight/2 - blockPixelSize*2;
	rect.w = blockPixelSize;
	rect.h = blockPixelSize*3;

	SDL_SetRenderDrawColor( sdlRenderer, 0x00, 0x00, 0x88, 0xFF );
	SDL_RenderFillRect( sdlRenderer, &rect );

	rect.x = screenWidth/2 + blockPixelSize/2;
	SDL_RenderFillRect( sdlRenderer, &rect );

	/* draw play icon instead of pause icon in controls panel */
	rect.w = rect.h = iconPixelSize;
	rect.x = screenWidth + 44;
	rect.y = screenHeight - 80;
	SDL_RenderCopy( sdlRenderer, iconPlay, NULL, &rect );

}

/* checks if there is full row and deletes the rows */
void checkLine( void ) {
	int i,j,k;

	/* check if there are full rows */
	int temp[height-1];
	int fullRows = 0;
	for(j=1; j<height; j++) {
		temp[j-1] = 0;
		for(i=0; i<width; i++) {
			temp[j-1]+=empty[i][j];
		}
		if( temp[j-1] == 0)
			fullRows++;
	}

	/* if at least 1 row is full */
	if( fullRows ) {

		allLines += fullRows;

		/* copy  the state of all tiles */
		int oldEmpty[width][height];
		for(j=0; j<height; j++)
			for(i=0; i<width; i++)
				oldEmpty[i][j] = empty[i][j];

		/* delete full rows and fall down all above */
		for(j=1; j<height; j++)
			if( temp[j-1] == 0 )
				for(i=0; i<width; i++)
					for(k=j; k>0; k--)
						empty[i][k]=empty[i][k-1];

		/* redraw changed tiles */
		SDL_Rect rect;
		rect.w = rect.h = blockPixelSize;
		for(j=1; j<height; j++)
			for(i=0; i<width; i++)
				if( oldEmpty[i][j] != empty[i][j] ) {
					rect.x = i*blockPixelSize;
					rect.y = j*blockPixelSize;
					if( empty[i][j] )
						SDL_RenderCopy( sdlRenderer, background, &rect, &rect );
					else
						SDL_RenderCopy( sdlRenderer, block, NULL, &rect );
				}

		score += fullRows*fullRows*10*(lastSuccesses+1); /* lastSuccesses: full line one after another block bonus */
		printf("score=%d   (+%d) (bonus *%d)   lines=%d\n", score, fullRows*fullRows*10, lastSuccesses+1, allLines);
		lastSuccesses++;

		blitScore();
		/* speed level handling */
		linesToNextLevel -= fullRows;
		if(linesToNextLevel <= 0 ) {
			if( speedLevel < maxSpeedLevel ) {
				linesToNextLevel += linesPerSpeedLevel;
				speedLevel++;
				blitSpeedLevel();
			}
		}
		SDL_RenderPresent( sdlRenderer );
	}
	else
		lastSuccesses = 0;

}

/* returns true if active block can move in specified direction */
bool movePossible( const blockMoveDirection move ) {
	int temp=0, i;

	if( move == down )
		for(i=0; i<4; i++) {
			if( block4.poz[i].y < height - 1 ) /* if pozition of mini block is not at the bottom */
				if(block4.poz[i].y < -1)
					temp++;
				else
					temp += empty[ block4.poz[i].x ][ block4.poz[i].y + 1 ];
		}
	else if( move == left )
		for(i=0; i<4; i++) {
			if ( block4.poz[i].x ) /* if pozition of mini block is not near left wall */
				if(block4.poz[i].y < 0)
					temp++;
				else
					temp += empty[block4.poz[i].x - 1][block4.poz[i].y];
		}
	else if( move == right )
		for(i=0; i<4; i++) {
			if( block4.poz[i].x < width - 1 ) /* if pozition of mini block is not near right wall */
				if(block4.poz[i].y < 0)
					temp++;
				else
					temp += empty[block4.poz[i].x + 1][block4.poz[i].y];
		}

	if( temp == 4 )
		return true;
	return false;
}

/* returns true if none of the part of active block is in the highest row or above the screen
 * In this game if block stops and at least 1 tile of the highest visible row is not empty game is over
 */
inline bool blockInPlane( void ) {
	int temp = 0, i;
	for(i=0; i<4; i++)
		if(block4.poz[i].y < 1)
			temp++;
	return !temp;
}

void clean( void ) {
	SDL_DestroyTexture( keyS );
	SDL_DestroyTexture( keyQ );
	SDL_DestroyTexture( keyP );
	SDL_DestroyTexture( keyEsc );
	SDL_DestroyTexture( iconPause );
	SDL_DestroyTexture( iconPlay );
	SDL_DestroyTexture( iconExit );
	SDL_DestroyTexture( digits );

	SDL_DestroyTexture( block );
	SDL_DestroyTexture( background );

	keyS = keyQ = keyP = keyEsc = iconPause = iconPlay = iconExit = digits = block = background = NULL;

	SDL_DestroyRenderer( sdlRenderer );
	SDL_DestroyWindow( screen );
	sdlRenderer = NULL;
	screen = NULL;
}

/* prints to console state of the game in nice format
 * can be used to play game in console :) cli rulezZZ... :-zZZZZ
 */
void printState( void ) {
	int i,j;
	printf("________________________\n");
	for(j=0; j<height; j++) {
		printf("|");
		for(i=0; i<width; i++) {
			if(block4.poz[0].x == i && block4.poz[0].y == j ||
				block4.poz[1].x == i && block4.poz[1].y == j ||
				block4.poz[2].x == i && block4.poz[2].y == j ||
				block4.poz[3].x == i && block4.poz[3].y == j )
				printf("X");
			else if ( empty[i][j] )
				printf(".");
			else
				printf("#");
		}
		printf("|\n");
	}
	printf("~~~~~~~~~~~~~~~~~~~~~~~~\n");
}

