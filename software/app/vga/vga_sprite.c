#include <hf-risc.h>
#include "vga_drv.h"
#include "sprites.h"

#define ENEMY_ROWS 5
#define ENEMY_COLS 11
#define TOTAL_ENEMIES (ENEMY_ROWS * ENEMY_COLS)
#define SHIELD_POSY 160
#define SHIELD_POSX 20
#define SHIELD_DURABILITY 20

#define LIFE3X 10
#define LIFE3Y 10
#define LIFE2X 25
#define LIFE2Y 10
#define LIFE1X 40
#define LIFE1Y 10

#define SCOREX 240
#define SCOREY 10

#define PLAYERLIVES 3
#define MAX_PLAYER_BULLETS 5
#define MAX_ENEMY_BULLETS 5

void draw_sprite(unsigned int x, unsigned int y, char *sprite,
                 unsigned int sizex, unsigned int sizey, int color)
{
    unsigned int px, py;
    
    if (color < 0) {
        for (py = 0; py < sizey; py++)
            for (px = 0; px < sizex; px++)
                display_pixel(x + px, y + py, sprite[py * sizex + px]);
    } else {
        for (py = 0; py < sizey; py++)
            for (px = 0; px < sizex; px++)
                display_pixel(x + px, y + py, color & 0xf);
    }
}

/* sprite based objects */
/* sprite based objects */
struct object_s {
    char *sprite_frame[3];
    char spriteszx, spriteszy;
    int cursprite;
    unsigned int posx, posy;
    int dx, dy;
    int speedx, speedy;
    int speedxcnt, speedycnt;
    int lives;
    int points; // NOVO CAMPO PARA PONTUAÇÃO
};

void init_object(struct object_s *obj, char *spritea, char *spriteb,
                 char *spritec, char spriteszx, char spriteszy, int posx, int posy,
                 int dx, int dy, int spx, int spy, int points) 
{
    obj->sprite_frame[0] = spritea;
    obj->sprite_frame[1] = spriteb;
    obj->sprite_frame[2] = spritec;
    obj->spriteszx = spriteszx;
    obj->spriteszy = spriteszy;
    obj->cursprite = 0;
    obj->posx = posx;
    obj->posy = posy;
    obj->dx = dx;
    obj->dy = dy;
    obj->speedx = spx;
    obj->speedy = spy;
    obj->speedxcnt = spx;
    obj->speedycnt = spy;
    obj->lives = 1;
    obj->points = points; 
}

void draw_object(struct object_s *obj, char chgsprite, int color)
{
    if (chgsprite) {
        obj->cursprite++;
        if (obj->sprite_frame[obj->cursprite] == 0)
            obj->cursprite = 0;
    }
    
    draw_sprite(obj->posx, obj->posy, obj->sprite_frame[obj->cursprite],
                obj->spriteszx, obj->spriteszy, color);
}

void move_object(struct object_s *obj)
{
    struct object_s oldobj;
    
    memcpy(&oldobj, obj, sizeof(struct object_s));
    
    if (--obj->speedxcnt == 0) {
        obj->speedxcnt = obj->speedx;
        obj->posx = obj->posx + obj->dx;

        if (obj->posx + obj->spriteszx >= VGA_WIDTH || obj->posx <= 0) obj->dx = -obj->dx; 
    }
    if (--obj->speedycnt == 0) {
        obj->speedycnt = obj->speedy;
        obj->posy = obj->posy + obj->dy;

        if (obj->posy + obj->spriteszy >= VGA_HEIGHT || obj->posy <= 0) obj->dy = -obj->dy;
    }

    if ((obj->speedx == obj->speedxcnt) || (obj->speedy == obj->speedycnt)) {
        draw_object(&oldobj, 0, 0);
        draw_object(obj, 1, -1);
    }
}

/* display and input */
void init_display()
{
    display_background(BLACK);
}

enum {
    KEY_CENTER  = 0x01,
    KEY_UP      = 0x02,
    KEY_LEFT    = 0x04,
    KEY_RIGHT   = 0x08,
    KEY_DOWN    = 0x10
};

void init_input()
{
    /* configure GPIOB pins 8 .. 12 as inputs */
    GPIOB->DDR &= ~(MASK_P8 | MASK_P9 | MASK_P10 | MASK_P11 | MASK_P12);
}

int get_input()
{
    int keys = 0;
    
    if (GPIOB->IN & MASK_P8)    keys |= KEY_CENTER;
    if (GPIOB->IN & MASK_P9)    keys |= KEY_UP;
    if (GPIOB->IN & MASK_P10)   keys |= KEY_LEFT;
    if (GPIOB->IN & MASK_P11)   keys |= KEY_RIGHT;
    if (GPIOB->IN & MASK_P12)   keys |= KEY_DOWN;
        
    return keys;
}

int detect_collision(struct object_s *obj1, struct object_s *obj2)
{
    if (obj1->lives && obj2->lives &&
        obj1->posx < obj2->posx + obj2->spriteszx &&
        obj1->posx + obj1->spriteszx > obj2->posx &&
        obj1->posy < obj2->posy + obj2->spriteszy &&
        obj1->posy + obj1->spriteszy > obj2->posy) return 1;

    return 0;
}

void init_enemy_fleet(struct object_s *enemies)
{
    unsigned int row, col;
    int start_x = 15;
    int start_y = 45;
    int spacing_x = 16;
    int spacing_y = 12;

    for (row = 0; row < ENEMY_ROWS; row++) {
        for (col = 0; col < ENEMY_COLS; col++) {
            struct object_s *current_enemy = &enemies[row * ENEMY_COLS + col];
            char *sprite_a = 0;
            char *sprite_b = 0;
            char size_x = 0;
            char size_y = 0;
            int points = 0; // Variável para os pontos

            if (row == 0) { // Invasor tipo 3
                sprite_a = (char *)monster3a;
                sprite_b = (char *)monster3b;
                size_x = 12; size_y = 8;
                points = 10;
            } else if (row < 3) { // Invasor tipo 2
                sprite_a = (char *)monster2a;
                sprite_b = (char *)monster2b;
                size_x = 11; size_y = 8;
                points = 20;
            } else { // Invasor tipo 1
                sprite_a = (char *)monster1a;
                sprite_b = (char *)monster1b;
                size_x = 8; size_y = 8;
                points = 30;
            }

            int pos_x = start_x + col * spacing_x;
            int pos_y = start_y + row * spacing_y;

            // Passa a pontuação para a função de inicialização
            init_object(current_enemy, sprite_a, sprite_b, 0,
                        size_x, size_y, pos_x, pos_y, 0, 0, 0, 0, points);
        }
    }
}

void move_player(struct object_s *obj)
{
    struct object_s oldobj;
    
    memcpy(&oldobj, obj, sizeof(struct object_s));
    
    if (--obj->speedxcnt == 0) {
        obj->speedxcnt = obj->speedx;
        int next_posx = (int)obj->posx + obj->dx;
       
        if( next_posx < 0){
            next_posx = 0;
        }
        if (next_posx + obj->spriteszx >= VGA_WIDTH) {
            next_posx = VGA_WIDTH - obj->spriteszx;
        }
        obj->posx = (unsigned int)next_posx;
    }

    if (obj->speedx == obj->speedxcnt) {
        draw_object(&oldobj, 0, 0);
        draw_object(obj, 1, -1);
    }
}

void kill_enemy(struct object_s *enemy) {
    if (enemy->lives != 1) return;
    draw_object(enemy, 0, 0); 
    draw_sprite(enemy->posx, enemy->posy, (char*)invaderExplosion, 13, 7, -1);
    delay_ms(10); 
    draw_sprite(enemy->posx, enemy->posy, (char*)invaderExplosion, 13, 7, 0); 
    enemy->lives = 0;
}

void move_enemy_fleet(struct object_s *enemies, int *fleet_dx, int *gamestate)
{
    if(*fleet_dx == 0) return;

    int move_down = 0;
    int step_down = 4;
    
    for (int i = 0; i < TOTAL_ENEMIES; i++) {
        if (!enemies[i].lives) continue;
        if ((enemies[i].posx + enemies[i].spriteszx >= VGA_WIDTH - 1 && *fleet_dx > 0) || (enemies[i].posx <= 1 && *fleet_dx < 0)) {
            *fleet_dx = -*fleet_dx;
            move_down = 1;
            break; 
        }
    }

    for (int i = 0; i < TOTAL_ENEMIES; i++) {
        if(enemies[i].lives){
            struct object_s oldenemy;
            struct object_s *enemy = &enemies[i];
            
            memcpy(&oldenemy, enemy, sizeof(struct object_s));
            
            enemy->posx += *fleet_dx; 
            if (move_down) {
                if(enemy->posy + step_down > SHIELD_POSY - enemy->spriteszy) {
                    enemy->posy = SHIELD_POSY - enemy->spriteszy;
                    *gamestate = 4;
                }
                else
                    enemy->posy += step_down;
            }
            
            draw_object(&oldenemy, 0, 0);
            draw_object(enemy, 1, -1);
        }
    }
}


void move_bullet(struct object_s *bullet) {
    if (!bullet->lives) return;

    draw_sprite(bullet->posx, bullet->posy, bullet->sprite_frame[0], bullet->spriteszx, bullet->spriteszy, 0);

    bullet->posy += bullet->dy;

    if (bullet->posy <= 25 || bullet->posy >= VGA_HEIGHT) {
        bullet->lives = 0; 
        return;
    }

    draw_sprite(bullet->posx, bullet->posy, bullet->sprite_frame[0], bullet->spriteszx, bullet->spriteszy, -1);
}


void fire_player_bullet(struct object_s *player, struct object_s *bullets) {
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (!bullets[i].lives) {
            int bullet_x = player->posx + (player->spriteszx / 2) - 2; 
            int bullet_y = player->posy;
            init_object(&bullets[i], (char*)playerBullet, 0, 0, 5, 3,
                        bullet_x, bullet_y, 0, -4, 1, 1, 0); 
            return; 
        }
    }
}

void fire_enemy_bullet(struct object_s *enemies, struct object_s *bullets) {
    // Chance de atirar (3% a cada frame)
    if (random() % 100 > 97) {
        int bullet_slot = -1;
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            if (!bullets[i].lives) {
                bullet_slot = i;
                break;
            }
        }
        if (bullet_slot == -1) return; 

        int alive_enemies_idx[TOTAL_ENEMIES];
        int num_alive_enemies = 0;
        for (int i = 0; i < TOTAL_ENEMIES; i++) {
            if (enemies[i].lives) {
                alive_enemies_idx[num_alive_enemies++] = i;
            }
        }

        if (num_alive_enemies > 0) {
            int random_enemy_idx = alive_enemies_idx[random() % num_alive_enemies];
            struct object_s *shooter = &enemies[random_enemy_idx];

            int bullet_x = shooter->posx + (shooter->spriteszx / 2) - 2;
            int bullet_y = shooter->posy + shooter->spriteszy;
            
            init_object(&bullets[bullet_slot], (char*)enemyBullet, 0, 0, 5, 3,
                        bullet_x, bullet_y, 0, 4, 1, 1, 0); 
        }
    }
}

void update_shield(struct object_s *s) {
    int px = 4;
    int py = 4;
    char str;
    if(s->lives > 0){
        s->lives = s->lives - 1;
        if(s->lives == 0) {
            draw_object(s, 0, 0);
            return;
        }
        if(s->lives < 10){
            px += 5;
        }
        itoa(s->lives + 1, &str, 10);
        if(s->lives + 1 == 10) display_print(&str, s->posx + px - 5, s->posy + py, 1, 2);
        else display_print(&str, s->posx + px, s->posy + py, 1, 2);
        itoa(s->lives, &str, 10);

        display_print(&str, s->posx + px, s->posy + py, 1, 0);
    }
}


void init_shields(struct object_s *s){
    int posx = SHIELD_POSX;
    char str;
    for(int i = 0; i < 4; i++) {
        struct object_s *current_shield = &s[i];
        init_object(current_shield, (char*)shield, 0, 0, 24, 18, posx, SHIELD_POSY, 0, 0, 0, 0, 0);
        current_shield->lives = SHIELD_DURABILITY;
        draw_object(current_shield, 0, -1);
        itoa(current_shield->lives, &str, 10);
        display_print(&str, current_shield->posx + 4, current_shield->posy + 4, 1, 0);
        posx = posx + 80;
    }
}

void update_player(struct object_s *p, int *gamestate){
    
    if(p->lives > 0){
        p->lives = p->lives - 1;
        if(p->lives == 2) {
            draw_sprite(LIFE3X, LIFE3Y, (char*)playerLivesSprite, 13, 8, 0);
            return;
        }
        else if(p->lives == 1) {
            draw_sprite(LIFE2X, LIFE2Y, (char*)playerLivesSprite, 13, 8, 0);
            return;
        }
        else if(p->lives == 0) draw_sprite(LIFE1X, LIFE1Y, (char*)playerLivesSprite, 13, 8, 0);
    }
    if(p->lives == 0){
        *gamestate = 4;
        draw_object(p, 0, 0); 
        draw_sprite(p->posx, p->posy, (char*)playerExplosion, 15, 8, -1);

    }   

}

void update_score(int *score, int points) {
    char str[12];
    itoa(*score, str, 10);
    display_print(str, SCOREX, SCOREY, 1, 0);
    *score += points;
    itoa(*score, str, 10);
    display_print(str, SCOREX, SCOREY, 1, 2);
}

void spawn_mysteryship(struct object_s *s){
    // 0.05% de chance por frame
    if (random() % 1000 > 995) {
        // pontos entre 100 e 150
        int points = 100 + random() % 50;
        init_object(s, (char*)mysteryShip, 0, 0, 16, 7, 15, 35, 2, 0, 2, 0, points);
    }

}


/* main game loop */
int main(void){
    int fleet_dx = 1;
    int score = 0;
    int fire_delay = 5; // 5 frames
    int aux_delay = fire_delay;
    int gamestate = 0;
    

    struct object_s enemies[TOTAL_ENEMIES];
    struct object_s player1;
    struct object_s shields[4];
    struct object_s mysteryship1;

    struct object_s player_bullets[MAX_PLAYER_BULLETS];
    struct object_s enemy_bullets[MAX_ENEMY_BULLETS];


    char *init_str1 = "SPACE INVADERS";
    int init_str1x = 30;
    int init_str1y = 40;
    char *init_str2 = "PRESSIONE QUALQUER BOTAO";
    int init_str2x = 40;
    int init_str2y = 180;
    char *lost_str = "VOCE PERDEU";
    int lost_strx = 30;
    int lost_stry = 30;
    char *score_str = "PONTOS";
    int score_strx = 170;
    int score_stry = 10;

    int total_enemies_alive = 0;
    init_input();

    while (1) {
        int key = get_input();
        if(gamestate == 0){
            init_display();
            display_print(init_str1, init_str1x, init_str1y, 2, 2);
            display_print(init_str2, init_str2x, init_str2y, 1, 2);
            gamestate = 1;
            
        }
        else if(gamestate == 1){
            if(key) {
                gamestate = 2;
            }
        }
        else if(gamestate == 2){
            init_display();
            score = 0;
            fleet_dx = 1;
            init_shields(shields);
            display_print(score_str, score_strx, score_stry, 1, 2);
            update_score(&score, 0);
            init_object(&player1, (char*)player, 0, 0, 13, 8, 150, 190, 0, 4, 1, 0, 0);
            player1.lives = PLAYERLIVES;
            for (int i = 0; i < MAX_PLAYER_BULLETS; i++) player_bullets[i].lives = 0;
            for (int i = 0; i < MAX_ENEMY_BULLETS; i++)  enemy_bullets[i].lives = 0;
            draw_sprite(LIFE3X, LIFE3Y, (char*)playerLivesSprite, 13, 8, -1);
            draw_sprite(LIFE2X, LIFE2Y, (char*)playerLivesSprite, 13, 8, -1);
            draw_sprite(LIFE1X, LIFE1Y, (char*)playerLivesSprite, 13, 8, -1);
            init_enemy_fleet(enemies);
            total_enemies_alive = TOTAL_ENEMIES;
            gamestate = 3;
        }
        else if(gamestate == 3) {
            if(total_enemies_alive == 0) gamestate = 5;
            
            if (mysteryship1.lives != 0) {
                move_object(&mysteryship1);
            } else {
                spawn_mysteryship(&mysteryship1);
            }
            move_player(&player1);
            move_enemy_fleet(enemies, &fleet_dx, &gamestate);
            
            for (int i = 0; i < MAX_PLAYER_BULLETS; i++) move_bullet(&player_bullets[i]);
            for (int i = 0; i < MAX_ENEMY_BULLETS; i++)  move_bullet(&enemy_bullets[i]);
            
            fire_enemy_bullet(enemies, enemy_bullets);

            
            
            // Colisão: Tiro do jogador vs Inimigos e Escudos
            for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
                if (!player_bullets[i].lives) continue;
                if(mysteryship1.lives && detect_collision(&player_bullets[i], &mysteryship1)){
                    kill_enemy(&mysteryship1);
                    update_score(&score, mysteryship1.points);
                }
                for (int j = 0; j < TOTAL_ENEMIES; j++) {

                    // Verifica se o inimigo está vivo antes da colisão
                    if (enemies[j].lives && detect_collision(&player_bullets[i], &enemies[j])) {
                        
                        
                        update_score(&score, enemies[j].points);
                        

                        kill_enemy(&enemies[j]);
                        total_enemies_alive--;
                        player_bullets[i].lives = 0;
                        draw_sprite(player_bullets[i].posx, player_bullets[i].posy, player_bullets[i].sprite_frame[0], player_bullets[i].spriteszx, player_bullets[i].spriteszy, 0);
                        break; 
                    }
                }
                for (int k = 0; k < 4; k++) {
                    if (shields[k].lives > 0 && detect_collision(&player_bullets[i], &shields[k])) {
                        player_bullets[i].lives = 0;
                        update_shield(&shields[k]); 
                        draw_sprite(player_bullets[i].posx, player_bullets[i].posy, player_bullets[i].sprite_frame[0], player_bullets[i].spriteszx, player_bullets[i].spriteszy, 0);
                        break;
                    }
                }
                
            }
            
            // Colisão: Tiro do Inimigo vs Jogador e Escudos
            for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
                if (!enemy_bullets[i].lives) continue;
                if(detect_collision(&enemy_bullets[i], &player1)) {
                    update_player(&player1, &gamestate);
                    enemy_bullets[i].lives = 0;
                    draw_sprite(enemy_bullets[i].posx, enemy_bullets[i].posy, enemy_bullets[i].sprite_frame[0], enemy_bullets[i].spriteszx, enemy_bullets[i].spriteszy, 0);
                }
                for (int j = 0; j < 4; j++) {
                    if (shields[j].lives > 0 && detect_collision(&enemy_bullets[i], &shields[j])) {
                        enemy_bullets[i].lives = 0;
                        update_shield(&shields[j]);
                        draw_sprite(enemy_bullets[i].posx, enemy_bullets[i].posy, enemy_bullets[i].sprite_frame[0], enemy_bullets[i].spriteszx, enemy_bullets[i].spriteszy, 0);
                        break;
                    }
                }
            }

            int key = get_input();
            if (key & KEY_LEFT) {
                player1.dx = -4;
            } else if (key & KEY_RIGHT) {
                player1.dx = 4;
            } else {
                player1.dx = 0;
            } 
            if(key & KEY_CENTER && aux_delay == fire_delay){
                fire_player_bullet(&player1, player_bullets);
                aux_delay = 0;
            }
            else if(aux_delay < fire_delay){
                aux_delay++;
            }
            
            
        }
        else if(gamestate == 4) {
            init_display();
            display_print(lost_str, lost_strx, lost_stry, 2, 2);
            char str[12];
            itoa(score, str, 10);
            display_print("PONTUACAO:", lost_strx, lost_stry + 25, 1, 2);
            display_print(str, lost_strx + 90, lost_stry + 25, 1, 2);

            display_print(init_str2, init_str2x, init_str2y, 1, 2);
            delay_ms(50);
            gamestate = 1;
            
        }
        else if(gamestate == 5){
            if (fleet_dx > 0) fleet_dx++;
            else fleet_dx--;
            init_enemy_fleet(enemies);
            total_enemies_alive = TOTAL_ENEMIES;
            delay_ms(50);

            gamestate = 3;

        }
        delay_ms(20);
    }

    return 0;
}