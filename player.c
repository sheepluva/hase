#define HOPS_TIME 200
#define HIGH_HOPS_TIME 403
#define MAX_HEALTH 100
#define AI_MAX_TRIES 96
#define AI_TRIES_EVERY_MS 32
int ai_shoot_tries = 0;
int last_ai_try = 0;

#include "window.h"
#include <math.h>

int lastAIDistance = 100000000;

int active_player = 0;
int player_count;
pPlayer *player;

void update_targeting()
{
	spSpritePointer sprite = spActiveSprite(targeting);
	int nr = 0;
	spSubSpritePointer mom = sprite->firstSub;
	while (mom != sprite->momSub)
	{
		nr++;
		mom = mom->next;
	}
	Sint32 subPosition = sprite->momSub->age * SP_ONE / sprite->momSub->duration;
	Sint32 divisor = spMax(SP_ONE/128,player[active_player]->activeHare->w_power);
	int new_duration = spDiv(SP_ONE,divisor) >> SP_ACCURACY-5;
	sprite->momSub->age = spDiv(subPosition,divisor) >> SP_ACCURACY-5;
	sprite->wholeAge = nr * new_duration + sprite->momSub->age;
	nr = 0;
	mom = sprite->firstSub;
	do
	{
		mom->duration = new_duration;
		nr++;
		mom = mom->next;
	}
	while (mom != sprite->firstSub);
	sprite->wholeDuration = nr * new_duration;	
}

static int circle_is_empty(int x, int y, int r,pHare except,int with_players)
{
	if (with_players != -1)
	{
		int a,b;
		int start_a = x-r;
		if (start_a < 0)
			start_a = 0;
		int end_a = x+r+1;
		if (end_a > LEVEL_WIDTH)
			end_a = LEVEL_WIDTH;
		int start_b = y-r;
		if (start_b < 0)
			start_b = 0;
		int end_b = y+r+1;
		if (end_b > LEVEL_HEIGHT)
			end_b = LEVEL_HEIGHT;
		for (a = start_a; a < end_a; a++)
			for (b = start_b; b < end_b; b++)
			if ((a-x)*(a-x)+(b-y)*(b-y) <= r*r)
			{
				if (level_pixel[a+b*LEVEL_WIDTH] != SP_ALPHA_COLOR)
					return 0;
			}
	}
	
	int i;
	if (with_players)
		for (i = 0; i < player_count; i++)
		{
			pHare hare = player[i]->firstHare;
			if (hare)
			do
			{
				if (hare == except)
				{
					hare = hare->next;
					continue;
				}
				int px = hare->x >> SP_ACCURACY;
				int py = hare->y >> SP_ACCURACY;
				int d = (x-px)*(x-px)+(y-py)*(y-py);
				if (d <= (r+PLAYER_PLAYER_RADIUS)*(r+PLAYER_PLAYER_RADIUS))
					return 0;
				hare = hare->next;
			}
			while (hare != player[i]->firstHare);
		}
	return 1;
}

static int pixel_is_empty(int x, int y)
{
	if (x < 0)
		return 1;
	if (x >= LEVEL_WIDTH)
		return 1;
	if (y < 0)
		return 1;
	if (y >= LEVEL_WIDTH)
		return 1;
	if (level_pixel[x+y*LEVEL_WIDTH] != SP_ALPHA_COLOR)
		return 0;
	return 1;
}

static Sint32 gravitation_x(int x,int y)
{
	int gx1 = x - (1 << GRAVITY_RESOLUTION - 1) >> GRAVITY_RESOLUTION;
	int gy1 = y - (1 << GRAVITY_RESOLUTION - 1) >> GRAVITY_RESOLUTION;
	int gx2 = gx1+1;
	int gy2 = gy1+1;
	int rx = x - (gx1 << GRAVITY_RESOLUTION);
	int ry = y - (gy1 << GRAVITY_RESOLUTION);
	if (gx1 < 0 || gy1 < 0 || gx1 >= (LEVEL_WIDTH >> GRAVITY_RESOLUTION) || gy1 >= (LEVEL_HEIGHT >> GRAVITY_RESOLUTION))
		return 0;
	if (gx2 < 0 || gy2 < 0 || gx2 >= (LEVEL_WIDTH >> GRAVITY_RESOLUTION) || gy2 >= (LEVEL_HEIGHT >> GRAVITY_RESOLUTION))
		return 0;
	int g1 = gravity[gx2+gy1*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].x * rx + gravity[gx1+gy1*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].x * ((1<<GRAVITY_RESOLUTION)-rx) >> GRAVITY_RESOLUTION;
	int g2 = gravity[gx2+gy2*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].x * rx + gravity[gx1+gy2*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].x * ((1<<GRAVITY_RESOLUTION)-rx) >> GRAVITY_RESOLUTION;
	return g2 * ry + g1 * ((1<<GRAVITY_RESOLUTION)-ry) >> GRAVITY_RESOLUTION;
}

static Sint32 gravitation_y(int x,int y)
{
	int gx1 = x - (1 << GRAVITY_RESOLUTION - 1) >> GRAVITY_RESOLUTION;
	int gy1 = y - (1 << GRAVITY_RESOLUTION - 1) >> GRAVITY_RESOLUTION;
	int gx2 = gx1+1;
	int gy2 = gy1+1;
	int rx = x - (gx1 << GRAVITY_RESOLUTION);
	int ry = y - (gy1 << GRAVITY_RESOLUTION);
	if (gx1 < 0 || gy1 < 0 || gx1 >= (LEVEL_WIDTH >> GRAVITY_RESOLUTION) || gy1 >= (LEVEL_HEIGHT >> GRAVITY_RESOLUTION))
		return 0;
	if (gx2 < 0 || gy2 < 0 || gx2 >= (LEVEL_WIDTH >> GRAVITY_RESOLUTION) || gy2 >= (LEVEL_HEIGHT >> GRAVITY_RESOLUTION))
		return 0;
	int g1 = gravity[gx2+gy1*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].y * rx + gravity[gx1+gy1*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].y * ((1<<GRAVITY_RESOLUTION)-rx) >> GRAVITY_RESOLUTION;
	int g2 = gravity[gx2+gy2*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].y * rx + gravity[gx1+gy2*(LEVEL_WIDTH>>GRAVITY_RESOLUTION)].y * ((1<<GRAVITY_RESOLUTION)-rx) >> GRAVITY_RESOLUTION;
	return g2 * ry + g1 * ((1<<GRAVITY_RESOLUTION)-ry) >> GRAVITY_RESOLUTION;
}

static Sint32 gravitation_force(int x,int y)
{
	int grav_x = gravitation_x(x,y);
	int grav_y = gravitation_y(x,y);
	if (grav_x == 0 && grav_y == 0)
		return 0;
	return spSqrt(spMul(grav_x,grav_x)+spMul(grav_y,grav_y));
}

void update_player()
{
	int j;
	for (j = 0; j < player_count; j++)
	{
		pHare hare = player[j]->firstHare;
		if (hare)
		do
		{
			if (hare->hops > 0)
			{
				hare->hops --;
				if (hare->hops <= 0)
				{
					Sint32 dx = spSin(hare->rotation);
					Sint32 dy = spCos(hare->rotation);
					Sint32 ox = hare->x;
					Sint32 oy = hare->y;
					int k;
					for (k = 1; k <= 16; k++)
					{
						if (circle_is_empty(spFixedToInt(hare->x+k*dx),spFixedToInt(hare->y-k*dy),PLAYER_RADIUS,hare,1))
						{
							hare->x += k*dx;
							hare->y -= k*dy;
							Sint32 angle = SP_PI/3;
							Sint32 divisor = 8;
							if (hare->high_hops)
							{
								angle = SP_PI/10;
								if (hare->high_hops >= 2) //super jump!
								{
									divisor = 4;
									if (hare->high_hops == 3) //Wingdaium
										angle = 0;
								}
								else
									divisor = 6;
							}
							if (hare->direction)
							{
								hare->dx += spSin(hare->rotation+angle)/divisor;
								hare->dy -= spCos(hare->rotation+angle)/divisor;
							}
							else
							{
								hare->dx += spSin(hare->rotation-angle)/divisor;
								hare->dy -= spCos(hare->rotation-angle)/divisor;
							}
							break;
						}
						if (k == 16)
						{
							k = -1;
							printf("Using special magic...\n");
						}
					}
					if (hare->high_hops)
						spSoundPlay(snd_high,-1,0,0,-1);
					else
						spSoundPlay(snd_low,-1,0,0,-1);
				}
			}
			hare->rotation = 0;
			hare->bums = 0;
			int force = gravitation_force(spFixedToInt(hare->x),spFixedToInt(hare->y));
			if (force)
			{
				Sint32 ac = spDiv(-gravitation_y(spFixedToInt(hare->x),spFixedToInt(hare->y)),force);
				if (ac < -SP_ONE)
					ac = -SP_ONE;
				if (ac > SP_ONE)
					ac = SP_ONE;
				hare->rotation = -spAcos(ac);
				if (-gravitation_x(spFixedToInt(hare->x),spFixedToInt(hare->y)) <= 0)
					hare->rotation = 2*SP_PI-hare->rotation;
				while (hare->rotation < 0)
					hare->rotation += 2*SP_PI;
				while (hare->rotation >= 2*SP_PI)
					hare->rotation -= 2*SP_PI;
				if (force > 1024)
					hare->cam_rotation = hare->rotation;
			}
			hare = hare->next;
		}
		while (hare != player[j]->firstHare);
	}
}

int hare_explosion_feedback( spParticleBunchPointer bunch, Sint32 action, Sint32 extra_data)
{
	int particleSize = spMax(0,zoom*gop_particles() >> SP_ACCURACY);
	if (action == SP_PARTICLE_UPDATE)
	{
		if (bunch->age > 10000)
			return 1;
		int i;
		int touched = 0;
		for (i = 0; i < bunch->count; i++)
			if (bunch->particle[i].status >= 0)
			{
				touched = 1;
				bunch->particle[i].dx -= gravitation_x(bunch->particle[i].x >> SP_ACCURACY,bunch->particle[i].y >> SP_ACCURACY) * extra_data >> PHYSIC_IMPACT;
				bunch->particle[i].dy -= gravitation_y(bunch->particle[i].x >> SP_ACCURACY,bunch->particle[i].y >> SP_ACCURACY) * extra_data >> PHYSIC_IMPACT;
				if (pixel_is_empty(bunch->particle[i].x+bunch->particle[i].dx * extra_data >> SP_ACCURACY,bunch->particle[i].y+bunch->particle[i].dy * extra_data >> SP_ACCURACY))
				{
					bunch->particle[i].x += bunch->particle[i].dx * extra_data;
					bunch->particle[i].y += bunch->particle[i].dy * extra_data;
				}
				else
				{
					//bounce!
					//Sint32 speed = spSqrt(spSquare(bunch->particle[i].dx)+spSquare(bunch->particle[i].dy));
					Sint32 speed = vector_length_approx(bunch->particle[i].dx,bunch->particle[i].dy);
					/*float l_1 = spFixedToFloat(speed);
					float l_2 = spFixedToFloat(vector_length_approx(bunch->particle[i].dx,bunch->particle[i].dy));
					float l_3 = spFixedToFloat(vector_length_guess(bunch->particle[i].dx,bunch->particle[i].dy));
					printf("Real: %.4f Approx: %.4f (+-%.2f) Guess: %.4f (+-%.2f)\n",l_1,l_2,fabs((l_1-l_2)/l_1)*100.0f,l_3,fabs((l_1-l_3)/l_1)*100.0f);*/
					if (speed <= (SP_ONE >> 4))
						bunch->particle[i].status = -1;
					else
					{
						Sint32 ax = spDiv(bunch->particle[i].dx,speed);
						Sint32 ay = spDiv(bunch->particle[i].dy,speed);
						Sint32 ex = gravitation_x(bunch->particle[i].x >> SP_ACCURACY,bunch->particle[i].y >> SP_ACCURACY);
						Sint32 ey = gravitation_y(bunch->particle[i].x >> SP_ACCURACY,bunch->particle[i].y >> SP_ACCURACY);
						//Sint32 len = spSqrt(spSquare(ex)+spSquare(ey));
						Sint32 len = vector_length_approx(ex,ey);
						if (len == 0)
							bunch->particle[i].status = -1;
						else
						{
							ex = spDiv(ex,len);
							ey = spDiv(ey,len);
							Sint32 p = -2*(spMul(ex,ax)+spMul(ey,ay));
							bunch->particle[i].dx = spMul(p,ex)+ax;
							bunch->particle[i].dy = spMul(p,ey)+ay;
							bunch->particle[i].dx = spMul(bunch->particle[i].dx,speed*3/4);
							bunch->particle[i].dy = spMul(bunch->particle[i].dy,speed*3/4);
						}
					}
				}
			}
		if (touched == 0)
			return 1;
	}
	if (action == SP_PARTICLE_DRAW)
	{
		if (bunch->age > 9000)
			spSetBlending(SP_ONE*(10000-bunch->age)/1000);
		int i;
		if (particleSize == 0)
		{
			for (i = 0; i < bunch->count; i++)
				if (bunch->particle[i].status >= 0)
				{
					Sint32 ox = spMul(bunch->particle[i].x-posX,zoom);
					Sint32 oy = spMul(bunch->particle[i].y-posY,zoom);
					Sint32	x = spMul(ox,spCos(rotation))-spMul(oy,spSin(rotation)) >> SP_ACCURACY;
					Sint32	y = spMul(ox,spSin(rotation))+spMul(oy,spCos(rotation)) >> SP_ACCURACY;
					spRectangle(screen->w/2+x,screen->h/2+y,0,0,0,bunch->particle[i].data.color);
					
				}
		}
		else
		for (i = 0; i < bunch->count; i++)
			if (bunch->particle[i].status >= 0)
			{
				Sint32 ox = spMul(bunch->particle[i].x-posX,zoom);
				Sint32 oy = spMul(bunch->particle[i].y-posY,zoom);
				Sint32	x = spMul(ox,spCos(rotation))-spMul(oy,spSin(rotation)) >> SP_ACCURACY;
				Sint32	y = spMul(ox,spSin(rotation))+spMul(oy,spCos(rotation)) >> SP_ACCURACY;
				spEllipse(screen->w/2+x,screen->h/2+y,0,particleSize,particleSize,bunch->particle[i].data.color);
			}
		spSetBlending(SP_ONE);
	}
	return 0;
}

void update_player_sprite(int steps)
{
	int j;
	for (j = 0; j < player_count; j++)
	{
		pHare hare = player[j]->firstHare;
		if (hare)
		{
			int count = 0;
			do
			{
				if (hare->hops > 0)
				{
					if (hare->high_hops)
					{
						if (hare->direction == 0)
							spSelectSprite(hare->hase,"high jump left");
						else
							spSelectSprite(hare->hase,"high jump right");
					}
					else
					{
						if (hare->direction == 0)
							spSelectSprite(hare->hase,"jump left");
						else
							spSelectSprite(hare->hase,"jump right");
					}
				}
				else
				{
					if (hare->direction == 0)
						spSelectSprite(hare->hase,"stand left");
					else
						spSelectSprite(hare->hase,"stand right");
				}
				spUpdateSprite(spActiveSprite(hare->hase),steps);
				count++;
				hare = hare->next;
			}
			while (hare != player[j]->firstHare);
			
			if (player[j]->next_round_extra && gop_particles() != 4)
			{
				int c = ((4-gop_particles()) * count * steps / 10);
				spParticleBunchPointer p = spParticleCreate(c,hare_explosion_feedback,&particles);
				p->age = 9000;
				int i;
				for (i = 0; i < c; i++)
				{
					int a = (int)((rand() & 32767) * 16) % (2*SP_PI);
					p->particle[i].dx = spSin(a)/8;
					p->particle[i].dy = spCos(a)/8;
					p->particle[i].dz = 0;
					p->particle[i].x = hare->x+p->particle[i].dx*8*8;
					p->particle[i].y = hare->y+p->particle[i].dy*8*8;
					p->particle[i].z = 0;
					p->particle[i].data.color = spGetFastRGB(255,0,0);
					p->particle[i].status = 0;
					hare = hare->next;
				}
			}
		}
	}
}	

int next_player_go = 0;

void next_player()
{
	next_player_go = 1;
}

void stop_thread(int kill)
{
	if (!hase_game->local && active_player >= 0)
	{
		if (!player[active_player]->computer)
		{
			if (player[active_player]->local)
			{
				printf("Ending Push Thread for player %s (s: %i)\n",player[active_player]->name,player[active_player]->time/1000);
				char buffer[320];
				sprintf(buffer,"Finishing sending turn data\nfor player %s...",player[active_player]->name);
				set_message(font,buffer);
				draw_message();
				spFlip();
				push_game_thread(player[active_player],player[active_player]->time/1000,send_data);
				memset(send_data,0,sizeof(char)*1536);
				end_push_thread(kill);
				spResetLoop();
			}
			else
			{
				printf("Ending Pull Thread for player %s\n",player[active_player]->name);
				end_pull_thread(player[active_player]);
			}
		}
		printf("Setting time of %s from %i to %i\n",player[active_player]->name,player[active_player]->time,((player[active_player]->time+999)/1000)*1000);
		player[active_player]->time = ((player[active_player]->time+999)/1000)*1000;
	}	
}

void start_thread()
{
	if (!hase_game->local)
	{
		if (!player[active_player]->computer)
		{
			memset(send_data,0,1536*sizeof(char));
			if (player[active_player]->local)
			{
				printf("Starting Push Thread for player %s\n",player[active_player]->name);
				start_push_thread();
			}
			else
			{
				printf("Starting Pull Thread for player %s\n",player[active_player]->name);
				start_pull_thread(player[active_player]);
			}
		}
	}
}

pItem dropItem = NULL;

void real_next_player()
{
	spSoundPause(1,-1);
	stop_thread(0);
	int j;
	for (j = 0; j < hase_game->player_count; j++)
		if (player[j]->activeHare == NULL)
		{
			player[j]->activeHare = player[j]->setActiveHare;
			player[j]->setActiveHare = NULL;
		}
	ai_shoot_tries = 0;
	last_ai_try = 0;
	lastAIDistance = 100000000;
	do
	{
		active_player = (active_player+1)%player_count;
	}
	while (player[active_player]->firstHare == NULL);
	player[active_player]->activeHare = player[active_player]->activeHare->next;
	if (player[active_player]->computer)
		player[active_player]->activeHare->direction = spRand()&1;
	countdown = hase_game->seconds_per_turn*1000;
	pHare hare = player[active_player]->firstHare;
	if (hare)
	do
	{
		hare->hops = 0;
		hare->high_hops = 0;
		hare = hare->next;
	}
	while (hare != player[active_player]->firstHare);
	player[active_player]->weapon_points = 3 + player[active_player]->next_round_extra;
	player[active_player]->next_round_extra = 0;

	extra_time = 0;
	memset(input_states,0,sizeof(int)*12);
	wp_choose = 0;
	if (spRand()/1337%player_count == 0)
		dropItem = items_drop(spRand()/1337%ITEMS_COUNT,-1,-1);
	update_targeting();
	start_thread();
	spSoundPause(0,-1);
}

void check_next_player()
{
	if (next_player_go && bullet_alpha() == 0)
	{
		next_player_go = 0;
		real_next_player();
	}
}

pHare add_hare(pHare* firstHare)
{
	pHare hare = (pHare)malloc(sizeof(tHare));
	if (*firstHare)
	{
		pHare last = (*firstHare)->before;
		last->next = hare;
		hare->before = last;
		hare->next = *firstHare;
		(*firstHare)->before = hare;
	}
	else
	{
		*firstHare = hare;
		hare->next = hare;
		hare->before = hare;
	}
	return hare;
}

#define SQRT_2 92672

Sint32 vector_length_approx(Sint32 x,Sint32 y)
{
	x = abs(x);
	y = abs(y);
	if (x > y)
	{
		Sint32 factor = spDiv(y,spMax(1,x));
		return spMul(SP_ONE-factor+spMul(factor,SQRT_2),x);
	}
	else
	{
		Sint32 factor = spDiv(x,spMax(1,y));
		return spMul(SP_ONE-factor+spMul(factor,SQRT_2),y);
	}
}

Sint32 vector_length_guess(Sint32 x,Sint32 y)
{
	x = abs(x);
	y = abs(y);
	return spMax(x,y);
}



pHare del_hare(pHare hare,pHare* firstHare)
{
	pHare next = NULL;
	if (hare->next == hare)
		*firstHare = NULL;
	else
	{
		hare->before->next = hare->next;
		hare->next->before = hare->before;
		if (*firstHare == hare)
			*firstHare = hare->next;
		next = hare->next;
	}
	if (gop_particles() != 4)
	{
		int inc = (1<<gop_particles()+gop_particles()-1)-1;
		spParticleBunchPointer bunch = spParticleFromSprite(hare->hase->active,hare_explosion_feedback,&particles);
		//Calculating the real position
		//rotating + tranforming to fixed point
		int i;
		for (i = 0; i < bunch->count; i++)
		{
			if ((i & inc) != inc)
			{
				bunch->particle[i].status = -1;
				continue;
			}
			Sint32 tx = spCos(hare->rotation) * (bunch->particle[i].x-16) - spSin(hare->rotation) * (bunch->particle[i].y-16) >> 1;
			Sint32 ty = spSin(hare->rotation) * (bunch->particle[i].x-16) + spCos(hare->rotation) * (bunch->particle[i].y-16) >> 1;
			bunch->particle[i].x = hare->x + tx;
			bunch->particle[i].y = hare->y + ty;
			bunch->particle[i].dx /= 4;
			bunch->particle[i].dy /= 4;
		}
	}
	spDeleteSpriteCollection(hare->hase,0);
	free(hare);
	return next;
}

void init_player(pPlayer player_list,int pc,int hc)
{
	lastAIDistance = 100000000;
	dropItem = NULL;
	next_player_go = 0;
	player_count = pc;
	player = (pPlayer*)malloc(sizeof(pPlayer)*pc);
	while (player_list)
	{
		printf("Setting player %i (%i)\n",player_list->id,player_list->position_in_game);
		player[player_list->position_in_game] = player_list;
		player_list = player_list->next;
	}
	int i,j;
	for (i = 0; i < player_count; i++)
	{
		player[i]->next_round_extra = 0;
		player[i]->firstHare = NULL;
	}
	for (i = 0; i < player_count; i++)
	{
		player[i]->kicked = 0;
		player[i]->weapon_points = 0;
		player[i]->d_time = 0;
		player[i]->d_health = 0;
		player[i]->time = 0;
		for (j = 0; j < hc; j++)
		{
			pHare hare = add_hare(&(player[i]->firstHare));
			hare->wp_x = 0;
			hare->wp_y = 0;
			hare->direction = 0;
			hare->w_direction = SP_ONE/2;
			hare->w_power = SP_ONE/2;
			hare->hops = 0;
			hare->high_hops = 0;
			int x,y;
			while (1)
			{
				x = spRand()%LEVEL_WIDTH;
				y = spRand()%LEVEL_HEIGHT;
				//printf("Tried %i %i... ",x,y);
				if (circle_is_empty(x,y,16,hare,1) && gravitation_force(x,y)/32768)
					break;
				//printf("NOT!\n");
			}
			//printf("Fine.\n");
			hare->x = x << SP_ACCURACY;
			hare->y = y << SP_ACCURACY;
			hare->dx = 0;
			hare->dy = 0;
			hare->health = MAX_HEALTH;
			hare->cam_rotation = hare->rotation;
			char buffer[256];
			sprintf(buffer,"./sprites/hase%i.ssc",player[i]->nr);
			hare->hase = spLoadSpriteCollection(buffer,NULL);
		}
		player[i]->activeHare = player[i]->firstHare;
		player[i]->setActiveHare = NULL;
		for (j = 0; j < TRACE_COUNT; j++)
			player[i]->trace[j] = NULL;
		player[i]->tracePos = 0;
	}
	update_player();
	active_player = 0;
	posX = player[active_player]->firstHare->x;
	posY = player[active_player]->firstHare->y;
	rotation = -player[active_player]->firstHare->rotation;
	ai_shoot_tries = 0;
	last_ai_try = 0;
	player[active_player]->weapon_points = 3;
	extra_time = 0;
	start_thread();
}

