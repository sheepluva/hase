#define GRAVITY_DENSITY 32
#define GRAVITY_RESOLUTION 4
#define GRAVITY_PER_PIXEL 32
#define GRAVITY_PER_PIXEL_CORRECTION 16
#define GRAVITY_CIRCLE 16
#include "level.h"

typedef struct {
	Sint32 mass,x,y;
} tGravity;

SDL_Surface* gravity_surface;
tGravity* gravity;

void free_gravity()
{
	if (gravity)
		free(gravity);
	gravity = NULL;
}

void realloc_gravity()
{
	free_gravity();
	gravity = (tGravity*)malloc(sizeof(tGravity)*LEVEL_WIDTH*LEVEL_HEIGHT>>2*GRAVITY_RESOLUTION);
}

Sint32 calc_mass(Uint16* original,int x,int y)
{
	Sint32 mass = 0;
	int a,b;
	for (a = 0; a < (1<<GRAVITY_RESOLUTION); a++)	
		for (b = 0; b < (1<<GRAVITY_RESOLUTION); b++)
		{
			if (original[(x<<GRAVITY_RESOLUTION)+a+((y<<GRAVITY_RESOLUTION)+b)*LEVEL_WIDTH] != SP_ALPHA_COLOR )
				mass+=GRAVITY_PER_PIXEL;
		}
	return mass;
}

void impact_gravity(Sint32 mass,int x,int y)
{
	int a,b;
	int start_a = x-GRAVITY_CIRCLE;
	if (start_a < 0)
		start_a = 0;
	int end_a = x+GRAVITY_CIRCLE+1;
	if (end_a > (LEVEL_WIDTH>>GRAVITY_RESOLUTION))
		end_a = (LEVEL_WIDTH>>GRAVITY_RESOLUTION);
	for (a = start_a; a < end_a; a++)
	{
		int start_b = y-GRAVITY_CIRCLE;
		if (start_b < 0)
			start_b = 0;
		int end_b = y+GRAVITY_CIRCLE+1;
		if (end_b > (LEVEL_HEIGHT>>GRAVITY_RESOLUTION))
			end_b = (LEVEL_HEIGHT>>GRAVITY_RESOLUTION);
		for (b = start_b; b < end_b; b++)
		if (a!=x && b!=y && (a-x)*(a-x)+(b-y)*(b-y) <= GRAVITY_CIRCLE*GRAVITY_CIRCLE)
		{
			Sint32 dx = a-x;
			Sint32 dy = b-y;
			//Sint32 sum = spFixedToInt(spSqrt(spIntToFixed(dx*dx+dy*dy+1)));
			//Sint32 sum = spFixedToInt(spSqrt(spSqrt(spIntToFixed(dx*dx+dy*dy+1))));
			Sint32 sum = dx*dx+dy*dy;
			gravity[a+b*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].x += dx*mass/sum;
			gravity[a+b*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].y += dy*mass/sum;
		}
	}
}

void negate_gravity(int mx,int my,int r)
{
	SDL_LockSurface(level_original);
	int x,y;
	int start_x = mx-r;
	int end_x = mx+r;
	int start_y = my-r;
	int end_y = my+r;
	if (start_x < 0)
		start_x = 0;
	if (start_y < 0)
		start_y = 0;
	if (end_x > (LEVEL_WIDTH >> GRAVITY_RESOLUTION))
		end_x = (LEVEL_WIDTH >> GRAVITY_RESOLUTION);
	if (end_y > (LEVEL_HEIGHT >> GRAVITY_RESOLUTION))
		end_y = (LEVEL_HEIGHT >> GRAVITY_RESOLUTION);	
	for (x = start_x; x < end_x; x++)
		for (y = start_y; y < end_y; y++)
			if (gravity[x+y*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].mass)
			{
				int new_mass = calc_mass(level_pixel,x,y);
				int diff = new_mass-gravity[x+y*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].mass;
				if (diff)
				{
					impact_gravity(diff,x,y);
					gravity[x+y*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].mass = new_mass;
				}
			}
	SDL_UnlockSurface(level_original);	
}

void posivate_gravity(int mx,int my,int r)
{
	SDL_LockSurface(level_original);
	int x,y;
	int start_x = mx-r;
	int end_x = mx+r;
	int start_y = my-r;
	int end_y = my+r;
	if (start_x < 0)
		start_x = 0;
	if (start_y < 0)
		start_y = 0;
	if (end_x > (LEVEL_WIDTH >> GRAVITY_RESOLUTION))
		end_x = (LEVEL_WIDTH >> GRAVITY_RESOLUTION);
	if (end_y > (LEVEL_HEIGHT >> GRAVITY_RESOLUTION))
		end_y = (LEVEL_HEIGHT >> GRAVITY_RESOLUTION);	
	for (x = start_x; x < end_x; x++)
		for (y = start_y; y < end_y; y++)
			//if (gravity[x+y*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].mass)
			{
				int new_mass = calc_mass(level_pixel,x,y);
				int diff = new_mass-gravity[x+y*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].mass;
				if (diff)
				{
					impact_gravity(diff,x,y);
					gravity[x+y*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].mass = new_mass;
				}
			}
	SDL_UnlockSurface(level_original);	
}
void update_gravity()
{
	SDL_LockSurface(level_original);
	memset(gravity,0,sizeof(tGravity)*LEVEL_WIDTH*LEVEL_HEIGHT>>2*GRAVITY_RESOLUTION);
	int x,y;
	for (x = 0; x < (LEVEL_WIDTH>>GRAVITY_RESOLUTION); x++)
		for (y = 0; y < (LEVEL_HEIGHT>>GRAVITY_RESOLUTION); y++)
		{
			gravity[x+y*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].mass = calc_mass(level_pixel,x,y);
			if (gravity[x+y*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].mass)
				impact_gravity(gravity[x+y*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].mass,x,y);
		}
	SDL_UnlockSurface(level_original);	
}

#define BORDER_SIZE 8
#define BORDER_SQUARE 64

void init_gravity()
{
	int x,y,a,b;
	loadInformation("Calculating Gravity...");
	update_gravity();
	loadInformation("Drawing level borders...");
	spSelectRenderTarget(level_original);
	Uint16* pixel = spGetTargetPixel();
	int BORDER_COLOR = get_border_color();
	for (x = BORDER_SIZE; x < LEVEL_WIDTH-BORDER_SIZE; x++)
	{
		for (y = BORDER_SIZE; y < LEVEL_HEIGHT-BORDER_SIZE; y++)
		{
			if (pixel[x+y*LEVEL_WIDTH] != SP_ALPHA_COLOR &&
			   (pixel[ x-1 +y*LEVEL_WIDTH] == SP_ALPHA_COLOR ||
			    pixel[ x+1 +y*LEVEL_WIDTH] == SP_ALPHA_COLOR ||
			    pixel[x+(y+1)*LEVEL_WIDTH] == SP_ALPHA_COLOR ||
			    pixel[x+(y-1)*LEVEL_WIDTH] == SP_ALPHA_COLOR))
				for (a = -BORDER_SIZE; a < BORDER_SIZE-1; a++)
					for (b = -BORDER_SIZE; b < BORDER_SIZE-1; b++)
					{
						if (a*a+b*b > BORDER_SQUARE)
							continue;
						if (pixel[x+a+(y+b)*LEVEL_WIDTH] != SP_ALPHA_COLOR)
							pixel[x+a+(y+b)*LEVEL_WIDTH] = BORDER_COLOR;
					}
		}
	}
	spSelectRenderTarget(screen);
	loadInformation("Drawing level...");
	spSelectRenderTarget(level);
	spClearTarget(0);
	spSetAlphaTest(1);
	/*Uint16* pixel = (Uint16*)level->pixels;
	for (x = 0; x < LEVEL_WIDTH; x++)
		for (y = 0; y < LEVEL_HEIGHT; y++)
			pixel[x+y*LEVEL_WIDTH] = spGetRGB(gravitation_force(x,y)/2048,gravitation_force(x,y)/1024,gravitation_force(x,y)/512);*/
	//spSetBlending(SP_ONE*3/4);
	for (x = 0; x < (LEVEL_WIDTH>>GRAVITY_RESOLUTION); x++)
	{
		for (y = 0; y < (LEVEL_HEIGHT>>GRAVITY_RESOLUTION); y++)
		{
			Sint32 force = spSqrt(spMul(gravity[x+y*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].x,
			                      gravity[x+y*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].x)+
			                      spMul(gravity[x+y*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].y,
			                      gravity[x+y*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].y));			                      
			int f = GRAVITY_DENSITY-1-force / GRAVITY_PER_PIXEL_CORRECTION / (16384/GRAVITY_DENSITY);
			if (f < 0)
				f = 0;
			if (f == 31 && force != 0)
				f = 31;
			Sint32 angle = 0;
			if (force)
			{
				Sint32 ac = spDiv(gravity[x+y*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].y,force);
				if (ac < -SP_ONE)
					ac = -SP_ONE;
				if (ac > SP_ONE)
					ac = SP_ONE;
				angle = spAcos(ac)*(GRAVITY_DENSITY/2)/SP_PI;
			}
			if (gravity[x+y*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].x <= 0)
				angle = GRAVITY_DENSITY-1-angle;
			if (angle > GRAVITY_DENSITY-1)
				angle = GRAVITY_DENSITY-1;
			if (angle < 0)
				angle = 0;
			spBlitSurfacePart(x<<GRAVITY_RESOLUTION,y<<GRAVITY_RESOLUTION,0,
			                  gravity_surface,angle<<GRAVITY_RESOLUTION+1,f<<GRAVITY_RESOLUTION+1,1<<GRAVITY_RESOLUTION+1,1<<GRAVITY_RESOLUTION+1);
		}
	}
	spRectangleBorder(LEVEL_WIDTH/2,LEVEL_HEIGHT/2,0,LEVEL_WIDTH,LEVEL_HEIGHT,4,4,BORDER_COLOR);
	spBlitSurface(LEVEL_WIDTH/2,LEVEL_HEIGHT/2,0,level_original);
	spSelectRenderTarget(screen);
}

void fill_gravity_surface()
{
	spSelectRenderTarget(gravity_surface);
	spClearTarget( SP_ALPHA_COLOR );
	spBindTexture( arrow );
	int x,y;
	Sint32 h = spGetHFromColor(get_level_color());
	Sint32 s = spGetSFromColor(get_level_color());
	Sint32 v = spGetVFromColor(get_level_color());
	for (x = 0; x < GRAVITY_DENSITY; x++)
	{
		int angle = (GRAVITY_DENSITY-1-x)*SP_PI*2/GRAVITY_DENSITY;
		for (y = 0; y < GRAVITY_DENSITY; y++)
		{
			int V = (GRAVITY_DENSITY*2-1-y)*150/(GRAVITY_DENSITY*2);
			Uint16 color = spGetHSV(h,s,V);
			Sint32 d = 1 << SP_ACCURACY+GRAVITY_RESOLUTION;
			Sint32 D = d*(GRAVITY_DENSITY-1-y)/GRAVITY_DENSITY*3/5;
			int X = (x<<GRAVITY_RESOLUTION+1+SP_ACCURACY)+d;
			int Y = (y<<GRAVITY_RESOLUTION+1+SP_ACCURACY)+d;
			int x1 = spFixedToInt(X+spMul(spCos(angle),+D)-spMul(spSin(angle),+D));
			int y1 = spFixedToInt(Y+spMul(spSin(angle),+D)+spMul(spCos(angle),+D));
			int x2 = spFixedToInt(X+spMul(spCos(angle),+D)-spMul(spSin(angle),-D));
			int y2 = spFixedToInt(Y+spMul(spSin(angle),+D)+spMul(spCos(angle),-D));
			int x3 = spFixedToInt(X+spMul(spCos(angle),-D)-spMul(spSin(angle),-D));
			int y3 = spFixedToInt(Y+spMul(spSin(angle),-D)+spMul(spCos(angle),-D));
			int x4 = spFixedToInt(X+spMul(spCos(angle),-D)-spMul(spSin(angle),+D));
			int y4 = spFixedToInt(Y+spMul(spSin(angle),-D)+spMul(spCos(angle),+D));
			spQuad_tex( x1, y1,0,arrow->w-1,arrow->h-1,
			            x2, y2,0,arrow->w-1,         0,
			            x3, y3,0,         0,         0,
			            x4, y4,0,         0,arrow->h-1,color);
		}
	}
	spSelectRenderTarget(screen);
}
