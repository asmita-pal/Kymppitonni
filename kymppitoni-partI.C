#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <sys/time.h>
#define MAX_DICE 6

#define VERBOSE 0 

// Globals
int games;

typedef struct{
  char num_dice;
  char triple;
  char face[MAX_DICE];
}dice_t;

void zap(dice_t *d) {
  d->triple = 0;
  bzero(d->face,sizeof(char)*MAX_DICE);
}

void display(dice_t *d) {
  for(int i=0;i<MAX_DICE;++i)
    for(int j=0;j<d->face[i];++j)
      printf(" %d",i+1);
  printf("\n");
}

void throw_dice(dice_t *d) {
  zap(d);
  for(int i=0;i<d->num_dice;++i) {
    int dice = random() % 6;
    d->face[dice]++;
  }
}

int straightsix(dice_t *d) {
  if (d->num_dice != 6)
    return 0;
  for(int i=0;i<MAX_DICE;++i)
    if (d->face[i] != 1)
      return 0;
  return 1;
}

int triple(dice_t *d) {
  if (d->num_dice < 3)
    return 0;
  for(int i=0;i<MAX_DICE;++i)
    if (d->face[i] >= 3)
      return i+1;
  return 0;
}

int allonesandfives(dice_t *d) {
  if (d->num_dice > (d->face[0] + d->face[4]))
    return 0;
  return 100*d->face[0] + 50*d->face[4];
}

int analyze_dice(dice_t *saved, dice_t *inplay) {
  int score = 0, dicesaved = 0;
  if (straightsix(inplay)) {
    saved->num_dice = 0;
    zap(saved);
    /* early out since all 6 dice used up */
    return 1000;
  }
  /* if a saved triple, see if I can expand it */
  int t=saved->triple;
  if (t) {
    if (inplay->face[t-1]) {
      int oldtriplescore = ((t==1) ? 10 : t) * 100;
      for(int i=3;i<saved->face[t-1];++i)
	oldtriplescore *= 2;
      score = oldtriplescore;
      for(int i=0;i<inplay->face[t-1];++i)
	score *= 2;
      /* only want add'l score on this roll so subtract saved multiplier */
      score -= oldtriplescore;
      /* save dice */
      saved->face[t-1] += inplay->face[t-1];
      saved->num_dice += inplay->face[t-1];
      dicesaved += inplay->face[t-1];
      inplay->num_dice -= inplay->face[t-1];
      inplay->face[t-1] = 0;
    }
  }
  /* if a new triple (or better), use it */
  /* make this a while loop since could have two different triples */
  while (t=triple(inplay)) {
    int triplescore = ((t==1) ? 10 : t) * 100;
    for(int i=3;i<inplay->face[t-1];++i)
      triplescore *= 2;
    score += triplescore;
    /* remember triple */
    saved->triple = t;
    /* save dice */
    saved->face[t-1] += inplay->face[t-1];
    saved->num_dice += inplay->face[t-1];
    dicesaved += inplay->face[t-1];
    inplay->num_dice -= inplay->face[t-1];
    inplay->face[t-1] = 0;
  }

  /* if all remaining dice are ones or fives, use them all */
  /* FIXME: if I have a triple I may want to keep trying for it */
  if (t=allonesandfives(inplay)) {
    score += t;
    /* clear saved dice */
    saved->num_dice = 0;
    zap(saved);
    inplay->num_dice = 6;
  }

  if (score == 0) {
    if (inplay->face[0]) {
      /* use a single one if you have it */
      inplay->num_dice--;
      inplay->face[0]--;
      saved->num_dice++;
      saved->face[0]++;
      score += 100;
    } else if (inplay->face[4]) {
      /* use a single five if you have it */
      inplay->num_dice--;
      inplay->face[4]--;
      saved->num_dice++;
      saved->face[4]++;
      score += 50;
    }
  }

  /* if we used all the dice, then we can throw all of them again */
  if (inplay->num_dice == 0) {
    saved->num_dice = 0;
    zap(saved);
    inplay->num_dice = 6;
  }

  return score;
}


int play_turn(int cutoff, long long* inplay_throws, long long* inplay_points, long long* inplay_duds) {
  int score = 0;
  dice_t saved, inplay;
  saved.num_dice = 0;
  zap(&saved);
  inplay.num_dice = 6;
#if VERBOSE
  printf(" Starting next turn\n");
#endif
  while (inplay.num_dice && (score < cutoff)) {
    throw_dice(&inplay);
#if VERBOSE
    printf("              Thrown: ");
    display(&inplay);
#endif
    int cur_dice = inplay.num_dice;
    int s = analyze_dice(&saved,&inplay);
    inplay_points[cur_dice] += s;
    inplay_throws[cur_dice]++;
#if VERBOSE
    printf("  Points %5d Saved: ",s);
    display(&saved);
#endif
    if (s == 0){
      inplay_duds[cur_dice]++;
      return 0;
    }
    score += s;
  }
#if VERBOSE
  printf(" Turn score: %d\n",score);
#endif
  return score;
}

int play_game(int cutoff, long long* inplay_throws, long long* inplay_points, long long* inplay_duds) {
  int score = 0;
  int turns = 0;

  for(int i=1; i<7; i++){
    inplay_throws[i] = 0;
    inplay_points[i] = 0;
    inplay_duds[i] = 0;
  }

#if VERBOSE
  printf("Starting new game with cutoff %d\n",cutoff);
#endif
  while (score < 10000) {
    score += play_turn(cutoff, inplay_throws, inplay_points, inplay_duds);
    ++turns;
#if VERBOSE
    printf(" Running score after turn %d: %d\n",turns,score);
#endif
  }
#if VERBOSE
  printf("Ending game with cutoff %d after %d turns\n",cutoff,turns);
#endif
  return turns;
}

int main( int argc, char** argv) {
  // Parse command line arguements
  if(argc!=4){
    printf("Error: Impropper usage.  %s <num games> <mincutoff> <maxcutoff>\n", argv[0]);
    exit(0);
  }
  games = atoi(argv[1]);
  int mincutoff = atoi(argv[2]);
  int maxcutoff = atoi(argv[3]);
  printf("Playing %d games for each cutoff ranging from %d-%d\n",games, mincutoff, maxcutoff);

  timeval start, finish; 
  if (gettimeofday(&start, NULL) != 0)
  {
	printf("Cannot get the time!! \n");
        exit(1);
  }

  long long total_inplay_throws[7];
  long long total_inplay_points[7];
  long long total_inplay_duds[7];

  long long max_turns = 0;
  long long min_turns = games*10000; // larger than should be possible
  int max_cutoff;
  int min_cutoff;

  timeval parallel1_start, parallel1_finish;
  if (gettimeofday(&parallel1_start, NULL) != 0)
  {
        printf("Cannot get the time!! \n");
        exit(1);
  }

  for(int i=1; i<7; i++){
    total_inplay_throws[i] = 0;
    total_inplay_points[i] = 0;
    total_inplay_duds[i] = 0;
  }
  
  if (gettimeofday(&parallel1_finish, NULL) != 0)
  {
        printf("Cannot get the time!! \n");
        exit(1);
  }

  int totalturns;

  timeval parallel2_start, parallel2_finish;
  if (gettimeofday(&parallel2_start, NULL) != 0)
  {
        printf("Cannot get the time!! \n");
        exit(1);
  }

  for(int c=mincutoff; c<maxcutoff; c+=50){
    long long cutoffturns = 0;
    for(int i=0;i<games;++i) {
      long long inplay_duds[7];
      long long inplay_throws[7];
      long long inplay_points[7];
      for(int i=1; i<7; i++){
        inplay_duds[i] = 0;
        inplay_throws[i] = 0;
        inplay_points[i] = 0;
      }
      int turns = play_game(c, inplay_throws, inplay_points, inplay_duds);
#if VERBOSE
      printf("Game %d Cutoff %d Turns %d\n",i,c,turns);
#endif
      for(int i=1; i<7; i++){
        total_inplay_throws[i] += inplay_throws[i];
        total_inplay_points[i] += inplay_points[i];
        total_inplay_duds[i] += inplay_duds[i];   
      }
      cutoffturns += turns;
    }
    if(cutoffturns > max_turns){
      max_turns = cutoffturns;
      max_cutoff = c;
    }else if(cutoffturns < min_turns){
      min_turns = cutoffturns;
      min_cutoff = c;
    }
  }
  if (gettimeofday(&parallel2_finish, NULL) != 0)
  {
        printf("Cannot get the time!! \n");
        exit(1);
  }
  long unsigned int time_taken_parallel = 1e6 * (parallel1_finish.tv_sec - parallel1_start.tv_sec) + 
					  1e6 * (parallel2_finish.tv_sec - parallel2_start.tv_sec) +
						(parallel1_finish.tv_usec - parallel1_start.tv_usec) +
						(parallel2_finish.tv_usec - parallel2_start.tv_usec); 
  printf("Execution time in microseconds for parallel part: %lu \n", time_taken_parallel);

   
  if (gettimeofday(&finish, NULL) != 0)
  {
	printf("Cannot get the time!! \n");
        exit(1);
  }
 
  long unsigned int time_taken = 1e6 * (finish.tv_sec - start.tv_sec) + (finish.tv_usec - start.tv_usec); 
  printf("Execution time in microseconds: %lu", time_taken);

  // Display results
  printf("\nPlayed %d games per cutoff leading to the folling statistics:\n", games);
  printf("\tBest Cutoff(%d) resulted in %.2f turns on average\n", min_cutoff, (double)min_turns/(double)games);
  printf("\tWorst Cutoff(%d) resulted in %.2f turns on average\n", max_cutoff, (double)max_turns/(double)games);
  for(int i=1; i<7; i++){
    printf("\tAverage points accumualted with %d dice in play: %.2f\n", i, (double)total_inplay_points[i]/(double)total_inplay_throws[i]);
  }
  for(int i=1; i<7; i++){
    printf("\tProbability of throwing a dud with %d dice in play: %.2f\n", i, (double)total_inplay_duds[i]/(double)total_inplay_throws[i]);
  }

 
  return 1;
}
